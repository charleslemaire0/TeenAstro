using ASCOM.TeenAstro;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace TeenAstroASCOM_V7.Tests
{
  [TestClass]
  public class UpdateCheckTests
  {
    [TestMethod]
    public void ParseVersion_ReadsUpToFourParts()
    {
      CollectionAssert.AreEqual(new[] { 1, 6, 9, 0 }, UpdateCheck.ParseVersion("1.6.9.0"));
      CollectionAssert.AreEqual(new[] { 1, 6, 16, 0 }, UpdateCheck.ParseVersion("1.6.16"));
      Assert.IsNull(UpdateCheck.ParseVersion("nightly"));
    }

    [TestMethod]
    public void Sources_YieldDriverAndFirmware()
    {
      Assert.AreEqual("1.6.10.0", UpdateCheck.AssemblyVersionFromSource("[assembly: AssemblyVersion(\"1.6.10.0\")]"));
      Assert.AreEqual("1.6.16", UpdateCheck.FirmwareNumberFromHeader("#define FirmwareNumber  \"1.6.16\""));
    }

    [TestMethod]
    public void HighestReleaseLine_PicksTheNewestBranch()
    {
      const string json = "[{\"ref\":\"refs/heads/Release_1.5\"},{\"ref\":\"refs/heads/Release_1.7\"},{\"ref\":\"refs/heads/Release_1.6\"}]";
      Assert.AreEqual("1.7", UpdateCheck.HighestReleaseLine(json));
    }

    [TestMethod]
    public void Describe_NamesNewerDriverAndFirmware()
    {
      string text = UpdateCheck.Describe("1.6.9.0", "1.6.10.0", "1.6.16", "1.6.9", "1.6");
      StringAssert.Contains(text, "A newer driver is available: 1.6.10");
      StringAssert.Contains(text, "This copy is 1.6.9.");
      StringAssert.Contains(text, "Newer firmware is available: 1.6.16");
      StringAssert.Contains(text, "The mount reports 1.6.9.");
    }

    [TestMethod]
    public void Describe_SaysWhenBothMatch()
    {
      string text = UpdateCheck.Describe("1.6.9.0", "1.6.9.0", "1.6.16", "1.6.16", "1.6");
      StringAssert.Contains(text, "the same as the published copy");
      StringAssert.Contains(text, "the same as the mount");
    }

    [TestMethod]
    public void Describe_SkipsTheMountWhenDisconnected()
    {
      string text = UpdateCheck.Describe("1.6.9.0", "1.6.9.0", "1.6.16", null, "1.6");
      StringAssert.Contains(text, "The mount is not connected");
    }
  }
}
