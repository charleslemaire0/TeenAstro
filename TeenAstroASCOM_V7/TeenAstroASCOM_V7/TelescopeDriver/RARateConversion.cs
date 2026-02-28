using System;

namespace ASCOM.TeenAstro.Telescope
{
  /// <summary>
  /// Pure conversion helpers for RA tracking rate between ASCOM and mount protocol.
  /// Motor requests are pier-side independent; the firmware interprets HA rate correctly for any pier.
  /// </summary>
  public static class RARateConversion
  {
    /// <summary>ASCOM rate (RA sec/sidereal sec) → value to send in SXRr.</summary>
    public static double AscomToMount(double ascomRate, int pierSide, int mountType)
    {
      return Math.Round(ascomRate, 4);
    }

    /// <summary>TrackRateRA from GXAS (×10000) → ASCOM rate.</summary>
    public static double MountToAscom(int trackRateRA, int pierSide, int mountType)
    {
      return trackRateRA / 10000d;
    }
  }
}
