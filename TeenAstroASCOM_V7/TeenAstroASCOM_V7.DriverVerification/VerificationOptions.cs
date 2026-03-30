namespace TeenAstroASCOM_V7.DriverVerification
{
  public sealed class VerificationOptions
  {
    public string IpField { get; set; } = "192.168.1.17:9999";
    public int PortField { get; set; } = 9999;
    public string ComPortField { get; set; } = "COM1";
    public string InterfaceType { get; set; } = "IP";
    public int Runs { get; set; } = 30;
    public int RunDelayMs { get; set; } = 250;
    public int ConnectTimeoutMs { get; set; } = 15000;
    public bool VerifyAllGet { get; set; } = true;
    public bool VerifyAllSet { get; set; } = true;
    public int MaxProperties { get; set; } = -1;
    public int ConnectRetries { get; set; } = 3;
    public int PrecheckRetries { get; set; } = 3;
    public bool VerboseVerify { get; set; }
    public bool TraceEnabled { get; set; }
    public bool VerifyRates { get; set; }

    /// <summary>
    /// If true, create driver via COM ProgID instead of in-process <c>new Telescope()</c>.
    /// </summary>
    public bool UseComActivation { get; set; }

    public const string DriverProgId = "ASCOM.TeenAstro.Telescope";
    public const string TraceKey = "Trace Level";
    public const string ComPortKey = "COM Port";
    public const string IpKey = "IP Adress";
    public const string PortKey = "Port";
    public const string InterfaceKey = "Interface";
  }
}
