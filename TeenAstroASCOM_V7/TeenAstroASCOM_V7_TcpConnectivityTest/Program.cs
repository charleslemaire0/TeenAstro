using System;
using System.IO;
using TeenAstroASCOM_V7.DriverVerification;

[System.Runtime.InteropServices.ComVisible(false)]
public class Program
{
  [STAThread]
  public static int Main(string[] args)
  {
    var options = new VerificationOptions();
    string reportJunitPath = null;
    string reportJsonPath = null;

    for (int i = 0; i < args.Length; i++)
    {
      string a = args[i];
      if (a.Equals("--ip", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) { options.IpField = args[++i]; }
      else if (a.Equals("--port", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int port)) options.PortField = port;
      }
      else if (a.Equals("--com-port", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) options.ComPortField = args[++i];
      else if (a.Equals("--interface", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) options.InterfaceType = args[++i];
      else if (a.Equals("--runs", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int runs)) options.Runs = runs;
      }
      else if (a.Equals("--delay", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int delay)) options.RunDelayMs = delay;
      }
      else if (a.Equals("--connect-timeout-ms", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int connectTimeout)) options.ConnectTimeoutMs = connectTimeout;
      }
      else if (a.Equals("--verify-get-only", StringComparison.OrdinalIgnoreCase)) options.VerifyAllSet = false;
      else if (a.Equals("--no-verify-get", StringComparison.OrdinalIgnoreCase)) options.VerifyAllGet = false;
      else if (a.Equals("--max-properties", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int maxProps)) options.MaxProperties = maxProps;
      }
      else if (a.Equals("--connect-retries", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int connectRetries)) options.ConnectRetries = connectRetries;
      }
      else if (a.Equals("--precheck-retries", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length)
      {
        if (int.TryParse(args[++i], out int precheckRetries)) options.PrecheckRetries = precheckRetries;
      }
      else if (a.Equals("--verbose-verify", StringComparison.OrdinalIgnoreCase)) options.VerboseVerify = true;
      else if (a.Equals("--trace", StringComparison.OrdinalIgnoreCase)) options.TraceEnabled = true;
      else if (a.Equals("--verify-rates", StringComparison.OrdinalIgnoreCase)) options.VerifyRates = true;
      else if (a.Equals("--com", StringComparison.OrdinalIgnoreCase)) options.UseComActivation = true;
      else if (a.Equals("--report-junit", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) reportJunitPath = args[++i];
      else if (a.Equals("--report-json", StringComparison.OrdinalIgnoreCase) && i + 1 < args.Length) reportJsonPath = args[++i];
      else if (a.Equals("--help", StringComparison.OrdinalIgnoreCase))
      {
        PrintUsage();
        return 0;
      }
    }

    VerificationReport report;
    try
    {
      report = TelescopeVerificationRunner.Run(options, Console.WriteLine);
    }
    catch (Exception ex)
    {
      Console.WriteLine("[FATAL] " + ex);
      return 2;
    }

    if (!string.IsNullOrWhiteSpace(reportJunitPath))
    {
      try
      {
        string dir = Path.GetDirectoryName(Path.GetFullPath(reportJunitPath));
        if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) Directory.CreateDirectory(dir);
        ReportWriter.WriteJunit(reportJunitPath, report);
        Console.WriteLine("[REPORT] JUnit: " + reportJunitPath);
      }
      catch (Exception ex)
      {
        Console.WriteLine("[ERROR] Failed to write JUnit report: " + ex.Message);
      }
    }

    if (!string.IsNullOrWhiteSpace(reportJsonPath))
    {
      try
      {
        string dir = Path.GetDirectoryName(Path.GetFullPath(reportJsonPath));
        if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) Directory.CreateDirectory(dir);
        ReportWriter.WriteJson(reportJsonPath, report);
        Console.WriteLine("[REPORT] JSON: " + reportJsonPath);
      }
      catch (Exception ex)
      {
        Console.WriteLine("[ERROR] Failed to write JSON report: " + ex.Message);
      }
    }

    Console.WriteLine("");
    Console.WriteLine($"[SUMMARY] Checks: OK={report.OkCount} Issue={report.IssueCount} Skipped={report.SkippedCount} Info={report.InfoCount}");
    return report.HasIssues ? 1 : 0;
  }

  private static void PrintUsage()
  {
    Console.WriteLine("TeenAstroASCOM_V7_TcpConnectivityTest (Conform-like driver verification)");
    Console.WriteLine("Args:");
    Console.WriteLine("  --ip <ip[:port]>            e.g. 192.168.1.17:9999");
    Console.WriteLine("  --port <port>              default 9999");
    Console.WriteLine("  --interface <COM|IP>       default IP");
    Console.WriteLine("  --com-port <COMx>          default COM1 (for COM interface)");
    Console.WriteLine("  --runs <n>                 default 30");
    Console.WriteLine("  --delay <ms>               default 250");
    Console.WriteLine("  --connect-timeout-ms <ms> default 15000");
    Console.WriteLine("  --verify-get-only         only verify GET (no SET)");
    Console.WriteLine("  --no-verify-get           disable GET verification");
    Console.WriteLine("  --max-properties <n>     stop after N properties (debug)");
    Console.WriteLine("  --connect-retries <n>    default 3");
    Console.WriteLine("  --precheck-retries <n>   default 3");
    Console.WriteLine("  --verbose-verify         log each property before calling get/set");
    Console.WriteLine("  --trace                  enable ASCOM Trace Level in profile");
    Console.WriteLine("  --verify-rates           after connect, test RA/DEC rate set/read (hex protocol)");
    Console.WriteLine("  --com                    create driver via COM ProgID (not in-process new Telescope)");
    Console.WriteLine("  --report-junit <path>    write JUnit XML report");
    Console.WriteLine("  --report-json <path>    write JSON report");
    Console.WriteLine("Exit: 0 = no issues, 1 = at least one Issue check, 2 = fatal error");
  }
}
