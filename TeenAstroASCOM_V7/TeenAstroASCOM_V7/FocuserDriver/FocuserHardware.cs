//
// ASCOM Focuser hardware class for TeenAstro
// Uses the same shared connection (COM or IP) as the Telescope driver.
//

using ASCOM.LocalServer;
using ASCOM.Utilities;
using System;
using System.Globalization;
using System.Windows.Forms;

namespace ASCOM.TeenAstro.Focuser
{
  [HardwareClass()]
  internal static class FocuserHardware
  {
    internal const string traceStateProfileName = "Trace Level";
    internal const string traceStateDefault = "true";
    internal const string comPortProfileName = "COM Port";
    internal const string comPortDefault = "COM1";
    internal const string IPProfileName = "IP Adress";
    internal const string IPDefault = "192.168.0.1";
    internal const string PortProfileName = "Port";
    internal const string PortDefault = "9999";
    internal const string InterfaceProfileName = "Interface";
    internal const string InterfaceDefault = "COM";

    /// <summary>Shared connection profile: both Telescope and Focuser use this ProgId for COM/IP settings.</summary>
    private const string ConnectionProfileProgId = "ASCOM.TeenAstro.Telescope";

    private static string driverProgId = "";
    private static string driverDescription = "";
    private static bool runOnce = false;
    internal static string comPort;
    internal static string IP;
    internal static short Port;
    internal static string Interface;
    internal static TraceLogger tl;

    /// <summary>Default max position (TeenAstro focuser step range).</summary>
    internal const int DefaultMaxStep = 65535;

    /// <summary>Position tolerance (steps) to consider a move complete for IsMoving.</summary>
    private const int MoveToleranceSteps = 2;

    /// <summary>Target position for current move; -1 when not moving.</summary>
    private static int moveTargetPosition = -1;

    static FocuserHardware()
    {
      try
      {
        tl = new TraceLogger("", "TeenAstro.Focuser.Hardware");
        driverProgId = Focuser.DriverProgId;
        ReadProfile();
        LogMessage("FocuserHardware", "Static initialiser completed.");
      }
      catch (Exception ex)
      {
        try { LogMessage("FocuserHardware", "Initialisation exception: " + ex); } catch { }
        MessageBox.Show("FocuserHardware - " + ex.Message, "Exception creating Focuser", MessageBoxButtons.OK, MessageBoxIcon.Error);
        throw;
      }
    }

    internal static void InitialiseHardware()
    {
      LogMessage("InitialiseHardware", "Start.");
      if (runOnce) return;
      runOnce = true;
      driverDescription = Focuser.DriverDescription;
      LogMessage("InitialiseHardware", "One-off initialisation complete.");
    }

    internal static void ReadProfile()
    {
      using (var profile = new Profile())
      {
        profile.DeviceType = "Focuser";
        tl.Enabled = Convert.ToBoolean(profile.GetValue(driverProgId, traceStateProfileName, string.Empty, traceStateDefault));
      }
      using (var profile = new Profile())
      {
        profile.DeviceType = "Telescope";
        comPort = profile.GetValue(ConnectionProfileProgId, comPortProfileName, string.Empty, comPortDefault);
        IP = profile.GetValue(ConnectionProfileProgId, IPProfileName, string.Empty, IPDefault);
        Port = Convert.ToInt16(profile.GetValue(ConnectionProfileProgId, PortProfileName, string.Empty, PortDefault));
        Interface = profile.GetValue(ConnectionProfileProgId, InterfaceProfileName, string.Empty, InterfaceDefault);
      }
    }

    internal static void WriteProfile()
    {
      using (var profile = new Profile())
      {
        profile.DeviceType = "Focuser";
        profile.WriteValue(driverProgId, traceStateProfileName, tl.Enabled.ToString());
      }
      using (var profile = new Profile())
      {
        profile.DeviceType = "Telescope";
        profile.WriteValue(ConnectionProfileProgId, comPortProfileName, comPort);
        profile.WriteValue(ConnectionProfileProgId, IPProfileName, IP);
        profile.WriteValue(ConnectionProfileProgId, PortProfileName, Port.ToString());
        profile.WriteValue(ConnectionProfileProgId, InterfaceProfileName, Interface);
      }
    }

    public static void SetConnected(Guid uniqueId, bool newState)
    {
      if (newState)
      {
        if (SharedResources.IsInstanceConnected(uniqueId)) return;
        SharedResources.Connect(uniqueId, comPort, IP, Port, Interface);
        LogMessage("SetConnected", "Focuser connected (shared connection).");
      }
      else
      {
        if (!SharedResources.IsInstanceConnected(uniqueId)) return;
        SharedResources.Disconnect(uniqueId);
        LogMessage("SetConnected", "Focuser disconnected.");
      }
    }

    public static bool IsConnected => SharedResources.IsConnected;

    /// <summary>Send a focuser command and optionally read response. Command must include : and # (e.g. ":F?#").</summary>
    private static bool SendFocuserCommand(string command, int mode, ref string buf, int retries = 3)
    {
      return SharedResources.SendCommand(command, mode, ref buf, retries);
    }

    /// <summary>Get current position from :F?#. Reply format: ?NNNNN NNN sDD.DD# or 0 if no focuser. Returns true if focuser present.</summary>
    public static bool GetFocuserPosition(out int position, out int speed, out double temperature)
    {
      position = 0;
      speed = 0;
      temperature = double.NaN;
      string buf = "";
      if (!SendFocuserCommand(":F?#", 2, ref buf))
      {
        LogMessage("GetFocuserPosition", "Send failed.");
        return false;
      }
      buf = (buf ?? "").Trim();
      if (buf == "0" || string.IsNullOrEmpty(buf))
      {
        LogMessage("GetFocuserPosition", "No focuser (reply 0).");
        return false;
      }
      if (buf.StartsWith("?")) buf = buf.Substring(1);
      string[] parts = buf.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
      if (parts.Length < 1 || !int.TryParse(parts[0], NumberStyles.Integer, CultureInfo.InvariantCulture, out position))
      {
        LogMessage("GetFocuserPosition", "Parse position failed: " + buf);
        return false;
      }
      if (parts.Length >= 2) int.TryParse(parts[1], NumberStyles.Integer, CultureInfo.InvariantCulture, out speed);
      if (parts.Length >= 3)
      {
        string t = parts[2];
        if (t.StartsWith("s")) t = "-" + t.Substring(1);
        double.TryParse(t, NumberStyles.Any, CultureInfo.InvariantCulture, out temperature);
      }
      return true;
    }

    /// <summary>Move to absolute position. Command :FG,nnnnn# (no reply).</summary>
    public static void MoveTo(int position)
    {
      string cmd = ":FG," + position.ToString("D5", CultureInfo.InvariantCulture) + "#";
      string buf = "";
      if (!SendFocuserCommand(cmd, 0, ref buf))
        throw new ASCOM.DriverException("Focuser move command failed.");
      moveTargetPosition = position;
      LogMessage("MoveTo", cmd);
    }

    /// <summary>Halt focuser. Command :FQ#.</summary>
    public static void Halt()
    {
      moveTargetPosition = -1;
      string buf = "";
      SendFocuserCommand(":FQ#", 0, ref buf);
      LogMessage("Halt", "Sent :FQ#");
    }

    /// <summary>True if a move is in progress. Polls position and clears move state when within tolerance of target.</summary>
    public static bool GetIsMoving()
    {
      if (moveTargetPosition < 0) return false;
      if (!GetFocuserPosition(out int pos, out _, out _)) return false;
      if (Math.Abs(pos - moveTargetPosition) <= MoveToleranceSteps)
      {
        moveTargetPosition = -1;
        return false;
      }
      return true;
    }

    #region EEPROM Configuration

    internal class FocuserEepromSettings
    {
      public ushort ParkPos;
      public ushort MaxPos;
      public ushort MinSpeed;
      public ushort MaxSpeed;
      public ushort CmdAcc;
      public ushort ManAcc;
      public bool Reverse;
      public ushort Micro;
      public ushort Resolution;
      public ushort Current;
    }

    /// <summary>Read focuser settings via :F~#. Response: ~parkPos maxPos minSpeed maxSpeed cmdAcc manAcc</summary>
    public static FocuserEepromSettings ReadEepromSettings()
    {
      var settings = new FocuserEepromSettings();
      string buf = "";
      for (int attempt = 0; attempt < 10; attempt++)
      {
        if (!SendFocuserCommand(":F~#", 2, ref buf)) continue;
        buf = (buf ?? "").Trim();
        if (!buf.StartsWith("~")) continue;
        buf = buf.Substring(1);
        string[] parts = buf.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
        if (parts.Length < 6) continue;
        try
        {
          settings.ParkPos = ushort.Parse(parts[0], CultureInfo.InvariantCulture);
          settings.MaxPos = ushort.Parse(parts[1], CultureInfo.InvariantCulture);
          settings.MinSpeed = ushort.Parse(parts[2], CultureInfo.InvariantCulture);
          settings.MaxSpeed = ushort.Parse(parts[3], CultureInfo.InvariantCulture);
          settings.CmdAcc = ushort.Parse(parts[4], CultureInfo.InvariantCulture);
          settings.ManAcc = ushort.Parse(parts[5], CultureInfo.InvariantCulture);
          LogMessage("ReadEepromSettings", "OK: " + buf);
          return settings;
        }
        catch { continue; }
      }
      throw new ASCOM.DriverException("Failed to read focuser EEPROM settings.");
    }

    /// <summary>Read motor settings via :FM#. Response: Mreverse micro resolution current</summary>
    public static void ReadEepromMotor(FocuserEepromSettings settings)
    {
      string buf = "";
      for (int attempt = 0; attempt < 10; attempt++)
      {
        if (!SendFocuserCommand(":FM#", 2, ref buf)) continue;
        buf = (buf ?? "").Trim();
        if (!buf.StartsWith("M")) continue;
        buf = buf.Substring(1);
        string[] parts = buf.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
        if (parts.Length < 4) continue;
        try
        {
          settings.Reverse = parts[0] != "0";
          settings.Micro = ushort.Parse(parts[1], CultureInfo.InvariantCulture);
          settings.Resolution = ushort.Parse(parts[2], CultureInfo.InvariantCulture);
          settings.Current = ushort.Parse(parts[3], CultureInfo.InvariantCulture);
          LogMessage("ReadEepromMotor", "OK: " + buf);
          return;
        }
        catch { continue; }
      }
      throw new ASCOM.DriverException("Failed to read focuser motor EEPROM settings.");
    }

    /// <summary>Write a single EEPROM value. Index is "0"-"8" for settings, "c" for current, "m" for microstep.</summary>
    public static bool WriteEepromValue(string index, ushort value)
    {
      string cmd = ":F" + index + "," + value.ToString(CultureInfo.InvariantCulture) + "#";
      string buf = "";
      bool ok = SendFocuserCommand(cmd, 1, ref buf);
      LogMessage("WriteEepromValue", cmd + " -> " + (ok ? buf : "FAIL"));
      return ok && buf == "1";
    }

    #endregion

    internal static void LogMessage(string identifier, string message)
    {
      tl?.LogMessageCrLf(identifier, message);
    }
  }
}
