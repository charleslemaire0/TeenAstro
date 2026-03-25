using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using ASCOM.Utilities;
using ASCOM.DeviceInterface;
using ASCOM.LocalServer;
using ASCOM.TeenAstro.Telescope;

[System.Runtime.InteropServices.ComVisible(false)]
public class Program
{
  // STA is safer for COM-based drivers.
  [STAThread]
  public static void Main(string[] args)
  {
    // Defaults (can be overridden by args).
    string ipField = "192.168.1.17:9999";
    int portField = 9999;
    string comPortField = "COM1";
    string interfaceType = "IP";
    int runs = 30;
    int runDelayMs = 250;
    int connectTimeoutMs = 15000;
    bool verifyAllGet = true;
    bool verifyAllSet = true;
    int maxProperties = -1; // -1 = no limit
    int connectRetries = 3;
    int precheckRetries = 3;
    bool verboseVerify = false;
    bool traceEnabled = false;
    bool verifyRates = false;

    // Very small arg parser.
    for (int i = 0; i < args.Length; i++)
    {
      string a = args[i];
      if (a.Equals("--ip", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) ipField = args[++i];
      else if (a.Equals("--port", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out portField);
      else if (a.Equals("--com-port", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) comPortField = args[++i];
      else if (a.Equals("--interface", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) interfaceType = args[++i];
      else if (a.Equals("--runs", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out runs);
      else if (a.Equals("--delay", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out runDelayMs);
      else if (a.Equals("--connect-timeout-ms", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out connectTimeoutMs);
      else if (a.Equals("--verify-get-only", StringComparison.OrdinalIgnoreCase)) verifyAllSet = false;
      else if (a.Equals("--no-verify-get", StringComparison.OrdinalIgnoreCase)) verifyAllGet = false;
      else if (a.Equals("--max-properties", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out maxProperties);
      else if (a.Equals("--connect-retries", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out connectRetries);
      else if (a.Equals("--precheck-retries", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) int.TryParse(args[++i], out precheckRetries);
      else if (a.Equals("--verbose-verify", StringComparison.OrdinalIgnoreCase)) verboseVerify = true;
      else if (a.Equals("--trace", StringComparison.OrdinalIgnoreCase)) traceEnabled = true;
      else if (a.Equals("--verify-rates", StringComparison.OrdinalIgnoreCase)) verifyRates = true;
      else if (a.Equals("--help", StringComparison.OrdinalIgnoreCase))
      {
        PrintUsage();
        return;
      }
    }

    // These keys must match TelescopeHardware.cs.
    const string driverProgId = "ASCOM.TeenAstro.Telescope";
    const string traceKey = "Trace Level";
    const string comPortKey = "COM Port";
    const string ipKey = "IP Adress";
    const string portKey = "Port";
    const string interfaceKey = "Interface";

    try
    {
      using (var profile = new Profile())
      {
        profile.DeviceType = "Telescope";
        profile.WriteValue(driverProgId, traceKey, traceEnabled ? "true" : "false");
        profile.WriteValue(driverProgId, comPortKey, comPortField);
        profile.WriteValue(driverProgId, ipKey, ipField);
        profile.WriteValue(driverProgId, portKey, portField.ToString());
        profile.WriteValue(driverProgId, interfaceKey, interfaceType);
      }
    }
    catch (Exception ex)
    {
      Console.WriteLine($"[ERROR] Failed to write ASCOM profile: {ex}");
      return;
    }

    // Use the real ASCOM driver code path directly in-process.
    // We also initialise the local-server TraceLogger/ref-counting so the driver
    // can construct without COM hosting.
    Telescope telescope = null;
    try
    {
      // Preflight: exercise the same underlying transport used by SharedResources,
      // but without COM activation, to verify reachability in-process.
      try
      {
        var preUid = Guid.NewGuid();
        string transport = interfaceType?.Trim().Equals("COM", StringComparison.OrdinalIgnoreCase) == true ? "COM" : "IP";
        SharedResources.Connect(preUid, comPortField, ipField, (short)portField, transport);
        string preBuf = "";
        bool preOk = SharedResources.SendCommand(":GVN#", 2, ref preBuf, retries: precheckRetries);
        SharedResources.Disconnect(preUid);

        Console.WriteLine($"[PRECHECK] SharedResources {transport} :GVN# ok={preOk} reply='{preBuf}'");
      }
      catch (Exception ex)
      {
        Console.WriteLine($"[PRECHECK] SharedResources preflight failed: {ex.Message}");
      }

      InitLocalServerForInProcessTests();
      
      Exception lastConnectException = null;
      bool connectedOk = false;
      double raProbe = 0;

      for (int attempt = 1; attempt <= connectRetries; attempt++)
      {
        Console.WriteLine($"[CONNECT] Attempt {attempt}/{connectRetries}...");
        try
        {
          telescope = new Telescope();
          telescope.Connected = true;

          // Wait for the connection to become operational by probing a property.
          var sw = Stopwatch.StartNew();
          connectedOk = false;
          raProbe = 0;
          while (sw.ElapsedMilliseconds < connectTimeoutMs)
          {
            try
            {
              raProbe = telescope.RightAscension;
              connectedOk = true;
              break;
            }
            catch
            {
              Thread.Sleep(250);
            }
          }

          if (!connectedOk)
            throw new ASCOM.NotConnectedException($"Connect did not become operational within {connectTimeoutMs}ms.");

          Console.WriteLine($"[OK] Connected (RA probe={raProbe:F4}h). Testing link stability with {runs} polls...");
          break;
        }
        catch (Exception ex)
        {
          lastConnectException = ex;
          Console.WriteLine($"[WARN] Connect attempt {attempt} failed: {ex.GetType().Name}: {ex.Message}");
          try { telescope?.Dispose(); } catch { }
          telescope = null;
          Thread.Sleep(1000);
        }
      }

      if (!connectedOk)
      {
        Console.WriteLine($"[ERROR] Could not connect after {connectRetries} attempts.");
        if (lastConnectException != null)
          Console.WriteLine($"[ERROR] Last exception: {lastConnectException.GetType().Name}: {lastConnectException.Message}");
        return;
      }

      int okCount = 0;
      int failCount = 0;
      long raTotalMs = 0;
      long decTotalMs = 0;

      for (int run = 1; run <= runs; run++)
      {
        if (!telescope.Connected)
        {
          Console.WriteLine($"[FAIL] Run {run}: driver reported disconnected.");
          failCount++;
          break;
        }

        try
        {
          // These properties validate the TCP request/response loop.
          var t1 = Stopwatch.StartNew();
          double ra = telescope.RightAscension; // should return hours
          t1.Stop();

          var t2 = Stopwatch.StartNew();
          double dec = telescope.Declination;   // should return degrees
          t2.Stop();

          // Touch another state property.
          bool tracking = telescope.Tracking;

          okCount++;
          raTotalMs += t1.ElapsedMilliseconds;
          decTotalMs += t2.ElapsedMilliseconds;

          Console.WriteLine($"[OK] {run}/{runs} RA={ra:F4}h DEC={dec:F4}deg Tracking={tracking} (RA {t1.ElapsedMilliseconds}ms, DEC {t2.ElapsedMilliseconds}ms)");
        }
        catch (Exception ex)
        {
          failCount++;
          Console.WriteLine($"[FAIL] Run {run}/{runs}: {ex.Message}");
          Console.WriteLine(ex);
          // keep going a bit; user can lower runs if needed
        }

        Thread.Sleep(runDelayMs);
      }

      Console.WriteLine("");
      Console.WriteLine($"[SUMMARY] OK={okCount} FAIL={failCount}");
      if (okCount > 0)
      {
        Console.WriteLine($"[SUMMARY] Avg RA read={raTotalMs / okCount}ms, Avg DEC read={decTotalMs / okCount}ms");
      }

      if (verifyRates)
      {
        Console.WriteLine("");
        VerifyRateRoundTrip(telescope);
      }

      if (verifyAllGet || verifyAllSet)
      {
        Console.WriteLine("");
        Console.WriteLine("[VERIFY] Enumerating ITelescopeV4 get/set properties...");
        VerifyAllProperties(telescope, verifyAllGet, verifyAllSet, maxProperties, verboseVerify);
      }
    }
    finally
    {
      try
      {
        if (telescope != null)
        {
          telescope.Connected = false;
          telescope.Dispose();
        }
      }
      catch { }

      telescope = null;
      GC.Collect();
      GC.WaitForPendingFinalizers();
    }
  }

  /// <summary>
  /// Exercises RightAscensionRate / DeclinationRate through the real driver (hex :SXRr/:SXRd, :GXRr/:GXRd).
  /// Requires firmware with CMD_MAX_PAYLOAD>=27 and tracking on sidereal where applicable.
  /// </summary>
  private static void VerifyRateRoundTrip(Telescope telescope)
  {
    Console.WriteLine("[RATE] Hex wire rate round-trip (ASCOM <-> mount)...");
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
        Console.WriteLine("[RATE] SKIP RightAscensionRate (CanSetRightAscensionRate=false)");
      }
      else
      {
        double savedRa = telescope.RightAscensionRate;
        const double targetRa = -0.0033333333333333335;
        telescope.RightAscensionRate = targetRa;
        double readRa = telescope.RightAscensionRate;
        double diffRa = Math.Abs(readRa - targetRa);
        if (diffRa > 1e-9)
          Console.WriteLine($"[RATE] WARN RightAscensionRate read {readRa} vs set {targetRa} (diff {diffRa}; cache/GXAS may quantize)");
        else
          Console.WriteLine($"[RATE] OK  RightAscensionRate long precision (diff {diffRa})");
        telescope.RightAscensionRate = savedRa;
      }

      if (!telescope.CanSetDeclinationRate)
      {
        Console.WriteLine("[RATE] SKIP DeclinationRate (CanSetDeclinationRate=false)");
      }
      else
      {
        double savedDec = telescope.DeclinationRate;
        const double targetDec = 0.05;
        telescope.DeclinationRate = targetDec;
        double readDec = telescope.DeclinationRate;
        double diffDec = Math.Abs(readDec - targetDec);
        if (diffDec > 1e-9)
          Console.WriteLine($"[RATE] WARN DeclinationRate read {readDec} vs set {targetDec} (diff {diffDec})");
        else
          Console.WriteLine($"[RATE] OK  DeclinationRate (diff {diffDec})");
        telescope.DeclinationRate = savedDec;
      }

      Console.WriteLine("[RATE] Done.");
    }
    catch (Exception ex)
    {
      Console.WriteLine($"[RATE] FAIL: {UnwrapForLogging(ex)}");
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
        Console.WriteLine($"[RATE] WARN restore tracking: {UnwrapForLogging(ex)}");
      }
    }
  }

  private static void InitLocalServerForInProcessTests()
  {
    // Telescope (and other ASCOM drivers) derive from ReferenceCountedObjectBase,
    // which calls ASCOM.LocalServer.Server.IncrementObjectCount() in its constructor.
    // In a normal setup this is initialised by running ASCOM.TeenAstro.exe (LocalServer.Main).
    // For an in-process test we only need to ensure Server.TL is non-null and startedByCOM=false.
    try
    {
      var serverType = typeof(ASCOM.LocalServer.Server);
      var tlField = serverType.GetField("TL", BindingFlags.NonPublic | BindingFlags.Static);
      if (tlField != null)
      {
        var tl = new TraceLogger("", "TeenAstro.LocalServer") { Enabled = false };
        tlField.SetValue(null, tl);
      }

      var startedByComField = serverType.GetField("startedByCOM", BindingFlags.NonPublic | BindingFlags.Static);
      if (startedByComField != null)
        startedByComField.SetValue(null, false);
    }
    catch
    {
      // Best-effort; if something goes wrong the driver constructor may still fail.
    }
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
    // ASCOM driver sometimes uses enums for configuration.
    return false;
  }

  private static string UnwrapForLogging(Exception ex)
  {
    if (ex == null) return "";
    Exception cur = ex;
    // Reflection wraps exceptions frequently.
    while (cur is System.Reflection.TargetInvocationException && cur.InnerException != null)
      cur = cur.InnerException;
    while (cur is AggregateException && cur.InnerException != null)
      cur = cur.InnerException;
    return $"{cur.GetType().Name}: {cur.Message}";
  }

  private static void VerifyAllProperties(Telescope telescope, bool verifyGet, bool verifySet, int maxProperties, bool verboseVerify)
  {
    ITelescopeV4 iface = (ITelescopeV4)(object)telescope;
    Type ifaceType = typeof(ITelescopeV4);

    int getOk = 0;
    int getFail = 0;
    int setOk = 0;
    int setFail = 0;

    var failedGets = new List<string>();
    var failedSets = new List<string>();

    // Avoid side effects that are not "set-to-same-value" safe.
    var skipProps = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
    {
      "Connected",
      "Connecting"
    };

    PropertyInfo[] props = ifaceType.GetProperties();
    int processed = 0;

    foreach (var prop in props)
    {
      if (prop.GetIndexParameters().Length != 0) continue; // skip indexers
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
            Console.WriteLine($"[VERIFY] GET {prop.Name} ...");
            Console.Out.Flush();
          }
          currentValue = prop.GetValue(iface, null);
          hasCurrent = true;
          getOk++;
        }
        catch (Exception ex)
        {
          getFail++;
          failedGets.Add($"{prop.Name}: {UnwrapForLogging(ex)}");
          hasCurrent = false;
        }
      }

      if (verifySet && prop.CanWrite && prop.CanRead && hasCurrent)
      {
        // Only attempt safe primitive-ish/enum/string types.
        if (!IsSettablePrimitiveLike(prop.PropertyType))
          continue;

        if (prop.PropertyType == typeof(DateTime))
        {
          // Setting DateTime values can sometimes be treated specially by ASCOM.
          // We skip it unless you explicitly want that (not required for connection testing).
          continue;
        }

        try
        {
          if (verboseVerify)
          {
            Console.WriteLine($"[VERIFY] SET {prop.Name} ...");
            Console.Out.Flush();
          }
          // Set back to the same value to exercise the "set" code path
          // without changing behaviour/state.
          prop.SetValue(iface, currentValue, null);
          setOk++;
        }
        catch (Exception ex)
        {
          setFail++;
          failedSets.Add($"{prop.Name}: {UnwrapForLogging(ex)}");
        }
      }
    }

    Console.WriteLine($"[VERIFY SUMMARY] GET ok={getOk} fail={getFail}");
    Console.WriteLine($"[VERIFY SUMMARY] SET ok={setOk} fail={setFail}");

    if (failedGets.Count > 0)
    {
      Console.WriteLine("[VERIFY FAILED GETS]");
      foreach (var s in failedGets)
        Console.WriteLine("  " + s);
    }
    if (failedSets.Count > 0)
    {
      Console.WriteLine("[VERIFY FAILED SETS]");
      foreach (var s in failedSets)
        Console.WriteLine("  " + s);
    }
  }

  private static void PrintUsage()
  {
    Console.WriteLine("TeenAstroASCOM_V7_TcpConnectivityTest");
    Console.WriteLine("Args:");
    Console.WriteLine("  --ip <ip[:port]>            e.g. 192.168.1.17:9999");
    Console.WriteLine("  --port <port>              default 9999");
    Console.WriteLine("  --interface <COM|IP>       default IP");
    Console.WriteLine("  --com-port <COMx>          default COM1 (for COM interface)");
    Console.WriteLine("  --runs <n>                 default 30");
    Console.WriteLine("  --delay <ms>               default 250");
    Console.WriteLine("  --connect-timeout-ms <ms> default 15000");
    Console.WriteLine("  --verify-get-only         only verify GET (no SET)");
    Console.WriteLine("  --no-verify-get           disable GET verification");
    Console.WriteLine("  --max-properties <n>     stop after N properties (debug)");
    Console.WriteLine("  --connect-retries <n>    default 3");
    Console.WriteLine("  --precheck-retries <n>   default 3");
    Console.WriteLine("  --verbose-verify         log each property before calling get/set");
    Console.WriteLine("  --trace                    enable ASCOM Trace Level in profile");
    Console.WriteLine("  --verify-rates             after connect, test RA/DEC rate set/read (hex protocol)");
  }
}
