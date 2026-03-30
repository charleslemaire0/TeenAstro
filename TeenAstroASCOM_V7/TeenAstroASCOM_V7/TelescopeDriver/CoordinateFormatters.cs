using System;
using System.Globalization;

namespace ASCOM.TeenAstro
{
  /// <summary>
  /// Pure-math coordinate formatting utilities used by the TeenAstro LX200 driver.
  /// Public so that unit tests can verify round-trip accuracy without triggering
  /// the TelescopeHardware static constructor.
  /// </summary>
  public static class CoordinateFormatters
  {
    /// <summary>
    /// Converts fractional degrees to integer degrees, minutes, seconds.
    /// Uses rounding: floor(deg*3600 + 0.5).
    /// </summary>
    public static void DegtoDMS(double degf, out int degi, out int mini, out int seci)
    {
      int tts = (int)Math.Round(Math.Floor(degf * 3600d + 0.5d));
      degi = tts / 3600;
      mini = (tts - degi * 3600) / 60;
      seci = tts % 60;
    }

    /// <summary>
    /// Formats degrees as DDD:MM:SS (3-digit degrees, zero-padded).
    /// Used for azimuth and longitude commands.
    /// </summary>
    public static string DegtoDDDMMSS(double value)
    {
      DegtoDMS(value, out int d, out int m, out int s);
      return d.ToString("000") + ":" + m.ToString("00") + ":" + s.ToString("00", CultureInfo.InvariantCulture);
    }

    /// <summary>
    /// Formats degrees as DD:MM:SS (2-digit degrees, zero-padded).
    /// Used for latitude and declination commands.
    /// </summary>
    public static string DegtoDDMMSS(double value)
    {
      DegtoDMS(value, out int d, out int m, out int s);
      return d.ToString("00") + ":" + m.ToString("00") + ":" + s.ToString("00", CultureInfo.InvariantCulture);
    }

    /// <summary>
    /// Builds the LX200 Sz command string for a target azimuth.
    /// </summary>
    public static string FormatSzCommand(double azDeg)
    {
      if (azDeg < 0.0 || azDeg > 360.0)
        throw new ArgumentOutOfRangeException(nameof(azDeg));
      return "Sz" + DegtoDDDMMSS(azDeg);
    }

    /// <summary>
    /// Builds the LX200 Sa command string for a target altitude.
    /// </summary>
    public static string FormatSaCommand(double altDeg)
    {
      if (altDeg < -30.0 || altDeg > 90.0)
        throw new ArgumentOutOfRangeException(nameof(altDeg));
      string sg = (altDeg >= 0) ? "+" : "-";
      return "Sa" + sg + DegtoDDMMSS(Math.Abs(altDeg));
    }

    /// <summary>
    /// Builds the LX200 St command string for site latitude.
    /// </summary>
    public static string FormatStCommand(double latDeg)
    {
      if (latDeg < -90.0 || latDeg > 90.0)
        throw new ArgumentOutOfRangeException(nameof(latDeg));
      string sg = (latDeg >= 0) ? "+" : "-";
      return "St" + sg + DegtoDDMMSS(Math.Abs(latDeg));
    }

    /// <summary>
    /// Builds the LX200 Sg command string for site longitude.
    /// Note: LX200 longitude sign convention is inverted (West positive) so
    /// the caller must negate ASCOM longitude before calling this.
    /// </summary>
    public static string FormatSgCommand(double lonDeg)
    {
      if (lonDeg < -180.0 || lonDeg > 180.0)
        throw new ArgumentOutOfRangeException(nameof(lonDeg));
      string sg = (lonDeg >= 0) ? "+" : "-";
      return "Sg" + sg + DegtoDDDMMSS(Math.Abs(lonDeg));
    }

    /// <summary>
    /// Parses a DD:MM:SS or DDD:MM:SS string back to fractional degrees.
    /// Handles optional leading +/- sign.
    /// </summary>
    public static double ParseSexagesimal(string sexa)
    {
      bool negative = sexa.StartsWith("-");
      if (sexa.StartsWith("+") || sexa.StartsWith("-"))
        sexa = sexa.Substring(1);

      string[] parts = sexa.Split(':');
      double d = double.Parse(parts[0], CultureInfo.InvariantCulture);
      double m = double.Parse(parts[1], CultureInfo.InvariantCulture);
      double s = double.Parse(parts[2], CultureInfo.InvariantCulture);
      double val = d + m / 60.0 + s / 3600.0;
      return negative ? -val : val;
    }
  }
}
