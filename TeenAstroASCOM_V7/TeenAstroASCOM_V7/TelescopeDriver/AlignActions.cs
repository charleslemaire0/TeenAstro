// PC alignment actions shared by the ASCOM telescope driver.
// AlpacaTelescope::putAction accepts the same names and parameters.
//
//   AutoAlign                         :AA#   then two SyncToCoordinates of solved positions
//   AlignStart     2 | m | r3..r9     :A0,2#  :A0,m#  :A0,r<n>#     mount is at home
//   AlignAtTarget  2 | m | r3..r9     :A*#    :A*,m#  :A*,r<n>#     tube is already on star 1
//   AlignAccept    <star>[,raHours,decDeg]
//                                      writes the solved target when coordinates are present,
//                                      then :A<star>#. Does not slew and does not sync.
//   AlignSave                         :AW#
//   AlignAbort                        :AB#
//   AlignPolarDone                    :AP#   end of a mechanical-pole session
//   AlignStatus                       stars=<n>;rms=<arcsec>;terms=<CPI>;pier=<in>,<out>
//   AlignErrors                       ch,np,id,rms in arcseconds; me,ma,ae in degrees

using System;
using System.Globalization;

namespace ASCOM.TeenAstro.Telescope
{
  public static class AlignActions
  {
    public static readonly string[] Names =
    {
      "AutoAlign",
      "AlignStart",
      "AlignAtTarget",
      "AlignAccept",
      "AlignSave",
      "AlignAbort",
      "AlignPolarDone",
      "AlignStatus",
      "AlignErrors"
    };

    public enum Kind
    {
      AutoAlign,
      Start,
      AtTarget,
      Accept,
      Save,
      Abort,
      PolarDone,
      Status,
      Errors
    }

    public enum ParseFailure
    {
      None,
      UnknownAction,
      BadParameters
    }

    public struct Plan
    {
      public Kind Kind;
      /// <summary>LX200 body without the leading ':' and trailing '#'. Null for a query action.</summary>
      public string Command;
      public string Reply;
      public bool SetTarget;
      public double RaHours;
      public double DecDeg;
    }

    public static bool TryParse(string actionName, string parameters, out Plan plan, out ParseFailure failure, out string error)
    {
      plan = new Plan();
      failure = ParseFailure.None;
      error = null;

      string name = (actionName ?? "").Trim();
      string p = (parameters ?? "").Trim();

      switch (name)
      {
        case "AutoAlign":
          plan.Kind = Kind.AutoAlign;
          plan.Command = "AA";
          plan.Reply = "AutoAlign Started";
          return true;
        case "AlignSave":
          return Bare(p, Kind.Save, "AW", "AlignSave", ref plan, ref failure, ref error);
        case "AlignAbort":
          return Bare(p, Kind.Abort, "AB", "AlignAbort", ref plan, ref failure, ref error);
        case "AlignPolarDone":
          return Bare(p, Kind.PolarDone, "AP", "AlignPolarDone", ref plan, ref failure, ref error);
        case "AlignStatus":
          return Bare(p, Kind.Status, null, null, ref plan, ref failure, ref error);
        case "AlignErrors":
          return Bare(p, Kind.Errors, null, null, ref plan, ref failure, ref error);
        case "AlignStart":
          return Mode(p, false, Kind.Start, "AlignStart", ref plan, ref failure, ref error);
        case "AlignAtTarget":
          return Mode(p, true, Kind.AtTarget, "AlignAtTarget", ref plan, ref failure, ref error);
        case "AlignAccept":
          return Accept(p, ref plan, ref failure, ref error);
        default:
          failure = ParseFailure.UnknownAction;
          error = "Action " + name + " is not implemented by this driver";
          return false;
      }
    }

    /// <summary>
    /// Status text returned by AlignStatus. rms is arcseconds, terms is three of C, P, I or '-'.
    /// </summary>
    public static bool TryFormatStatus(int stars, double rmsArcsec, string terms, int pierIn, int pierOut, out string text)
    {
      text = null;
      if (stars < 0 || pierIn < 0 || pierOut < 0)
        return false;
      if (double.IsNaN(rmsArcsec) || double.IsInfinity(rmsArcsec))
        return false;
      if (!TermsOk(terms))
        return false;
      text = string.Format(
        CultureInfo.InvariantCulture,
        "stars={0};rms={1};terms={2};pier={3},{4}",
        stars,
        rmsArcsec.ToString("0.000000", CultureInfo.InvariantCulture),
        terms,
        pierIn,
        pierOut);
      return true;
    }

    /// <summary>
    /// AlignErrors text. ch, np, id and rms are arcseconds. me, ma and ae are degrees.
    /// me is Wallace ME, ma is Wallace MA, ae is the two-star residual.
    /// </summary>
    public static bool TryFormatErrors(double ch, double np, double id, double meDeg, double maDeg, double rmsArcsec, double aeDeg, out string text)
    {
      text = null;
      if (!Finite(ch) || !Finite(np) || !Finite(id) || !Finite(meDeg) || !Finite(maDeg) || !Finite(rmsArcsec) || !Finite(aeDeg))
        return false;
      text = string.Format(
        CultureInfo.InvariantCulture,
        "ch={0};np={1};id={2};me={3};ma={4};rms={5};ae={6}",
        ch.ToString("0.000000", CultureInfo.InvariantCulture),
        np.ToString("0.000000", CultureInfo.InvariantCulture),
        id.ToString("0.000000", CultureInfo.InvariantCulture),
        meDeg.ToString("0.000000", CultureInfo.InvariantCulture),
        maDeg.ToString("0.000000", CultureInfo.InvariantCulture),
        rmsArcsec.ToString("0.000000", CultureInfo.InvariantCulture),
        aeDeg.ToString("0.000000", CultureInfo.InvariantCulture));
      return true;
    }

    /// <summary>LX200 signed DMS, for example <c>+00*12:34</c>, as decimal degrees.</summary>
    public static bool TryParseDms(string text, out double degrees)
    {
      degrees = 0;
      text = (text ?? "").Trim();
      if (text.Length < 6)
        return false;
      int sign = 1;
      int i = 0;
      if (text[0] == '+' || text[0] == '-')
      {
        if (text[0] == '-')
          sign = -1;
        i = 1;
      }
      int star = text.IndexOf('*', i);
      int colon = star < 0 ? -1 : text.IndexOf(':', star + 1);
      if (star < 0 || colon < 0 || colon + 1 >= text.Length)
        return false;
      if (!int.TryParse(text.Substring(i, star - i), NumberStyles.Integer, CultureInfo.InvariantCulture, out int dd)
          || !int.TryParse(text.Substring(star + 1, colon - star - 1), NumberStyles.Integer, CultureInfo.InvariantCulture, out int mm)
          || !int.TryParse(text.Substring(colon + 1), NumberStyles.Integer, CultureInfo.InvariantCulture, out int ss))
        return false;
      if (dd < 0 || mm < 0 || mm > 59 || ss < 0 || ss > 59)
        return false;
      degrees = sign * (dd + mm / 60.0 + ss / 3600.0);
      return true;
    }

    static bool Finite(double value)
    {
      return !double.IsNaN(value) && !double.IsInfinity(value);
    }

    static bool Bare(string p, Kind kind, string command, string reply, ref Plan plan, ref ParseFailure failure, ref string error)
    {
      if (p.Length != 0)
      {
        failure = ParseFailure.BadParameters;
        error = "This action takes no parameters";
        return false;
      }
      plan.Kind = kind;
      plan.Command = command;
      plan.Reply = reply;
      return true;
    }

    static bool Mode(string p, bool atTarget, Kind kind, string replyName, ref Plan plan, ref ParseFailure failure, ref string error)
    {
      if (!TryMode(p, atTarget, out string command, out string label))
      {
        failure = ParseFailure.BadParameters;
        error = replyName + " requires 2, m, or r3..r9";
        return false;
      }
      plan.Kind = kind;
      plan.Command = command;
      plan.Reply = replyName + " " + label;
      return true;
    }

    static bool Accept(string p, ref Plan plan, ref ParseFailure failure, ref string error)
    {
      string[] parts = p.Split(',');
      for (int i = 0; i < parts.Length; i++)
        parts[i] = parts[i].Trim();

      if (parts.Length != 1 && parts.Length != 3)
      {
        failure = ParseFailure.BadParameters;
        error = "AlignAccept requires a star 1..9, or star,raHours,decDeg";
        return false;
      }
      if (!TryStar(parts[0], out int star))
      {
        failure = ParseFailure.BadParameters;
        error = "AlignAccept star must be 1..9";
        return false;
      }

      plan.Kind = Kind.Accept;
      plan.Command = "A" + star.ToString(CultureInfo.InvariantCulture);
      plan.Reply = "AlignAccept " + star.ToString(CultureInfo.InvariantCulture);

      if (parts.Length == 3)
      {
        if (!TryFinite(parts[1], out double ra) || ra < 0.0 || ra > 24.0
            || !TryFinite(parts[2], out double dec) || dec < -90.0 || dec > 90.0)
        {
          failure = ParseFailure.BadParameters;
          error = "AlignAccept coordinates must be RA hours 0..24 and Dec degrees -90..90";
          return false;
        }
        plan.SetTarget = true;
        plan.RaHours = ra;
        plan.DecDeg = dec;
      }
      return true;
    }

    /// <summary>
    /// 2 → two-star, m → mechanical pole, r3..r9 → rigid or progressive session.
    /// At the target, two-star is :A*# (the firmware rejects :A*,2#).
    /// </summary>
    static bool TryMode(string p, bool atTarget, out string command, out string label)
    {
      command = null;
      label = null;
      if (p == "2")
      {
        command = atTarget ? "A*" : "A0,2";
        label = "2";
        return true;
      }
      if (p == "m")
      {
        command = atTarget ? "A*,m" : "A0,m";
        label = "m";
        return true;
      }
      if (p.Length == 2 && p[0] == 'r' && p[1] >= '3' && p[1] <= '9')
      {
        command = (atTarget ? "A*,r" : "A0,r") + p[1];
        label = p;
        return true;
      }
      return false;
    }

    static bool TryStar(string text, out int star)
    {
      star = 0;
      if (text == null || text.Length != 1 || text[0] < '1' || text[0] > '9')
        return false;
      star = text[0] - '0';
      return true;
    }

    static bool TryFinite(string text, out double value)
    {
      value = 0;
      if (string.IsNullOrEmpty(text))
        return false;
      if (!double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out value))
        return false;
      return !double.IsNaN(value) && !double.IsInfinity(value);
    }

    static bool TermsOk(string terms)
    {
      if (terms == null || terms.Length != 3)
        return false;
      for (int i = 0; i < 3; i++)
      {
        char c = terms[i];
        if (c != 'C' && c != 'P' && c != 'I' && c != '-')
          return false;
      }
      return true;
    }
  }
}
