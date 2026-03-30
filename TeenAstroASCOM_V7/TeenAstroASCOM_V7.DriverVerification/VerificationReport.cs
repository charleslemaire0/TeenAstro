using System;
using System.Collections.Generic;
using System.Linq;

namespace TeenAstroASCOM_V7.DriverVerification
{
  public sealed class VerificationReport
  {
    public VerificationReport()
    {
      Checks = new List<CheckResult>();
      StartedUtc = DateTime.UtcNow;
    }

    public DateTime StartedUtc { get; set; }
    public DateTime? FinishedUtc { get; set; }
    public List<CheckResult> Checks { get; }

    public int OkCount => Checks.Count(c => c.Outcome == CheckOutcome.Ok);
    public int IssueCount => Checks.Count(c => c.Outcome == CheckOutcome.Issue);
    public int SkippedCount => Checks.Count(c => c.Outcome == CheckOutcome.Skipped);
    public int InfoCount => Checks.Count(c => c.Outcome == CheckOutcome.Info);

    public bool HasIssues => IssueCount > 0;
  }
}
