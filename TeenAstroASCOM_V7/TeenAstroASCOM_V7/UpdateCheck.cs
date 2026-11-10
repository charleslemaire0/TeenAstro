//
// Asks GitHub whether a newer TeenAstro ASCOM driver or firmware is published.
//

using System;
using System.Globalization;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;

namespace ASCOM.TeenAstro
{
  /// <summary>
  /// Compares this driver and, when known, the mount firmware with the Release branch on GitHub.
  /// </summary>
  public static class UpdateCheck
  {
    const string Repo = "charleslemaire0/TeenAstro";
    const string Api = "https://api.github.com/repos/" + Repo;
    const string ReleasesUrl = "https://github.com/" + Repo + "/releases/latest";

    /// <summary>major, minor, patch, revision. Missing parts are 0. Null when the text has no version.</summary>
    public static int[] ParseVersion(string text)
    {
      if (string.IsNullOrEmpty(text))
        return null;
      Match match = Regex.Match(text, @"(\d+)\.(\d+)(?:\.(\d+))?(?:\.(\d+))?");
      if (!match.Success)
        return null;
      return new[]
      {
        int.Parse(match.Groups[1].Value, CultureInfo.InvariantCulture),
        int.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture),
        match.Groups[3].Success ? int.Parse(match.Groups[3].Value, CultureInfo.InvariantCulture) : 0,
        match.Groups[4].Success ? int.Parse(match.Groups[4].Value, CultureInfo.InvariantCulture) : 0
      };
    }

    public static int Compare(int[] left, int[] right)
    {
      if (left == null || right == null)
        return 0;
      for (int i = 0; i < 4; i++)
      {
        int delta = left[i] - right[i];
        if (delta != 0)
          return delta;
      }
      return 0;
    }

    public static string Format(int[] version)
    {
      if (version == null)
        return "";
      if (version[3] != 0)
        return version[0] + "." + version[1] + "." + version[2] + "." + version[3];
      if (version[2] != 0)
        return version[0] + "." + version[1] + "." + version[2];
      return version[0] + "." + version[1];
    }

    public static string AssemblyVersionFromSource(string source)
    {
      if (string.IsNullOrEmpty(source))
        return null;
      Match match = Regex.Match(source, "AssemblyVersion\\(\"([^\"]+)\"\\)");
      return match.Success ? match.Groups[1].Value : null;
    }

    public static string FirmwareNumberFromHeader(string header)
    {
      if (string.IsNullOrEmpty(header))
        return null;
      Match match = Regex.Match(header, "FirmwareNumber\\s+\"([^\"]+)\"");
      return match.Success ? match.Groups[1].Value : null;
    }

    /// <summary>Highest Release_X.Y named in a GitHub matching-refs payload.</summary>
    public static string HighestReleaseLine(string json)
    {
      if (string.IsNullOrEmpty(json))
        return null;
      string bestLabel = null;
      int bestMajor = -1;
      int bestMinor = -1;
      foreach (Match match in Regex.Matches(json, "Release_(\\d+)\\.(\\d+)"))
      {
        int major = int.Parse(match.Groups[1].Value, CultureInfo.InvariantCulture);
        int minor = int.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture);
        if (major > bestMajor || (major == bestMajor && minor > bestMinor))
        {
          bestMajor = major;
          bestMinor = minor;
          bestLabel = major + "." + minor;
        }
      }
      return bestLabel;
    }

    public static string Describe(string localDriver, string publishedDriver, string publishedFirmware, string mountFirmware, string releaseLine)
    {
      int[] local = ParseVersion(localDriver);
      int[] published = ParseVersion(publishedDriver);
      var lines = new System.Collections.Generic.List<string>();
      string branch = string.IsNullOrEmpty(releaseLine) ? "" : " (Release " + releaseLine + ")";

      if (published == null)
        lines.Add("The published driver version could not be read.");
      else if (local != null && Compare(published, local) > 0)
      {
        lines.Add("A newer driver is available: " + Format(published) + branch);
        lines.Add("This copy is " + Format(local) + ".");
        lines.Add(ReleasesUrl);
      }
      else if (local != null && Compare(published, local) < 0)
        lines.Add("This driver is " + Format(local) + ". The published copy is " + Format(published) + branch + ".");
      else
        lines.Add("This driver is " + Format(local) + ", the same as the published copy" + branch + ".");

      lines.Add("");
      int[] firmware = ParseVersion(publishedFirmware);
      int[] mount = ParseVersion(mountFirmware);
      if (firmware == null)
        lines.Add("The published firmware version could not be read.");
      else if (mount == null)
        lines.Add("Published firmware is " + Format(firmware) + ". The mount is not connected, so its version was not read.");
      else if (Compare(firmware, mount) > 0)
      {
        lines.Add("Newer firmware is available: " + Format(firmware) + branch);
        lines.Add("The mount reports " + Format(mount) + ".");
      }
      else if (Compare(firmware, mount) < 0)
        lines.Add("The mount reports " + Format(mount) + ". Published firmware is " + Format(firmware) + branch + ".");
      else
        lines.Add("Published firmware is " + Format(firmware) + ", the same as the mount.");

      return string.Join(Environment.NewLine, lines.ToArray());
    }

    /// <summary>Firmware from :GVN# when a driver already holds the connection. Null otherwise.</summary>
    public static string TryReadMountFirmware()
    {
      try
      {
        if (!ASCOM.LocalServer.SharedResources.IsConnected)
          return null;
        return Telescope.TelescopeHardware.CommandString("GVN", false);
      }
      catch (Exception)
      {
        return null;
      }
    }

    public static string Query(string localDriver, string mountFirmware)
    {
      try
      {
        ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
      }
      catch (NotSupportedException)
      {
      }

      int[] local = ParseVersion(localDriver);
      string line = local == null ? "1.6" : local[0] + "." + local[1];
      string listed = HighestReleaseLine(Get(Api + "/git/matching-refs/heads/Release_"));
      if (!string.IsNullOrEmpty(listed))
      {
        int[] listedVersion = ParseVersion(listed);
        int[] currentLine = ParseVersion(line);
        if (listedVersion != null && currentLine != null && Compare(listedVersion, currentLine) > 0)
          line = listed;
      }

      string driverSource = Get(Raw(line, "TeenAstroASCOM_V7/TeenAstroASCOM_V7/Properties/AssemblyInfo.cs"));
      string firmwareHeader = Get(Raw(line, "TeenAstroMainUnit/FirmwareDef.h"));
      if (driverSource == null && firmwareHeader == null)
        return "Could not reach GitHub to check for updates.";

      return Describe(
        localDriver,
        AssemblyVersionFromSource(driverSource),
        FirmwareNumberFromHeader(firmwareHeader),
        mountFirmware,
        line);
    }

    static string Raw(string releaseLine, string path)
    {
      return "https://raw.githubusercontent.com/" + Repo + "/Release_" + releaseLine + "/" + path;
    }

    static string Get(string url)
    {
      try
      {
        var request = (HttpWebRequest)WebRequest.Create(url);
        request.UserAgent = "TeenAstroAscom";
        request.Accept = "application/vnd.github+json";
        request.Timeout = 8000;
        request.ReadWriteTimeout = 8000;
        using (var response = (HttpWebResponse)request.GetResponse())
        using (var reader = new StreamReader(response.GetResponseStream()))
          return reader.ReadToEnd();
      }
      catch (Exception)
      {
        return null;
      }
    }
  }
}
