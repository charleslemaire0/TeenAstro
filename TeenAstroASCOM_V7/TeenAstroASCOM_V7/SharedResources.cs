//
// ================
// Shared Resources
// ================
//
// This class is a container for all shared resources that may be needed
// by the drivers served by the Local Server.
//
// NOTES:
//
//	* ALL DECLARATIONS MUST BE STATIC HERE!! INSTANCES OF THIS CLASS MUST NEVER BE CREATED!

using ASCOM.Utilities;
using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace ASCOM.LocalServer
{
  /// <summary>
  /// Add and manage resources that are shared by all drivers served by this local server here.
  /// Holds the single physical connection (COM or IP) to the TeenAstro mount, shared by
  /// both the Telescope and Focuser drivers. Ref-counted by uniqueId so the connection
  /// opens when the first driver connects and closes when the last disconnects.
  /// </summary>
  [HardwareClass]
  public static class SharedResources
  {
    // Object used for locking to prevent multiple drivers accessing common code at the same time
    private static readonly object lockObject = new object();

    // Shared serial port for COM connection
    private static Serial sharedSerial;
    // Shared list of connected driver instance unique IDs (Telescope and/or Focuser)
    private static List<Guid> uniqueIds = new List<Guid>();

    // Connection parameters (set when first driver connects)
    private static string connectionComPort;
    private static string connectionIP;
    private static short connectionPort;
    private static string connectionInterface;

    /// <summary>Whether the physical connection is currently open (COM port connected or IP session allowed).</summary>
    public static bool IsConnected
    {
      get
      {
        lock (lockObject)
        {
          if (connectionInterface == "COM")
            return sharedSerial != null && sharedSerial.Connected;
          if (connectionInterface == "IP")
            return uniqueIds.Count > 0; // For IP we don't hold a persistent socket
          return false;
        }
      }
    }

    /// <summary>True if the given driver instance is in the connected list.</summary>
    public static bool IsInstanceConnected(Guid uniqueId)
    {
      lock (lockObject) return uniqueIds.Contains(uniqueId);
    }

    /// <summary>
    /// Connect to the hardware. If this is the first connection, opens the COM port or establishes that IP is used.
    /// If another driver is already connected, only adds this instance to the ref count.
    /// </summary>
    /// <param name="uniqueId">Unique ID of the driver instance connecting.</param>
    /// <param name="comPort">COM port name (e.g. COM3). Used when interface is COM.</param>
    /// <param name="ip">IP address. Used when interface is IP.</param>
    /// <param name="port">TCP port. Used when interface is IP.</param>
    /// <param name="interfaceType">"COM" or "IP".</param>
    public static void Connect(Guid uniqueId, string comPort, string ip, short port, string interfaceType)
    {
      lock (lockObject)
      {
        if (uniqueIds.Contains(uniqueId))
          return;

        if (uniqueIds.Count == 0)
        {
          connectionComPort = comPort;
          connectionIP = ip;
          connectionPort = port;
          connectionInterface = interfaceType ?? "COM";

          if (connectionInterface == "COM")
          {
            sharedSerial = new Serial();
            try
            {
              string c = connectionComPort.Replace("COM", "");
              sharedSerial.Port = Convert.ToInt32(c);
            }
            catch (Exception ex)
            {
              sharedSerial = null;
              throw new ASCOM.DriverException("Invalid COM port: " + ex.Message);
            }
            sharedSerial.Speed = SerialSpeed.ps57600;
            sharedSerial.ReceiveTimeoutMs = 5000;
            try
            {
              sharedSerial.Connected = true;
            }
            catch (Exception ex)
            {
              sharedSerial.Dispose();
              sharedSerial = null;
              throw new ASCOM.DriverException("COM connect failed: " + ex.Message);
            }
          }
          // For IP we don't open a persistent socket; SendCommand will create TcpClient per request
        }

        uniqueIds.Add(uniqueId);
      }
    }

    /// <summary>
    /// Disconnect this driver instance. If this was the last instance, closes the physical connection.
    /// </summary>
    public static void Disconnect(Guid uniqueId)
    {
      lock (lockObject)
      {
        if (!uniqueIds.Contains(uniqueId))
          return;

        uniqueIds.Remove(uniqueId);

        if (uniqueIds.Count == 0)
        {
          if (connectionInterface == "COM" && sharedSerial != null)
          {
            try
            {
              if (sharedSerial.Connected)
                sharedSerial.Connected = false;
            }
            catch { }
            try { sharedSerial.Dispose(); } catch { }
            sharedSerial = null;
          }
          connectionInterface = null;
          connectionComPort = null;
          connectionIP = null;
        }
      }
    }

    /// <summary>
    /// Send a command to the mount and optionally read a response.
    /// </summary>
    /// <param name="command">Full command string (e.g. ":F?#" or ":GVP#").</param>
    /// <param name="mode">0 = blind (no response), 1 = single character response, 2 = response terminated by '#'.</param>
    /// <param name="buf">Response string (for mode 1 or 2). For mode 2, the trailing '#' is stripped.</param>
    /// <param name="retries">Number of retries on failure (default 3).</param>
    /// <returns>True if command was sent (and if mode 1/2, response received).</returns>
    public static bool SendCommand(string command, int mode, ref string buf, int retries = 3)
    {
      lock (lockObject)
      {
        if (connectionInterface == "COM")
          return SendCommandSerial(command, mode, ref buf, retries);
        if (connectionInterface == "IP")
          return SendCommandIP(command, mode, ref buf, retries);
        return false;
      }
    }

    private static bool SendCommandSerial(string command, int mode, ref string buf, int retries)
    {
      if (sharedSerial == null || !sharedSerial.Connected)
        return false;

      for (int k = 0; k <= retries; k++)
      {
        try
        {
          sharedSerial.ClearBuffers();
          sharedSerial.Transmit(command);
        }
        catch (Exception)
        {
          return false;
        }

        try
        {
          switch (mode)
          {
            case 0:
              return true;
            case 1:
              buf = sharedSerial.ReceiveCounted(1);
              return !string.IsNullOrEmpty(buf);
            case 2:
              buf = sharedSerial.ReceiveTerminated("#").TrimEnd('#');
              // Mode 2 expects a non-empty response payload before the '#'.
              // Treat empty as failure so callers can retry (helps when transport
              // returns an empty string on disconnect/timeout).
              return !string.IsNullOrEmpty(buf);
          }
        }
        catch
        {
          if (k == retries) return false;
        }
      }
      return false;
    }

    private static bool SendCommandIP(string command, int mode, ref string buf, int retries)
    {
      if (string.IsNullOrWhiteSpace(connectionIP))
        return false;

      // Users sometimes paste an URL (http://ip:port/path) or have trailing spaces.
      // The old code required a "pure" dotted-quad string and would fail parsing.
      int portToUse = connectionPort;
      string normalizedHost = null;

      // Accept common "IP:port" input in the IP textbox (e.g. "192.168.1.17:9999").
      if (!TryNormalizeHostAndOptionalPort(connectionIP, out normalizedHost, out portToUse))
        return false;

      if (string.IsNullOrWhiteSpace(normalizedHost))
        return false;

      if (!TryResolveToIPAddress(normalizedHost, out IPAddress addr))
        return false;

      // Keep each request bounded; the TeensAstro WiFi server may keep the TCP
      // connection open for a while (AutoClose idle timeout).
      // GVN/GVP/GXAS commands are expected to be fast, but initial network/firmware
      // responses can occasionally exceed the defaults, so keep some headroom.
      const int connectTimeoutMs = 5000;
      const int ioTimeoutMs = 10000;

      for (int k = 0; k <= retries; k++)
      {
        TcpClient client = null;
        try
        {
          client = new TcpClient();
          client.NoDelay = true;
          client.ReceiveTimeout = ioTimeoutMs;
          client.SendTimeout = ioTimeoutMs;

          var result = client.BeginConnect(addr, portToUse, null, null);
          if (!result.AsyncWaitHandle.WaitOne(connectTimeoutMs, true))
          {
            client.Close();
            return false;
          }
          client.EndConnect(result);

          var stream = client.GetStream();
          byte[] outBytes = Encoding.ASCII.GetBytes(command);
          stream.Write(outBytes, 0, outBytes.Length);
          stream.Flush();
          buf = "";

          switch (mode)
          {
            case 0:
              client.Close();
              return true;
            case 1:
            case 2:
              if (stream.CanRead)
              {
                // Mode 1: expect a single-byte reply (first char only).
                // Mode 2: expect a response terminated by '#'.
                //
                // The previous implementation read until the TCP socket closed,
                // which could hang if the server uses "auto-close on idle".
                if (mode == 1)
                {
                  int first = stream.ReadByte(); // respects ReceiveTimeout
                  buf = first >= 0 ? ((char)first).ToString() : "";
                }
                else
                {
                  var sb = new StringBuilder();
                  // Replies are small ASCII strings, but cap to avoid runaway on bad firmware.
                  int maxChars = Math.Max(128, client.ReceiveBufferSize + 1);
                  while (sb.Length < maxChars)
                  {
                    int b = stream.ReadByte(); // respects ReceiveTimeout
                    if (b < 0) break; // remote closed
                    if ((char)b == '#') break;
                    sb.Append((char)b);
                  }
                  buf = sb.ToString();
                }
              }
              client.Close();
              // Mode 1/2 must receive some payload. If the WiFi bridge closes
              // the socket without sending data, buf will be empty; treat as
              // failure so SendCommand() can retry.
              return !string.IsNullOrEmpty(buf);
            default:
              client.Close();
              return false;
          }
        }
        catch
        {
          try { client?.Close(); } catch { }
          if (k == retries) return false;
        }
      }
      return false;
    }

    // Returns true when a usable host is extracted.
    // Also optionally extracts a numeric ":port" suffix from typical IPv4/hostname forms.
    private static bool TryNormalizeHostAndOptionalPort(string input, out string host, out int portOverride)
    {
      host = null;
      portOverride = connectionPort;

      if (string.IsNullOrWhiteSpace(input))
        return false;

      string s = input.Trim();

      // Remove URL scheme if present.
      if (s.StartsWith("http://", StringComparison.OrdinalIgnoreCase))
        s = s.Substring("http://".Length);
      else if (s.StartsWith("https://", StringComparison.OrdinalIgnoreCase))
        s = s.Substring("https://".Length);

      // Remove path/query/fragment.
      int slash = s.IndexOf('/');
      if (slash >= 0) s = s.Substring(0, slash);
      int q = s.IndexOf('?');
      if (q >= 0) s = s.Substring(0, q);
      int hash = s.IndexOf('#');
      if (hash >= 0) s = s.Substring(0, hash);

      s = s.Trim();
      if (s.Length == 0) return false;

      // Bracketed IPv6: [::1]:9999
      if (s.StartsWith("[", StringComparison.OrdinalIgnoreCase))
      {
        int end = s.IndexOf(']');
        if (end < 0) return false;

        host = s.Substring(1, end - 1);

        string after = s.Substring(end + 1).Trim();
        if (after.StartsWith(":", StringComparison.Ordinal))
        {
          string portPart = after.Substring(1).Trim();
          if (int.TryParse(portPart, out int p) && p > 0 && p <= 65535)
            portOverride = p;
        }
        return true;
      }

      // IPv4/hostname with single colon: "192.168.1.17:9999" or "host:9999"
      int firstColon = s.IndexOf(':');
      int lastColon = s.LastIndexOf(':');
      if (firstColon > 0 && firstColon == lastColon)
      {
        string maybeHost = s.Substring(0, firstColon).Trim();
        string portPart = s.Substring(firstColon + 1).Trim();
        if (int.TryParse(portPart, out int p) && p > 0 && p <= 65535)
        {
          host = maybeHost;
          portOverride = p;
          return true;
        }
      }

      // No port detected; treat whole input as host/IP.
      host = s.Trim();
      return !string.IsNullOrWhiteSpace(host);
    }

    // Removes optional scheme and any trailing path/query/port so the remaining
    // value can be parsed/resolved as an IP/host.
    private static string NormalizeIpHost(string input)
    {
      if (input == null) return "";

      string s = input.Trim();
      if (s.Length == 0) return "";

      // Remove URL scheme if present.
      if (s.StartsWith("http://", StringComparison.OrdinalIgnoreCase))
        s = s.Substring("http://".Length);
      else if (s.StartsWith("https://", StringComparison.OrdinalIgnoreCase))
        s = s.Substring("https://".Length);

      // Remove path/query/fragment.
      int slash = s.IndexOf('/');
      if (slash >= 0) s = s.Substring(0, slash);
      int q = s.IndexOf('?');
      if (q >= 0) s = s.Substring(0, q);
      int hash = s.IndexOf('#');
      if (hash >= 0) s = s.Substring(0, hash);

      // If it's host:port (IPv4 or hostname), keep only host.
      // For bracketed IPv6 ([::1]:1234) keep the bracketed host.
      if (s.StartsWith("[", StringComparison.OrdinalIgnoreCase))
      {
        int end = s.IndexOf(']');
        if (end >= 0) return s.Substring(1, end - 1);
      }
      else
      {
        int firstColon = s.IndexOf(':');
        int lastColon = s.LastIndexOf(':');
        // Only strip port for "single-colon" forms like "192.168.0.10:9999".
        if (firstColon > 0 && firstColon == lastColon)
          s = s.Substring(0, firstColon);
      }

      return s.Trim();
    }

    private static bool TryResolveToIPAddress(string hostOrIp, out IPAddress addr)
    {
      addr = null;
      if (string.IsNullOrWhiteSpace(hostOrIp)) return false;

      if (IPAddress.TryParse(hostOrIp, out addr))
        return true;

      try
      {
        var addresses = Dns.GetHostAddresses(hostOrIp);
        foreach (var a in addresses)
        {
          addr = a;
          return true;
        }
      }
      catch
      {
        // ignore and return false
      }
      return false;
    }

    #region Dispose method to clean up resources before close

    /// <summary>
    /// Deterministically release both managed and unmanaged resources that are used by this class.
    /// </summary>
    public static void Dispose()
    {
      lock (lockObject)
      {
        uniqueIds.Clear();
        if (sharedSerial != null)
        {
          try
          {
            if (sharedSerial.Connected)
              sharedSerial.Connected = false;
          }
          catch { }
          try { sharedSerial.Dispose(); } catch { }
          sharedSerial = null;
        }
        connectionInterface = null;
      }
    }

    #endregion

    #region Legacy API (for any code that still references these)

    /// <summary>
    /// Shared serial port (may be null if using IP or not connected).
    /// </summary>
    public static Serial SharedSerial
    {
      get { return sharedSerial; }
    }

    /// <summary>
    /// Number of driver instances currently connected.
    /// </summary>
    public static int Connections
    {
      get { lock (lockObject) return uniqueIds.Count; }
    }

    /// <summary>
    /// Send a message and return response terminated by '#'.
    /// </summary>
    public static string SendMessage(string message)
    {
      string buf = "";
      if (!SendCommand(message, 2, ref buf))
        throw new ASCOM.DriverException("SendMessage failed");
      return buf;
    }

    #endregion
  }
}
