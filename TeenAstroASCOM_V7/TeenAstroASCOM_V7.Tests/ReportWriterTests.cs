using System;
using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using TeenAstroASCOM_V7.DriverVerification;

namespace TeenAstroASCOM_V7.Tests
{
  [TestClass]
  public class ReportWriterTests
  {
    [TestMethod]
    public void VerificationReport_Counts_Aggregate_Outcomes()
    {
      var r = new VerificationReport();
      r.Checks.Add(new CheckResult("A", "x", CheckOutcome.Ok));
      r.Checks.Add(new CheckResult("A", "y", CheckOutcome.Issue, "bad"));
      r.Checks.Add(new CheckResult("B", "z", CheckOutcome.Skipped));
      r.Checks.Add(new CheckResult("B", "w", CheckOutcome.Info, "note"));

      Assert.AreEqual(1, r.OkCount);
      Assert.AreEqual(1, r.IssueCount);
      Assert.AreEqual(1, r.SkippedCount);
      Assert.AreEqual(1, r.InfoCount);
      Assert.IsTrue(r.HasIssues);
    }

    [TestMethod]
    public void WriteJson_RoundTrip_ContainsSummaryAndChecks()
    {
      var r = new VerificationReport();
      r.StartedUtc = new DateTime(2026, 3, 25, 12, 0, 0, DateTimeKind.Utc);
      r.FinishedUtc = new DateTime(2026, 3, 25, 12, 0, 5, DateTimeKind.Utc);
      r.Checks.Add(new CheckResult("Profile", "Write", CheckOutcome.Ok));

      string path = Path.Combine(Path.GetTempPath(), "ta-dv-test-" + Guid.NewGuid().ToString("n") + ".json");
      try
      {
        ReportWriter.WriteJson(path, r);
        string text = File.ReadAllText(path);
        StringAssert.Contains(text, "\"ok\":1");
        StringAssert.Contains(text, "\"issue\":0");
        StringAssert.Contains(text, "Profile");
        StringAssert.Contains(text, "Write");
      }
      finally
      {
        try { File.Delete(path); } catch { }
      }
    }

    [TestMethod]
    public void WriteJunit_ContainsFailure_WhenIssue()
    {
      var r = new VerificationReport();
      r.Checks.Add(new CheckResult("X", "bad", CheckOutcome.Issue, "msg"));

      string path = Path.Combine(Path.GetTempPath(), "ta-dv-junit-" + Guid.NewGuid().ToString("n") + ".xml");
      try
      {
        ReportWriter.WriteJunit(path, r);
        string text = File.ReadAllText(path);
        StringAssert.Contains(text, "failures=\"1\"");
        StringAssert.Contains(text, "<failure");
        StringAssert.Contains(text, "msg");
      }
      finally
      {
        try { File.Delete(path); } catch { }
      }
    }

    [TestMethod]
    public void WriteJunit_EscapesXml_InMessage()
    {
      var r = new VerificationReport();
      r.Checks.Add(new CheckResult("X", "bad", CheckOutcome.Issue, "a<b&c"));

      string path = Path.Combine(Path.GetTempPath(), "ta-dv-junit2-" + Guid.NewGuid().ToString("n") + ".xml");
      try
      {
        ReportWriter.WriteJunit(path, r);
        string text = File.ReadAllText(path);
        Assert.IsFalse(text.Contains("a<b"), "raw angle brackets should be escaped");
        StringAssert.Contains(text, "&lt;");
      }
      finally
      {
        try { File.Delete(path); } catch { }
      }
    }
  }
}
