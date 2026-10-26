using ASCOM.TeenAstro.Telescope;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace TeenAstroASCOM_V7.Tests
{
  [TestClass]
  public class AlignActionTests
  {
    [TestMethod]
    public void AlignStart_RigidSix_SendsA0r6()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignStart", "r6", out AlignActions.Plan plan, out AlignActions.ParseFailure failure, out string error));
      Assert.AreEqual(AlignActions.ParseFailure.None, failure);
      Assert.IsNull(error);
      Assert.AreEqual(AlignActions.Kind.Start, plan.Kind);
      Assert.AreEqual("A0,r6", plan.Command);
      Assert.AreEqual("AlignStart r6", plan.Reply);
    }

    [TestMethod]
    public void AlignStart_ThreeStars_IsProgressiveSession()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignStart", " r3 ", out AlignActions.Plan plan, out _, out _));
      Assert.AreEqual("A0,r3", plan.Command);
    }

    [TestMethod]
    public void AlignStart_TwoAndMechanical_UseHomeCommands()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignStart", "2", out AlignActions.Plan two, out _, out _));
      Assert.AreEqual("A0,2", two.Command);
      Assert.IsTrue(AlignActions.TryParse("AlignStart", "m", out AlignActions.Plan mech, out _, out _));
      Assert.AreEqual("A0,m", mech.Command);
    }

    [TestMethod]
    public void AlignAtTarget_TwoStars_OmitsRejectedSuffix()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignAtTarget", "2", out AlignActions.Plan plan, out _, out _));
      Assert.AreEqual("A*", plan.Command);
      Assert.IsTrue(AlignActions.TryParse("AlignAtTarget", "r4", out AlignActions.Plan rigid, out _, out _));
      Assert.AreEqual("A*,r4", rigid.Command);
      Assert.IsTrue(AlignActions.TryParse("AlignAtTarget", "m", out AlignActions.Plan mech, out _, out _));
      Assert.AreEqual("A*,m", mech.Command);
    }

    [TestMethod]
    public void AlignStart_RejectsOutOfRange()
    {
      Assert.IsFalse(AlignActions.TryParse("AlignStart", "", out _, out AlignActions.ParseFailure empty, out _));
      Assert.AreEqual(AlignActions.ParseFailure.BadParameters, empty);
      Assert.IsFalse(AlignActions.TryParse("AlignStart", "r2", out _, out AlignActions.ParseFailure low, out _));
      Assert.AreEqual(AlignActions.ParseFailure.BadParameters, low);
      Assert.IsFalse(AlignActions.TryParse("AlignStart", "r10", out _, out AlignActions.ParseFailure high, out _));
      Assert.AreEqual(AlignActions.ParseFailure.BadParameters, high);
    }

    [TestMethod]
    public void AlignAccept_StarOnly_DoesNotSetTarget()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignAccept", "3", out AlignActions.Plan plan, out _, out _));
      Assert.AreEqual("A3", plan.Command);
      Assert.AreEqual("AlignAccept 3", plan.Reply);
      Assert.IsFalse(plan.SetTarget);
    }

    [TestMethod]
    public void AlignAccept_WithSolvedCoordinates_SetsTarget()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignAccept", "2, 5.25, -10.5", out AlignActions.Plan plan, out _, out _));
      Assert.AreEqual("A2", plan.Command);
      Assert.IsTrue(plan.SetTarget);
      Assert.AreEqual(5.25, plan.RaHours, 1e-9);
      Assert.AreEqual(-10.5, plan.DecDeg, 1e-9);
    }

    [TestMethod]
    public void AlignAccept_RejectsBadCoordinates()
    {
      Assert.IsFalse(AlignActions.TryParse("AlignAccept", "1,24.1,0", out _, out AlignActions.ParseFailure ra, out _));
      Assert.AreEqual(AlignActions.ParseFailure.BadParameters, ra);
      Assert.IsFalse(AlignActions.TryParse("AlignAccept", "1,1,90.1", out _, out _, out _));
      Assert.IsFalse(AlignActions.TryParse("AlignAccept", "0", out _, out _, out _));
      Assert.IsFalse(AlignActions.TryParse("AlignAccept", "1,NaN,0", out _, out _, out _));
    }

    [TestMethod]
    public void BareActions_RejectParameters()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignSave", "", out AlignActions.Plan save, out _, out _));
      Assert.AreEqual("AW", save.Command);
      Assert.IsTrue(AlignActions.TryParse("AlignAbort", null, out AlignActions.Plan abort, out _, out _));
      Assert.AreEqual("AB", abort.Command);
      Assert.IsTrue(AlignActions.TryParse("AlignPolarDone", "  ", out AlignActions.Plan polar, out _, out _));
      Assert.AreEqual("AP", polar.Command);
      Assert.IsTrue(AlignActions.TryParse("AutoAlign", "ignored", out AlignActions.Plan auto, out _, out _));
      Assert.AreEqual("AA", auto.Command);
      Assert.AreEqual("AutoAlign Started", auto.Reply);
      Assert.IsFalse(AlignActions.TryParse("AlignSave", "1", out _, out AlignActions.ParseFailure extra, out _));
      Assert.AreEqual(AlignActions.ParseFailure.BadParameters, extra);
    }

    [TestMethod]
    public void UnknownAction_IsReported()
    {
      Assert.IsFalse(AlignActions.TryParse("Nope", "", out _, out AlignActions.ParseFailure failure, out _));
      Assert.AreEqual(AlignActions.ParseFailure.UnknownAction, failure);
    }

    [TestMethod]
    public void AlignStatus_FormatsTheSharedText()
    {
      Assert.IsTrue(AlignActions.TryFormatStatus(4, 1.5, "-PI", 4, 0, out string text));
      Assert.AreEqual("stars=4;rms=1.500000;terms=-PI;pier=4,0", text);
      Assert.IsFalse(AlignActions.TryFormatStatus(0, 0, "XX-", 0, 0, out _));
    }

    [TestMethod]
    public void AlignErrors_TakesNoParameters()
    {
      Assert.IsTrue(AlignActions.TryParse("AlignErrors", "", out AlignActions.Plan plan, out _, out _));
      Assert.AreEqual(AlignActions.Kind.Errors, plan.Kind);
      Assert.IsFalse(AlignActions.TryParse("AlignErrors", "1", out _, out AlignActions.ParseFailure extra, out _));
      Assert.AreEqual(AlignActions.ParseFailure.BadParameters, extra);
    }

    [TestMethod]
    public void AlignErrors_FormatsTheSharedText()
    {
      double me = 12.0 / 60.0 + 34.0 / 3600.0;
      Assert.IsTrue(AlignActions.TryParseDms("+00*12:34", out double parsed));
      Assert.AreEqual(me, parsed, 1e-12);
      Assert.IsTrue(AlignActions.TryParseDms("-01*30:00", out double negative));
      Assert.AreEqual(-1.5, negative, 1e-12);
      Assert.IsFalse(AlignActions.TryParseDms("+00*60:00", out _));

      Assert.IsTrue(AlignActions.TryFormatErrors(1.5, -2.25, 0, me, -1.5, 12.5, 0.0166666667, out string text));
      Assert.AreEqual(
        "ch=1.500000;np=-2.250000;id=0.000000;me=0.209444;ma=-1.500000;rms=12.500000;ae=0.016667",
        text);
      Assert.IsFalse(AlignActions.TryFormatErrors(double.NaN, 0, 0, 0, 0, 0, 0, out _));
    }
  }
}
