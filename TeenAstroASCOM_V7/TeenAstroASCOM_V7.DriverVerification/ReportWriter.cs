using System;
using System.Globalization;
using System.IO;
using System.Text;

namespace TeenAstroASCOM_V7.DriverVerification
{
  public static class ReportWriter
  {
    public static void WriteJson(string path, VerificationReport report)
    {
      if (string.IsNullOrWhiteSpace(path)) return;
      var sb = new StringBuilder(512);
      sb.Append("{\"startedUtc\":").Append(JsonString(report.StartedUtc.ToString("o", CultureInfo.InvariantCulture)));
      sb.Append(",\"finishedUtc\":");
      if (report.FinishedUtc.HasValue)
        sb.Append(JsonString(report.FinishedUtc.Value.ToString("o", CultureInfo.InvariantCulture)));
      else
        sb.Append("null");
      sb.Append(",\"summary\":{");
      sb.Append("\"ok\":").Append(report.OkCount);
      sb.Append(",\"issue\":").Append(report.IssueCount);
      sb.Append(",\"skipped\":").Append(report.SkippedCount);
      sb.Append(",\"info\":").Append(report.InfoCount);
      sb.Append("},\"checks\":[");
      for (int i = 0; i < report.Checks.Count; i++)
      {
        if (i > 0) sb.Append(',');
        var c = report.Checks[i];
        sb.Append("{\"phase\":").Append(JsonString(c.Phase));
        sb.Append(",\"name\":").Append(JsonString(c.Name));
        sb.Append(",\"outcome\":").Append(JsonString(c.Outcome.ToString()));
        sb.Append(",\"detail\":");
        if (c.Detail == null) sb.Append("null");
        else sb.Append(JsonString(c.Detail));
        sb.Append('}');
      }
      sb.Append("]}");
      File.WriteAllText(path, sb.ToString(), Encoding.UTF8);
    }

    public static void WriteJunit(string path, VerificationReport report)
    {
      if (string.IsNullOrWhiteSpace(path)) return;
      int tests = report.Checks.Count;
      int failures = report.IssueCount;
      var sb = new StringBuilder(tests * 120 + 300);
      sb.Append("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
      sb.Append("<testsuites tests=\"").Append(tests).Append("\" failures=\"").Append(failures).Append("\">");
      sb.Append("<testsuite name=\"TeenAstroDriverVerification\" tests=\"").Append(tests).Append("\" failures=\"").Append(failures).Append("\" time=\"0\">");
      foreach (var c in report.Checks)
      {
        string safeName = XmlEscape($"{c.Phase}.{c.Name}".Replace(' ', '_'));
        sb.Append("<testcase name=\"").Append(safeName).Append("\" classname=\"").Append(XmlEscape(c.Phase)).Append("\"");
        if (c.Outcome != CheckOutcome.Issue)
        {
          sb.Append(" />");
          continue;
        }
        sb.Append(">");
        string msg = XmlEscape(c.Detail ?? c.Name);
        sb.Append("<failure message=\"").Append(msg).Append("\">").Append(msg).Append("</failure>");
        sb.Append("</testcase>");
      }
      sb.Append("</testsuite></testsuites>");
      File.WriteAllText(path, sb.ToString(), Encoding.UTF8);
    }

    private static string JsonString(string s)
    {
      if (s == null) return "null";
      var sb = new StringBuilder(s.Length + 8);
      sb.Append('"');
      foreach (char ch in s)
      {
        switch (ch)
        {
          case '\\': sb.Append("\\\\"); break;
          case '"': sb.Append("\\\""); break;
          case '\n': sb.Append("\\n"); break;
          case '\r': sb.Append("\\r"); break;
          case '\t': sb.Append("\\t"); break;
          default:
            if (ch < 32) sb.AppendFormat(CultureInfo.InvariantCulture, "\\u{0:x4}", (int)ch);
            else sb.Append(ch);
            break;
        }
      }
      sb.Append('"');
      return sb.ToString();
    }

    private static string XmlEscape(string s)
    {
      if (string.IsNullOrEmpty(s)) return "";
      return s.Replace("&", "&amp;")
        .Replace("<", "&lt;")
        .Replace(">", "&gt;")
        .Replace("\"", "&quot;")
        .Replace("'", "&apos;");
    }
  }
}
