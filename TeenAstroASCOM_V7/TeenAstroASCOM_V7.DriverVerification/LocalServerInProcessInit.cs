using System.Reflection;
using ASCOM.Utilities;

namespace TeenAstroASCOM_V7.DriverVerification
{
  /// <summary>
  /// Telescope (and other ASCOM drivers) derive from ReferenceCountedObjectBase,
  /// which calls ASCOM.LocalServer.Server.IncrementObjectCount() in its constructor.
  /// For in-process tests we only need to ensure Server.TL is non-null and startedByCOM=false.
  /// </summary>
  public static class LocalServerInProcessInit
  {
    public static void Apply()
    {
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
  }
}
