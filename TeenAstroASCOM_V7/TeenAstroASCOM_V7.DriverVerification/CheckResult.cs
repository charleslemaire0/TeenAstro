using System;

namespace TeenAstroASCOM_V7.DriverVerification
{
  public sealed class CheckResult
  {
    public CheckResult(string phase, string name, CheckOutcome outcome, string detail = null)
    {
      Phase = phase ?? "";
      Name = name ?? "";
      Outcome = outcome;
      Detail = detail;
    }

    public string Phase { get; }
    public string Name { get; }
    public CheckOutcome Outcome { get; }
    public string Detail { get; }

    public bool IsFailure => Outcome == CheckOutcome.Issue;
  }
}
