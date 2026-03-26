using System;
using System.Globalization;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using ASCOM.TeenAstro;

namespace TeenAstroASCOM_V7.Tests
{
  [TestClass]
  public class CoordinateFormatterTests
  {
    private const double OneArcsec = 1.0 / 3600.0;

    #region DegtoDMS

    [TestMethod]
    public void DegtoDMS_Zero()
    {
      CoordinateFormatters.DegtoDMS(0.0, out int d, out int m, out int s);
      Assert.AreEqual(0, d);
      Assert.AreEqual(0, m);
      Assert.AreEqual(0, s);
    }

    [TestMethod]
    public void DegtoDMS_WholeNumber()
    {
      CoordinateFormatters.DegtoDMS(45.0, out int d, out int m, out int s);
      Assert.AreEqual(45, d);
      Assert.AreEqual(0, m);
      Assert.AreEqual(0, s);
    }

    [TestMethod]
    public void DegtoDMS_HalfDegree()
    {
      CoordinateFormatters.DegtoDMS(45.5, out int d, out int m, out int s);
      Assert.AreEqual(45, d);
      Assert.AreEqual(30, m);
      Assert.AreEqual(0, s);
    }

    [TestMethod]
    public void DegtoDMS_WithSeconds()
    {
      double input = 45.0 + 30.0 / 60.0 + 30.0 / 3600.0;
      CoordinateFormatters.DegtoDMS(input, out int d, out int m, out int s);
      Assert.AreEqual(45, d);
      Assert.AreEqual(30, m);
      Assert.AreEqual(30, s);
    }

    [TestMethod]
    public void DegtoDMS_359_59_59()
    {
      double input = 359.0 + 59.0 / 60.0 + 59.0 / 3600.0;
      CoordinateFormatters.DegtoDMS(input, out int d, out int m, out int s);
      Assert.AreEqual(359, d);
      Assert.AreEqual(59, m);
      Assert.AreEqual(59, s);
    }

    [TestMethod]
    public void DegtoDMS_RoundsUpTo_NextDegree()
    {
      double input = 2.0 - 0.5 / 3600.0;
      CoordinateFormatters.DegtoDMS(input, out int d, out int m, out int s);
      Assert.AreEqual(2, d);
      Assert.AreEqual(0, m);
      Assert.AreEqual(0, s);
    }

    [TestMethod]
    public void DegtoDMS_90Degrees()
    {
      CoordinateFormatters.DegtoDMS(90.0, out int d, out int m, out int s);
      Assert.AreEqual(90, d);
      Assert.AreEqual(0, m);
      Assert.AreEqual(0, s);
    }

    [TestMethod]
    public void DegtoDMS_360Degrees()
    {
      CoordinateFormatters.DegtoDMS(360.0, out int d, out int m, out int s);
      Assert.AreEqual(360, d);
      Assert.AreEqual(0, m);
      Assert.AreEqual(0, s);
    }

    #endregion

    #region DegtoDDDMMSS

    [TestMethod]
    public void DegtoDDDMMSS_Zero()
    {
      Assert.AreEqual("000:00:00", CoordinateFormatters.DegtoDDDMMSS(0.0));
    }

    [TestMethod]
    public void DegtoDDDMMSS_180_30_00()
    {
      Assert.AreEqual("180:30:00", CoordinateFormatters.DegtoDDDMMSS(180.5));
    }

    [TestMethod]
    public void DegtoDDDMMSS_360()
    {
      Assert.AreEqual("360:00:00", CoordinateFormatters.DegtoDDDMMSS(360.0));
    }

    [TestMethod]
    public void DegtoDDDMMSS_SmallValue()
    {
      double val = 1.0 + 2.0 / 60.0 + 3.0 / 3600.0;
      Assert.AreEqual("001:02:03", CoordinateFormatters.DegtoDDDMMSS(val));
    }

    [TestMethod]
    public void DegtoDDDMMSS_249_30_00()
    {
      Assert.AreEqual("249:30:00", CoordinateFormatters.DegtoDDDMMSS(249.5));
    }

    [DataTestMethod]
    [DataRow(0.0)]
    [DataRow(1.0)]
    [DataRow(90.0)]
    [DataRow(180.0)]
    [DataRow(249.504722)]
    [DataRow(270.0)]
    [DataRow(359.999722)]
    [DataRow(360.0)]
    public void DegtoDDDMMSS_RoundTrip_SubArcsec(double input)
    {
      string formatted = CoordinateFormatters.DegtoDDDMMSS(input);
      double parsed = CoordinateFormatters.ParseSexagesimal(formatted);
      Assert.AreEqual(input, parsed, OneArcsec,
        $"Input={input:F6} Formatted={formatted} Parsed={parsed:F6} Error={Math.Abs(input - parsed) * 3600:F2}\"");
    }

    #endregion

    #region DegtoDDMMSS

    [TestMethod]
    public void DegtoDDMMSS_Zero()
    {
      Assert.AreEqual("00:00:00", CoordinateFormatters.DegtoDDMMSS(0.0));
    }

    [TestMethod]
    public void DegtoDDMMSS_45_30_30()
    {
      double input = 45.0 + 30.0 / 60.0 + 30.0 / 3600.0;
      Assert.AreEqual("45:30:30", CoordinateFormatters.DegtoDDMMSS(input));
    }

    [TestMethod]
    public void DegtoDDMMSS_90()
    {
      Assert.AreEqual("90:00:00", CoordinateFormatters.DegtoDDMMSS(90.0));
    }

    [TestMethod]
    public void DegtoDDMMSS_SiteLatitude_Paris()
    {
      double lat = 48.0 + 51.0 / 60.0 + 24.0 / 3600.0;
      Assert.AreEqual("48:51:24", CoordinateFormatters.DegtoDDMMSS(lat));
    }

    [DataTestMethod]
    [DataRow(0.0)]
    [DataRow(1.0)]
    [DataRow(10.5)]
    [DataRow(45.0)]
    [DataRow(48.8566)]
    [DataRow(89.999722)]
    [DataRow(90.0)]
    public void DegtoDDMMSS_RoundTrip_SubArcsec(double input)
    {
      string formatted = CoordinateFormatters.DegtoDDMMSS(input);
      double parsed = CoordinateFormatters.ParseSexagesimal(formatted);
      Assert.AreEqual(input, parsed, OneArcsec,
        $"Input={input:F6} Formatted={formatted} Parsed={parsed:F6} Error={Math.Abs(input - parsed) * 3600:F2}\"");
    }

    #endregion

    #region Sz (Azimuth) command format

    [TestMethod]
    public void FormatSzCommand_Zero()
    {
      Assert.AreEqual("Sz000:00:00", CoordinateFormatters.FormatSzCommand(0.0));
    }

    [TestMethod]
    public void FormatSzCommand_180()
    {
      Assert.AreEqual("Sz180:00:00", CoordinateFormatters.FormatSzCommand(180.0));
    }

    [TestMethod]
    public void FormatSzCommand_360()
    {
      Assert.AreEqual("Sz360:00:00", CoordinateFormatters.FormatSzCommand(360.0));
    }

    [TestMethod]
    public void FormatSzCommand_ConformValue()
    {
      double az = 249.0 + 30.0 / 60.0 + 17.0 / 3600.0;
      Assert.AreEqual("Sz249:30:17", CoordinateFormatters.FormatSzCommand(az));
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatSzCommand_Negative_Throws()
    {
      CoordinateFormatters.FormatSzCommand(-0.1);
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatSzCommand_Over360_Throws()
    {
      CoordinateFormatters.FormatSzCommand(360.1);
    }

    #endregion

    #region Sa (Altitude) command format

    [TestMethod]
    public void FormatSaCommand_Zero()
    {
      Assert.AreEqual("Sa+00:00:00", CoordinateFormatters.FormatSaCommand(0.0));
    }

    [TestMethod]
    public void FormatSaCommand_45_30()
    {
      Assert.AreEqual("Sa+45:30:00", CoordinateFormatters.FormatSaCommand(45.5));
    }

    [TestMethod]
    public void FormatSaCommand_Plus90()
    {
      Assert.AreEqual("Sa+90:00:00", CoordinateFormatters.FormatSaCommand(90.0));
    }

    [TestMethod]
    public void FormatSaCommand_Minus15()
    {
      Assert.AreEqual("Sa-15:00:00", CoordinateFormatters.FormatSaCommand(-15.0));
    }

    [TestMethod]
    public void FormatSaCommand_Minus30()
    {
      Assert.AreEqual("Sa-30:00:00", CoordinateFormatters.FormatSaCommand(-30.0));
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatSaCommand_UnderMinus30_Throws()
    {
      CoordinateFormatters.FormatSaCommand(-30.1);
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatSaCommand_Over90_Throws()
    {
      CoordinateFormatters.FormatSaCommand(90.1);
    }

    #endregion

    #region St (Site Latitude) command format

    [TestMethod]
    public void FormatStCommand_Paris_North()
    {
      double lat = 48.0 + 51.0 / 60.0 + 24.0 / 3600.0;
      Assert.AreEqual("St+48:51:24", CoordinateFormatters.FormatStCommand(lat));
    }

    [TestMethod]
    public void FormatStCommand_Sydney_South()
    {
      double lat = -(33.0 + 52.0 / 60.0 + 8.0 / 3600.0);
      Assert.AreEqual("St-33:52:08", CoordinateFormatters.FormatStCommand(lat));
    }

    [TestMethod]
    public void FormatStCommand_Equator()
    {
      Assert.AreEqual("St+00:00:00", CoordinateFormatters.FormatStCommand(0.0));
    }

    [TestMethod]
    public void FormatStCommand_NorthPole()
    {
      Assert.AreEqual("St+90:00:00", CoordinateFormatters.FormatStCommand(90.0));
    }

    [TestMethod]
    public void FormatStCommand_SouthPole()
    {
      Assert.AreEqual("St-90:00:00", CoordinateFormatters.FormatStCommand(-90.0));
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatStCommand_Over90_Throws()
    {
      CoordinateFormatters.FormatStCommand(90.1);
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatStCommand_UnderMinus90_Throws()
    {
      CoordinateFormatters.FormatStCommand(-90.1);
    }

    #endregion

    #region Sg (Site Longitude) command format

    [TestMethod]
    public void FormatSgCommand_Paris_East()
    {
      double lg = 2.0 + 21.0 / 60.0 + 8.0 / 3600.0;
      Assert.AreEqual("Sg+002:21:08", CoordinateFormatters.FormatSgCommand(lg));
    }

    [TestMethod]
    public void FormatSgCommand_NewYork_West()
    {
      double lg = -(73.0 + 59.0 / 60.0 + 9.0 / 3600.0);
      Assert.AreEqual("Sg-073:59:09", CoordinateFormatters.FormatSgCommand(lg));
    }

    [TestMethod]
    public void FormatSgCommand_Greenwich()
    {
      Assert.AreEqual("Sg+000:00:00", CoordinateFormatters.FormatSgCommand(0.0));
    }

    [TestMethod]
    public void FormatSgCommand_DateLine_Plus180()
    {
      Assert.AreEqual("Sg+180:00:00", CoordinateFormatters.FormatSgCommand(180.0));
    }

    [TestMethod]
    public void FormatSgCommand_DateLine_Minus180()
    {
      Assert.AreEqual("Sg-180:00:00", CoordinateFormatters.FormatSgCommand(-180.0));
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatSgCommand_Over180_Throws()
    {
      CoordinateFormatters.FormatSgCommand(180.1);
    }

    [TestMethod]
    [ExpectedException(typeof(ArgumentOutOfRangeException))]
    public void FormatSgCommand_UnderMinus180_Throws()
    {
      CoordinateFormatters.FormatSgCommand(-180.1);
    }

    #endregion

    #region ParseSexagesimal round-trip

    [TestMethod]
    public void ParseSexagesimal_Positive()
    {
      Assert.AreEqual(45.5, CoordinateFormatters.ParseSexagesimal("+45:30:00"), OneArcsec);
    }

    [TestMethod]
    public void ParseSexagesimal_Negative()
    {
      Assert.AreEqual(-33.5, CoordinateFormatters.ParseSexagesimal("-33:30:00"), OneArcsec);
    }

    [TestMethod]
    public void ParseSexagesimal_NoSign()
    {
      Assert.AreEqual(180.0, CoordinateFormatters.ParseSexagesimal("180:00:00"), OneArcsec);
    }

    [TestMethod]
    public void ParseSexagesimal_ThreeDigitDeg()
    {
      double expected = 249.0 + 30.0 / 60.0 + 17.0 / 3600.0;
      Assert.AreEqual(expected, CoordinateFormatters.ParseSexagesimal("249:30:17"), OneArcsec);
    }

    #endregion

    #region Worst-case quantization error (statistical)

    [TestMethod]
    public void QuantizationError_IsAtMost_HalfArcsec()
    {
      double maxError = 0;
      var rng = new Random(42);
      for (int i = 0; i < 10000; i++)
      {
        double input = rng.NextDouble() * 360.0;
        string formatted = CoordinateFormatters.DegtoDDDMMSS(input);
        double parsed = CoordinateFormatters.ParseSexagesimal(formatted);
        double error = Math.Abs(input - parsed) * 3600.0;
        if (error > maxError) maxError = error;
      }
      Assert.IsTrue(maxError <= 0.51,
        $"Max quantization error across 10000 random values: {maxError:F3} arcsec (expected <= 0.5)");
    }

    [TestMethod]
    public void QuantizationError_DDMMSS_AtMost_HalfArcsec()
    {
      double maxError = 0;
      var rng = new Random(42);
      for (int i = 0; i < 10000; i++)
      {
        double input = rng.NextDouble() * 90.0;
        string formatted = CoordinateFormatters.DegtoDDMMSS(input);
        double parsed = CoordinateFormatters.ParseSexagesimal(formatted);
        double error = Math.Abs(input - parsed) * 3600.0;
        if (error > maxError) maxError = error;
      }
      Assert.IsTrue(maxError <= 0.51,
        $"Max quantization error across 10000 random DDMMSS values: {maxError:F3} arcsec (expected <= 0.5)");
    }

    #endregion

    #region Verify TelescopeHardware.SiteLatitude command matches CoordinateFormatters

    [TestMethod]
    public void SiteLatitude_DriverLogic_MatchesFormatter()
    {
      double lat = 48.8566;
      string sg = (lat >= 0) ? "+" : "-";
      string driverCmd = "St" + sg + CoordinateFormatters.DegtoDDMMSS(Math.Abs(lat));
      string formatterCmd = CoordinateFormatters.FormatStCommand(lat);
      Assert.AreEqual(formatterCmd, driverCmd,
        "Driver site-latitude command assembly differs from CoordinateFormatters");
    }

    [TestMethod]
    public void SiteLongitude_DriverLogic_MatchesFormatter()
    {
      double lg = 2.3522;
      string sg = (lg >= 0) ? "+" : "-";
      string driverCmd = "Sg" + sg + CoordinateFormatters.DegtoDDDMMSS(Math.Abs(lg));
      string formatterCmd = CoordinateFormatters.FormatSgCommand(lg);
      Assert.AreEqual(formatterCmd, driverCmd,
        "Driver site-longitude command assembly differs from CoordinateFormatters");
    }

    #endregion

    #region Edge cases matching Conform failures

    [TestMethod]
    public void Az_ConformOffset_102Arcsec_DiagnosedByRoundTrip()
    {
      // Conform reported 102.8 arcsec Az offset after AltAz slew.
      // This test checks that the formatter itself is NOT the cause.
      double targetAz = 249.504722; // typical Conform target
      string szPayload = CoordinateFormatters.DegtoDDDMMSS(targetAz);
      double readback = CoordinateFormatters.ParseSexagesimal(szPayload);
      double errorArcsec = Math.Abs(targetAz - readback) * 3600.0;
      Assert.IsTrue(errorArcsec < 1.0,
        $"Formatter round-trip error {errorArcsec:F2}\" is too large to explain the 102.8\" Conform offset. " +
        $"Formatter: {szPayload}, Parsed: {readback:F6}");
    }

    [TestMethod]
    public void Alt_ConformOffset_DiagnosedByRoundTrip()
    {
      double targetAlt = 50.0 + 25.0 / 60.0 + 13.0 / 3600.0;
      string saPayload = CoordinateFormatters.DegtoDDMMSS(targetAlt);
      double readback = CoordinateFormatters.ParseSexagesimal(saPayload);
      double errorArcsec = Math.Abs(targetAlt - readback) * 3600.0;
      Assert.IsTrue(errorArcsec < 1.0,
        $"Formatter round-trip error {errorArcsec:F2}\" for Alt. " +
        $"Formatter: {saPayload}, Parsed: {readback:F6}");
    }

    #endregion
  }

  [TestClass]
  [TestCategory("Integration")]
  public class GotoIntegrationTests
  {
    [TestMethod]
    [Ignore("Requires connected mount")]
    public void EQ_Goto_KnownStar_PositionWithinOneArcmin()
    {
      // 1. Connect, set tracking ON
      // 2. SlewToCoordinatesAsync to Polaris (RA≈2h31m49s, Dec≈+89:15:51)
      // 3. Wait until Slewing == false
      // 4. Read back RA/Dec
      // 5. Verify error < 1 arcmin per axis
      Assert.Inconclusive("Manual - requires mount connection");
    }

    [TestMethod]
    [Ignore("Requires connected mount")]
    public void AltAz_Goto_KnownPosition_TrackingOff()
    {
      // 1. Connect, set tracking OFF
      // 2. SlewToAltAz(180.0, 45.0)
      // 3. Read back Az/Alt
      // 4. Verify error < 1 arcmin per axis
      Assert.Inconclusive("Manual - requires mount connection");
    }

    [TestMethod]
    [Ignore("Requires connected mount")]
    public void SiteCoords_WriteRead_AtPark()
    {
      // 1. Park mount
      // 2. Write SiteLatitude = 48.8566, read back, compare < 1 arcsec
      // 3. Write SiteLongitude = 2.3522, read back, compare < 1 arcsec
      Assert.Inconclusive("Manual - requires mount connection");
    }

    [TestMethod]
    [Ignore("Requires connected mount")]
    public void Raw_LX200_SzSa_GzGa_Roundtrip()
    {
      // 1. Send Sz249:30:17, read GZ, parse, compare < 1 arcsec
      // 2. Send Sa+45:30:00, read GA, parse, compare < 1 arcsec
      Assert.Inconclusive("Manual - requires mount connection");
    }

    [TestMethod]
    [Ignore("Requires connected mount")]
    public void EQ_Goto_MultipleTargets_NoCumulativeError()
    {
      // Slew to 5 EQ targets sequentially, verify error stays < 1 arcmin
      Assert.Inconclusive("Manual - requires mount connection");
    }

    [TestMethod]
    [Ignore("Requires connected mount")]
    public void AltAz_Goto_MultipleTargets_TrackingOff()
    {
      // Slew to 5 AltAz targets sequentially (tracking off), verify < 1 arcmin
      Assert.Inconclusive("Manual - requires mount connection");
    }
  }
}
