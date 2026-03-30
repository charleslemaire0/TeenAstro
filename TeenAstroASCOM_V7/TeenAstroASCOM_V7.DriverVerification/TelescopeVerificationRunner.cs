using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using ASCOM.DeviceInterface;
using ASCOM.LocalServer;
using ASCOM.TeenAstro.Telescope;
using ASCOM.Utilities;

namespace TeenAstroASCOM_V7.DriverVerification
{
  public static class TelescopeVerificationRunner
  {
    public static VerificationReport Run(VerificationOptions options, Action<string> logLine = null)
    {
      void Log(string s)
      {
        logLine?.Invoke(s);
      }

      var report = new VerificationReport();

      void Add(string phase, string name, CheckOutcome outcome, string detail = null)
      {
        report.Checks.Add(new CheckResult(phase, name, outcome, detail));
      }

      // --- Profile ---
      try
      {
        using (var profile = new Profile())
        {
          profile.DeviceType = "Telescope";
          profile.WriteValue(VerificationOptions.DriverProgId, VerificationOptions.TraceKey, options.TraceEnabled ? "true" : "false");
          profile.WriteValue(VerificationOptions.DriverProgId, VerificationOptions.ComPortKey, options.ComPortField);
          profile.WriteValue(VerificationOptions.DriverProgId, VerificationOptions.IpKey, options.IpField);
          profile.WriteValue(VerificationOptions.DriverProgId, VerificationOptions.PortKey, options.PortField.ToString());
          profile.WriteValue(VerificationOptions.DriverProgId, VerificationOptions.InterfaceKey, options.InterfaceType);
        }
        Add("Profile", "WriteASCOMProfile", CheckOutcome.Ok);
      }
      catch (Exception ex)
      {
        Add("Profile", "WriteASCOMProfile", CheckOutcome.Issue, ex.Message);
        report.FinishedUtc = DateTime.UtcNow;
        return report;
      }

      // --- Precheck ---
      try
      {
        var preUid = Guid.NewGuid();
        string transport = options.InterfaceType?.Trim().Equals("COM", StringComparison.OrdinalIgnoreCase) == true ? "COM" : "IP";
        SharedResources.Connect(preUid, options.ComPortField, options.IpField, (short)options.PortField, transport);
        string preBuf = "";
        bool preOk = SharedResources.SendCommand(":GVN#", 2, ref preBuf, retries: options.PrecheckRetries);
        SharedResources.Disconnect(preUid);
        Log($"[PRECHECK] SharedResources {transport} :GVN# ok={preOk} reply='{preBuf}'");
        Add("Precheck", "SharedResources_GVN", preOk ? CheckOutcome.Ok : CheckOutcome.Issue, preOk ? preBuf : "empty or failed");
      }
      catch (Exception ex)
      {
        Log($"[PRECHECK] failed: {ex.Message}");
        Add("Precheck", "SharedResources_GVN", CheckOutcome.Issue, ex.Message);
      }

      if (!options.UseComActivation)
        LocalServerInProcessInit.Apply();

      object telescopeObj = null;
      ITelescopeV4 scope = null;

      try
      {
        Type comDriverType = null;
        if (options.UseComActivation)
        {
          comDriverType = Type.GetTypeFromProgID(VerificationOptions.DriverProgId, throwOnError: false);
          if (comDriverType == null)
          {
            Add("Connect", "ComProgId", CheckOutcome.Issue, "ProgID not registered: " + VerificationOptions.DriverProgId);
            report.FinishedUtc = DateTime.UtcNow;
            return report;
          }
        }

        Exception lastConnectException = null;
        bool connectedOk = false;
        double raProbe = 0;

        for (int attempt = 1; attempt <= options.ConnectRetries; attempt++)
        {
          DisposeTelescope(telescopeObj);
          telescopeObj = null;
          scope = null;

          if (options.UseComActivation)
          {
            telescopeObj = Activator.CreateInstance(comDriverType);
            scope = telescopeObj as ITelescopeV4;
            if (scope == null)
            {
              Add("Connect", "CastITelescopeV4", CheckOutcome.Issue, "Instance does not implement ITelescopeV4");
              report.FinishedUtc = DateTime.UtcNow;
              return report;
            }
          }
          else
          {
            telescopeObj = new Telescope();
            scope = (ITelescopeV4)(object)telescopeObj;
          }

          Log($"[CONNECT] Attempt {attempt}/{options.ConnectRetries}...");
          try
          {
            scope.Connected = true;
            var sw = Stopwatch.StartNew();
            connectedOk = false;
            raProbe = 0;
            while (sw.ElapsedMilliseconds < options.ConnectTimeoutMs)
            {
              try
              {
                raProbe = scope.RightAscension;
                connectedOk = true;
                break;
              }
              catch
              {
                Thread.Sleep(250);
              }
            }

            if (!connectedOk)
              throw new ASCOM.NotConnectedException($"Connect did not become operational within {options.ConnectTimeoutMs}ms.");

            Log($"[OK] Connected (RA probe={raProbe:F4}h). Testing link stability with {options.Runs} polls...");
            Add("Connect", "Connected_RightAscensionProbe", CheckOutcome.Ok, $"RA={raProbe:F4}h");
            break;
          }
          catch (Exception ex)
          {
            lastConnectException = ex;
            Log($"[WARN] Connect attempt {attempt} failed: {ex.GetType().Name}: {ex.Message}");
            try
            {
              if (scope != null) scope.Connected = false;
            }
            catch { }
            DisposeTelescope(telescopeObj);
            telescopeObj = null;
            scope = null;
            Thread.Sleep(1000);
          }
        }

        if (!connectedOk || scope == null)
        {
          Add("Connect", "Operational", CheckOutcome.Issue, lastConnectException?.Message ?? "timeout");
          report.FinishedUtc = DateTime.UtcNow;
          return report;
        }

        // --- Metadata ---
        TryMetadata(scope, Add, Log);

        // --- Can* ---
        TryCanProperties(scope, Add);

        // --- DeviceState ---
        TryDeviceState(scope, Add);

        // --- Poll stability ---
        int okCount = 0;
        int failCount = 0;
        long raTotalMs = 0;
        long decTotalMs = 0;
        for (int run = 1; run <= options.Runs; run++)
        {
          if (!scope.Connected)
          {
            Add("PollStability", $"Run{run}", CheckOutcome.Issue, "driver reported disconnected");
            failCount++;
            break;
          }

          try
          {
            var t1 = Stopwatch.StartNew();
            double ra = scope.RightAscension;
            t1.Stop();
            var t2 = Stopwatch.StartNew();
            double dec = scope.Declination;
            t2.Stop();
            bool tracking = scope.Tracking;
            okCount++;
            raTotalMs += t1.ElapsedMilliseconds;
            decTotalMs += t2.ElapsedMilliseconds;
            Log($"[OK] {run}/{options.Runs} RA={ra:F4}h DEC={dec:F4}deg Tracking={tracking} (RA {t1.ElapsedMilliseconds}ms, DEC {t2.ElapsedMilliseconds}ms)");
          }
          catch (Exception ex)
          {
            failCount++;
            Add("PollStability", $"Run{run}", CheckOutcome.Issue, UnwrapForLogging(ex));
            Log($"[FAIL] Run {run}/{options.Runs}: {ex.Message}");
          }

          Thread.Sleep(options.RunDelayMs);
        }

        string pollDetail = failCount == 0
          ? $"OK={okCount} avgRaMs={(okCount > 0 ? raTotalMs / okCount : 0)} avgDecMs={(okCount > 0 ? decTotalMs / okCount : 0)}"
          : $"OK={okCount} FAIL={failCount}";
        Add("PollStability", "Summary", failCount == 0 ? CheckOutcome.Ok : CheckOutcome.Issue, pollDetail);

        if (options.VerifyRates)
          VerifyRateRoundTrip(scope, Add, Log);

        if (options.VerifyAllGet || options.VerifyAllSet)
        {
          Log("");
          Log("[VERIFY] Enumerating ITelescopeV4 get/set properties...");
          VerifyAllProperties(scope, options.VerifyAllGet, options.VerifyAllSet, options.MaxProperties, options.VerboseVerify, report, Add, Log);
        }
      }
      finally
      {
        try
        {
          if (scope != null)
          {
            try { scope.Connected = false; } catch { }
          }
          DisposeTelescope(telescopeObj);
        }
        catch { }

        telescopeObj = null;
        scope = null;
        GC.Collect();
        GC.WaitForPendingFinalizers();
        report.FinishedUtc = DateTime.UtcNow;
      }

      return report;
    }

    private static void TryMetadata(ITelescopeV4 scope, Action<string, string, CheckOutcome, string> add, Action<string> log)
    {
      void M(string name, Action action)
      {
        try
        {
          action();
          add("Metadata", name, CheckOutcome.Ok, null);
        }
        catch (Exception ex)
        {
          add("Metadata", name, CheckOutcome.Issue, ex.Message);
        }
      }

      M("InterfaceVersion", () => { int v = scope.InterfaceVersion; log?.Invoke($"[META] InterfaceVersion={v}"); });
      M("Description", () => { var s = scope.Description; log?.Invoke($"[META] Description={s}"); });
      M("DriverInfo", () => { var s = scope.DriverInfo; log?.Invoke($"[META] DriverInfo={s}"); });
      M("DriverVersion", () => { var s = scope.DriverVersion; log?.Invoke($"[META] DriverVersion={s}"); });
      M("Name", () => { var s = scope.Name; log?.Invoke($"[META] Name={s}"); });
      M("SupportedActions", () => { var a = scope.SupportedActions; log?.Invoke($"[META] SupportedActions count={a?.Count ?? 0}"); });
    }

    private static void TryCanProperties(ITelescopeV4 scope, Action<string, string, CheckOutcome, string> add)
    {
      Type ifaceType = typeof(ITelescopeV4);
      foreach (var prop in ifaceType.GetProperties())
      {
        if (prop.GetIndexParameters().Length != 0) continue;
        if (!prop.Name.StartsWith("Can", StringComparison.Ordinal)) continue;
        if (!prop.CanRead) continue;
        try
        {
          object val = prop.GetValue(scope, null);
          add("Can", prop.Name, CheckOutcome.Ok, val?.ToString());
        }
        catch (Exception ex)
        {
          add("Can", prop.Name, CheckOutcome.Issue, UnwrapForLogging(ex));
        }
      }
    }

    private static void TryDeviceState(ITelescopeV4 scope, Action<string, string, CheckOutcome, string> add)
    {
      Type ifaceType = typeof(ITelescopeV4);
      var p = ifaceType.GetProperty("DeviceState");
      if (p == null || !p.CanRead)
      {
        add("DeviceState", "DeviceState", CheckOutcome.Skipped, "property not present");
        return;
      }
      try
      {
        object val = p.GetValue(scope, null);
        add("DeviceState", "DeviceState", CheckOutcome.Ok, val != null ? "received" : "null");
      }
      catch (Exception ex)
      {
        if (ex is System.Runtime.InteropServices.COMException || IsNotImplemented(ex))
          add("DeviceState", "DeviceState", CheckOutcome.Skipped, ex.Message);
        else
          add("DeviceState", "DeviceState", CheckOutcome.Issue, UnwrapForLogging(ex));
      }
    }

    private static bool IsNotImplemented(Exception ex)
    {
      return ex is ASCOM.PropertyNotImplementedException || ex is ASCOM.MethodNotImplementedException;
    }

    private static void VerifyRateRoundTrip(ITelescopeV4 telescope, Action<string, string, CheckOutcome, string> add, Action<string> log)
    {
      log?.Invoke("");
      log?.Invoke("[RATE] Hex wire rate round-trip (ASCOM <-> mount)...");
      bool trackingWas = false;
      DriveRates rateWas = DriveRates.driveSidereal;
      try
      {
        trackingWas = telescope.Tracking;
        rateWas = telescope.TrackingRate;
        if (!telescope.Tracking)
          telescope.Tracking = true;
        telescope.TrackingRate = DriveRates.driveSidereal;

        if (!telescope.CanSetRightAscensionRate)
        {
          add("Rates", "RightAscensionRate", CheckOutcome.Skipped, "CanSetRightAscensionRate=false");
          log?.Invoke("[RATE] SKIP RightAscensionRate (CanSetRightAscensionRate=false)");
        }
        else
        {
          double savedRa = telescope.RightAscensionRate;
          const double targetRa = -0.0033333333333333335;
          telescope.RightAscensionRate = targetRa;
          double readRa = telescope.RightAscensionRate;
          double diffRa = Math.Abs(readRa - targetRa);
          if (diffRa > 1e-9)
          {
            add("Rates", "RightAscensionRate", CheckOutcome.Info, $"read {readRa} vs set {targetRa} (diff {diffRa})");
            log?.Invoke($"[RATE] WARN RightAscensionRate read {readRa} vs set {targetRa}");
          }
          else
            add("Rates", "RightAscensionRate", CheckOutcome.Ok, null);
          telescope.RightAscensionRate = savedRa;
        }

        if (!telescope.CanSetDeclinationRate)
        {
          add("Rates", "DeclinationRate", CheckOutcome.Skipped, "CanSetDeclinationRate=false");
          log?.Invoke("[RATE] SKIP DeclinationRate (CanSetDeclinationRate=false)");
        }
        else
        {
          double savedDec = telescope.DeclinationRate;
          const double targetDec = 0.05;
          telescope.DeclinationRate = targetDec;
          double readDec = telescope.DeclinationRate;
          double diffDec = Math.Abs(readDec - targetDec);
          if (diffDec > 1e-9)
          {
            add("Rates", "DeclinationRate", CheckOutcome.Info, $"read {readDec} vs set {targetDec}");
            log?.Invoke($"[RATE] WARN DeclinationRate read {readDec} vs set {targetDec}");
          }
          else
            add("Rates", "DeclinationRate", CheckOutcome.Ok, null);
          telescope.DeclinationRate = savedDec;
        }

        log?.Invoke("[RATE] Done.");
      }
      catch (Exception ex)
      {
        add("Rates", "RoundTrip", CheckOutcome.Issue, UnwrapForLogging(ex));
        log?.Invoke($"[RATE] FAIL: {UnwrapForLogging(ex)}");
      }
      finally
      {
        try
        {
          telescope.TrackingRate = rateWas;
          telescope.Tracking = trackingWas;
        }
        catch (Exception ex)
        {
          add("Rates", "RestoreTracking", CheckOutcome.Issue, UnwrapForLogging(ex));
          log?.Invoke($"[RATE] WARN restore tracking: {UnwrapForLogging(ex)}");
        }
      }
    }

    private static void VerifyAllProperties(ITelescopeV4 iface, bool verifyGet, bool verifySet, int maxProperties, bool verboseVerify,
      VerificationReport report, Action<string, string, CheckOutcome, string> add, Action<string> log)
    {
      Type ifaceType = typeof(ITelescopeV4);
      var skipProps = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
      {
        "Connected",
        "Connecting"
      };

      PropertyInfo[] props = ifaceType.GetProperties();
      int processed = 0;

      foreach (var prop in props)
      {
        if (prop.GetIndexParameters().Length != 0) continue;
        if (skipProps.Contains(prop.Name)) continue;
        if (maxProperties >= 0 && processed >= maxProperties) break;
        processed++;

        object currentValue = null;
        bool hasCurrent = false;

        if (verifyGet && prop.CanRead)
        {
          try
          {
            if (verboseVerify)
            {
              log?.Invoke($"[VERIFY] GET {prop.Name} ...");
            }
            currentValue = prop.GetValue(iface, null);
            hasCurrent = true;
            add("Properties", "GET_" + prop.Name, CheckOutcome.Ok, null);
          }
          catch (Exception ex)
          {
            add("Properties", "GET_" + prop.Name, CheckOutcome.Issue, UnwrapForLogging(ex));
            hasCurrent = false;
          }
        }

        if (verifySet && prop.CanWrite && prop.CanRead && hasCurrent)
        {
          if (!IsSettablePrimitiveLike(prop.PropertyType))
            continue;
          if (prop.PropertyType == typeof(DateTime))
            continue;

          try
          {
            if (verboseVerify)
            {
              log?.Invoke($"[VERIFY] SET {prop.Name} ...");
            }
            prop.SetValue(iface, currentValue, null);
            add("Properties", "SET_" + prop.Name, CheckOutcome.Ok, null);
          }
          catch (Exception ex)
          {
            add("Properties", "SET_" + prop.Name, CheckOutcome.Issue, UnwrapForLogging(ex));
          }
        }
      }

      int getOk = 0, getFail = 0, setOk = 0, setFail = 0;
      foreach (var c in report.Checks)
      {
        if (c.Phase != "Properties") continue;
        if (c.Name.StartsWith("GET_", StringComparison.Ordinal))
        {
          if (c.Outcome == CheckOutcome.Ok) getOk++;
          else if (c.Outcome == CheckOutcome.Issue) getFail++;
        }
        else if (c.Name.StartsWith("SET_", StringComparison.Ordinal))
        {
          if (c.Outcome == CheckOutcome.Ok) setOk++;
          else if (c.Outcome == CheckOutcome.Issue) setFail++;
        }
      }
      log?.Invoke($"[VERIFY SUMMARY] GET ok={getOk} fail={getFail}");
      log?.Invoke($"[VERIFY SUMMARY] SET ok={setOk} fail={setFail}");
    }

    private static bool IsSettablePrimitiveLike(Type t)
    {
      if (t == typeof(string)) return true;
      if (t == typeof(bool)) return true;
      if (t == typeof(byte) || t == typeof(sbyte) || t == typeof(short) || t == typeof(ushort) ||
          t == typeof(int) || t == typeof(uint) || t == typeof(long) || t == typeof(ulong) ||
          t == typeof(float) || t == typeof(double) || t == typeof(decimal)) return true;
      if (t == typeof(DateTime)) return true;
      if (t.IsEnum) return true;
      return false;
    }

    private static string UnwrapForLogging(Exception ex)
    {
      if (ex == null) return "";
      Exception cur = ex;
      while (cur is System.Reflection.TargetInvocationException && cur.InnerException != null)
        cur = cur.InnerException;
      while (cur is AggregateException && cur.InnerException != null)
        cur = cur.InnerException;
      return $"{cur.GetType().Name}: {cur.Message}";
    }

    private static void DisposeTelescope(object telescope)
    {
      if (telescope == null) return;
      try
      {
        if (telescope is IDisposable d)
          d.Dispose();
        else if (Marshal.IsComObject(telescope))
          Marshal.ReleaseComObject(telescope);
      }
      catch { }
    }
  }
}
