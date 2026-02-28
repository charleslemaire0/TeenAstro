using System;
using ASCOM.TeenAstro.Telescope;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace TeenAstroASCOM_V7.Tests
{
  [TestClass]
  public class RARateConversionTests
  {
    private const int PIER_EAST = 0;
    private const int PIER_WEST = 1;
    private const int GEM = 1;
    private const int FORK = 2;

    [TestMethod]
    public void AscomToMount_PassThrough_PierIndependent()
    {
      // Driver sends same value regardless of pier; firmware interprets for pier side.
      double resultE = RARateConversion.AscomToMount(0.0033, PIER_EAST, GEM);
      double resultW = RARateConversion.AscomToMount(0.0033, PIER_WEST, GEM);
      Assert.AreEqual(0.0033, resultE, 0.0001);
      Assert.AreEqual(0.0033, resultW, 0.0001);
    }

    [TestMethod]
    public void AscomToMount_NegativeRate()
    {
      double result = RARateConversion.AscomToMount(-0.0033, PIER_WEST, GEM);
      Assert.AreEqual(-0.0033, result, 0.0001);
    }

    [TestMethod]
    public void AscomToMount_LargeRate()
    {
      double result = RARateConversion.AscomToMount(2.6667, PIER_WEST, GEM);
      Assert.AreEqual(2.6667, result, 0.0001);
    }

    [TestMethod]
    public void MountToAscom_PassThrough_PierIndependent()
    {
      // TrackRateRA 33 → 0.0033 regardless of pier
      double resultE = RARateConversion.MountToAscom(33, PIER_EAST, GEM);
      double resultW = RARateConversion.MountToAscom(33, PIER_WEST, GEM);
      Assert.AreEqual(0.0033, resultE, 0.0001);
      Assert.AreEqual(0.0033, resultW, 0.0001);
    }

    [TestMethod]
    public void MountToAscom_Sidereal()
    {
      // TrackRateRA 0 = sidereal (HA=1)
      double result = RARateConversion.MountToAscom(0, PIER_EAST, GEM);
      Assert.AreEqual(0.0, result, 0.0001);
    }

    [TestMethod]
    public void RoundTrip_PierWest()
    {
      const double ascomRate = 0.0033;
      double toMount = RARateConversion.AscomToMount(ascomRate, PIER_WEST, GEM);
      int trackRateRA = (int)Math.Round(toMount * 10000);
      double backToAscom = RARateConversion.MountToAscom(trackRateRA, PIER_WEST, GEM);
      Assert.AreEqual(ascomRate, backToAscom, 0.0001);
    }

    [TestMethod]
    public void RoundTrip_PierEast()
    {
      const double ascomRate = -0.0033;
      double toMount = RARateConversion.AscomToMount(ascomRate, PIER_EAST, GEM);
      int trackRateRA = (int)Math.Round(toMount * 10000);
      double backToAscom = RARateConversion.MountToAscom(trackRateRA, PIER_EAST, GEM);
      Assert.AreEqual(ascomRate, backToAscom, 0.0001);
    }
  }
}
