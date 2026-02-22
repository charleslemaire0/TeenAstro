/*
 * TeenAstroConnection.cs - Pure C# LX200 connection for ASCOM driver
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;
using System.Text;

namespace ASCOM.TeenAstro.Telescope
{
    internal sealed class TeenAstroConnection
    {
        private IX200Transport _transport;
        private LX200Protocol _protocol;
        private const int DefaultBaud = 57600;
        private const int CmdTimeout = 30;
        private const int GXASTimeout = 100;

        public bool IsConnected => _transport != null && _transport.IsOpen;

        public bool ConnectSerial(string port)
        {
            if (string.IsNullOrEmpty(port))
                return false;
            Disconnect();
            var serial = new SerialTransport();
            if (!serial.Open(port, DefaultBaud))
            {
                serial.Dispose();
                return false;
            }
            _transport = serial;
            _protocol = new LX200Protocol(_transport);
            _protocol.SetTimeout(CmdTimeout);
            return true;
        }

        public bool ConnectTcp(string ip, int port)
        {
            if (string.IsNullOrEmpty(ip) || port <= 0)
                return false;
            Disconnect();
            var tcp = new TcpTransport();
            if (!tcp.Open(ip, port))
            {
                tcp.Dispose();
                return false;
            }
            _transport = tcp;
            _protocol = new LX200Protocol(_transport);
            _protocol.SetTimeout(CmdTimeout);
            return true;
        }

        public void Disconnect()
        {
            _protocol = null;
            _transport?.Dispose();
            _transport = null;
        }

        private void EnsureConnected()
        {
            if (!IsConnected)
                throw new InvalidOperationException("Not connected");
        }

        private static string BuildCommand(string cmd, bool raw)
        {
            if (raw)
                return cmd;
            return cmd.Contains("#") ? cmd : ":" + cmd + "#";
        }

        public bool CommandBlind(string command, bool raw)
        {
            EnsureConnected();
            string c = BuildCommand(command, raw);
            return _protocol.SendReceive(c, CmdReply.No, null, out _);
        }

        public bool CommandBool(string command, bool raw)
        {
            EnsureConnected();
            string c = BuildCommand(command, raw);
            byte[] buf = new byte[32];
            CmdReply rt = LX200Protocol.GetReplyType(c);
            if (rt == CmdReply.Invalid)
                rt = CmdReply.ShortBool;
            bool ok = _protocol.SendReceive(c, rt, buf, out int len);
            if (!ok) return false;
            return len > 0 && (char)buf[0] == '1';
        }

        public string CommandString(string command, bool raw)
        {
            EnsureConnected();
            string c = BuildCommand(command, raw);
            byte[] buf = new byte[256];
            if (!_protocol.Get(c, buf, out int len))
                return null;
            return Encoding.ASCII.GetString(buf, 0, len);
        }

        public bool AbortSlew()
        {
            EnsureConnected();
            return _protocol.Set(":Q#");
        }

        public bool HasSite()
        {
            EnsureConnected();
            byte[] buf = new byte[8];
            if (!_protocol.Get(":hS#", buf, out int len))
                return false;
            return len > 0 && (char)buf[0] == '1';
        }

        public bool HasMotors()
        {
            EnsureConnected();
            byte[] buf = new byte[8];
            if (!_protocol.Get(":GXJm#", buf, out int len))
                return false;
            return len > 0 && (char)buf[0] == '1';
        }

        public bool Park()
        {
            EnsureConnected();
            return _protocol.Set(":hP#");
        }

        public bool Unpark()
        {
            EnsureConnected();
            return _protocol.Set(":hR#");
        }

        public bool SetPark()
        {
            EnsureConnected();
            return _protocol.Set(":hQ#");
        }

        public bool PulseGuide(int direction, int durationMs)
        {
            EnsureConnected();
            if (durationMs < 1 || durationMs > 30000 || direction < 0 || direction > 3)
                return false;
            char[] dirs = { 'n', 's', 'e', 'w' };
            string cmd = string.Format(":Mg{0}{1}#", dirs[direction], durationMs);
            return _protocol.Set(cmd);
        }

        public bool MoveAxis(int axis, double rateArcsecPerSec)
        {
            EnsureConnected();
            char ax = (axis == 0) ? '1' : '2';
            string cmd = string.Format(":M{0}{1:F2}#", ax, rateArcsecPerSec);
            return _protocol.Set(cmd);
        }

        public bool SyncToEquatorial()
        {
            EnsureConnected();
            byte[] buf = new byte[16];
            if (!_protocol.Get(":CM#", buf, out int len))
                return false;
            string s = Encoding.ASCII.GetString(buf, 0, len).Trim();
            return s == "N/A";
        }

        public bool SyncToAltAz()
        {
            EnsureConnected();
            return _protocol.Set(":CA#");
        }

        public bool SlewToEquatorial(byte[] outReply, int outSize)
        {
            EnsureConnected();
            if (outReply == null || outSize < 2)
                return false;
            Array.Clear(outReply, 0, Math.Min(outReply.Length, outSize));
            return _protocol.Get(":MS#", outReply, out int len) && len > 0;
        }

        public bool SlewToAltAz(byte[] outReply, int outSize)
        {
            EnsureConnected();
            if (outReply == null || outSize < 2)
                return false;
            Array.Clear(outReply, 0, Math.Min(outReply.Length, outSize));
            return _protocol.Get(":MA#", outReply, out int len) && len > 0;
        }

        public bool SetTargetRA(double raHours)
        {
            EnsureConnected();
            string cmd = string.Format("SrL,{0:F5}", 15.0 * raHours);
            return CommandBool(cmd, false);
        }

        public bool SetTargetDec(double decDeg)
        {
            EnsureConnected();
            string sg = decDeg >= 0 ? "+" : "-";
            string cmd = string.Format("SdL{0}{1:F5}", sg, Math.Abs(decDeg));
            return CommandBool(cmd, false);
        }

        public bool SetTargetAz(string azStr)
        {
            EnsureConnected();
            if (string.IsNullOrEmpty(azStr))
                return false;
            string cmd = ":Sz" + azStr + "#";
            return _protocol.Set(cmd);
        }

        public bool SetTargetAlt(string altStr)
        {
            EnsureConnected();
            if (string.IsNullOrEmpty(altStr))
                return false;
            string cmd = ":Sa" + altStr + "#";
            return _protocol.Set(cmd);
        }

        public bool GetSiteLatitude(out double lat)
        {
            lat = 0;
            EnsureConnected();
            byte[] buf = new byte[32];
            if (!_protocol.Get(":Gtf#", buf, out int len))
                return false;
            string s = Encoding.ASCII.GetString(buf, 0, len).Trim();
            return DmsToDouble(s, out lat);
        }

        public bool GetSiteLongitude(out double lon)
        {
            lon = 0;
            EnsureConnected();
            byte[] buf = new byte[32];
            if (!_protocol.Get(":Ggf#", buf, out int len))
                return false;
            string s = Encoding.ASCII.GetString(buf, 0, len).Trim();
            int sign = 1;
            if (s.Length > 0 && (s[0] == '-' || s[0] == '+'))
            {
                sign = s[0] == '-' ? -1 : 1;
                s = s.Substring(1);
            }
            if (!DmsToDouble(s, out lon))
                return false;
            lon *= sign;
            return true;
        }

        public bool SetSiteLatitude(string latStr)
        {
            EnsureConnected();
            if (string.IsNullOrEmpty(latStr))
                return false;
            return _protocol.Set(":St" + latStr + "#");
        }

        public bool SetSiteLongitude(string lonStr)
        {
            EnsureConnected();
            if (string.IsNullOrEmpty(lonStr))
                return false;
            return _protocol.Set(":Sg" + lonStr + "#");
        }

        public bool GetUTCTimestamp(out double secs)
        {
            secs = 0;
            EnsureConnected();
            byte[] buf = new byte[24];
            if (!_protocol.Get(":GXT2#", buf, out int len))
                return false;
            string s = Encoding.ASCII.GetString(buf, 0, len).Trim();
            return double.TryParse(s, System.Globalization.NumberStyles.Float,
                System.Globalization.CultureInfo.InvariantCulture, out secs);
        }

        public bool SetUTCTimestamp(long unixSecs)
        {
            EnsureConnected();
            string cmd = string.Format(":SXT2,{0}#", unixSecs);
            return _protocol.Set(cmd);
        }

        public bool EnableTracking(bool on)
        {
            EnsureConnected();
            return _protocol.Set(on ? ":Te#" : ":Td#");
        }

        public bool GetMountState(out GXASState state)
        {
            state = default;
            EnsureConnected();
            _protocol.SetTimeout(GXASTimeout);
            byte[] buf = new byte[96];
            bool ok = _protocol.Get(":GXAS#", buf, out int len);
            _protocol.SetTimeout(CmdTimeout);
            if (!ok || len != 88)
                return false;
            string b64 = Encoding.ASCII.GetString(buf, 0, 88);
            return GXASDecoder.TryDecode(b64, out state);
        }

        private static bool DmsToDouble(string dms, out double deg)
        {
            deg = 0;
            if (string.IsNullOrWhiteSpace(dms))
                return false;
            dms = dms.Trim();
            char[] sep = { ':', '*', (char)0x00B0, '\'' };
            string[] parts = dms.Split(sep, StringSplitOptions.RemoveEmptyEntries);
            if (parts.Length < 2)
                return double.TryParse(dms, System.Globalization.NumberStyles.Float,
                    System.Globalization.CultureInfo.InvariantCulture, out deg);
            if (!int.TryParse(parts[0].Trim(), out int d))
                return false;
            if (!int.TryParse(parts.Length > 1 ? parts[1].Trim() : "0", out int m))
                return false;
            double s = 0;
            if (parts.Length > 2)
                double.TryParse(parts[2].Trim(), System.Globalization.NumberStyles.Float,
                    System.Globalization.CultureInfo.InvariantCulture, out s);
            deg = d + m / 60.0 + s / 3600.0;
            return true;
        }
    }

}
