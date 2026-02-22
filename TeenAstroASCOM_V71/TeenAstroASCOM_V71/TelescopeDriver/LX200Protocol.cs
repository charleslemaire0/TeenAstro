/*
 * LX200Protocol.cs - LX200 protocol layer for TeenAstro ASCOM driver
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;
using System.Text;

namespace ASCOM.TeenAstro.Telescope
{
    internal enum CmdReply
    {
        No,           // No reply expected
        Short,        // Single character reply
        ShortBool,    // Single character '0' or '1'
        Long,         // String terminated by '#'
        Invalid
    }

    internal sealed class LX200Protocol
    {
        private readonly IX200Transport _transport;
        private int _timeoutMs = 30;
        private const int DefaultTimeout = 30;

        public LX200Protocol(IX200Transport transport)
        {
            _transport = transport ?? throw new ArgumentNullException(nameof(transport));
        }

        public void SetTimeout(int ms) => _timeoutMs = ms;

        internal static CmdReply GetReplyType(string command)
        {
            if (string.IsNullOrEmpty(command))
                return CmdReply.Invalid;
            if (command[0] != ':')
                return CmdReply.Invalid;

            if (command.Length < 2)
                return CmdReply.Invalid;

            char c1 = command[1];
            char c2 = command.Length > 2 ? command[2] : '\0';

            switch (c1)
            {
                case 'A': // Alignment
                    if ("*0123456789CWA".IndexOf(c2) >= 0) return CmdReply.ShortBool;
                    if (c2 == 'E') return CmdReply.Long;
                    break;
                case 'C': // Sync
                    if ("AMU".IndexOf(c2) >= 0) return CmdReply.Long;
                    if (c2 == 'S') return CmdReply.No;
                    break;
                case 'G': // Get
                    if ("AaCcDdefgGhLMNOPmnoRrSTtVXWZ".IndexOf(c2) >= 0) return CmdReply.Long;
                    break;
                case 'h': // Home/Park
                    if ("BbCFOPQRS".IndexOf(c2) >= 0) return CmdReply.ShortBool;
                    break;
                case 'M': // Move
                    if ("ewnsg".IndexOf(c2) >= 0) return CmdReply.No;
                    if ("SAUF?".IndexOf(c2) >= 0) return CmdReply.Short;
                    if ("12@".IndexOf(c2) >= 0) return CmdReply.ShortBool;
                    break;
                case 'Q': // Halt
                    if ("#ewns".IndexOf(c2) >= 0) return CmdReply.No;
                    break;
                case 'S': // Set
                    if ("!aBCedgGhLmMnNoOrtTUXz".IndexOf(c2) >= 0) return CmdReply.ShortBool;
                    break;
                case 'T': // Tracking
                    if ("ed012".IndexOf(c2) >= 0) return CmdReply.ShortBool;
                    break;
            }

            // SX extended (TeenAstro)
            if (c1 == 'S' && c2 == 'X' && command.Length > 3)
                return CmdReply.ShortBool;
            return CmdReply.Invalid;
        }

        private void FlushInput()
        {
            while (_transport.Available > 0)
                _transport.ReadByte();
        }

        public bool SendReceive(string command, CmdReply replyType, byte[] recvBuffer, out int recvLen)
        {
            recvLen = 0;
            if (recvBuffer != null)
                Array.Clear(recvBuffer, 0, recvBuffer.Length);

            if (replyType == CmdReply.Invalid)
                return false;

            _transport.SetTimeout(_timeoutMs);
            _transport.Flush();
            FlushInput();

            byte[] cmdBytes = Encoding.ASCII.GetBytes(command);
            if (_transport.Write(cmdBytes, 0, cmdBytes.Length) != cmdBytes.Length)
                return false;

            int bufSize = recvBuffer?.Length ?? 0;
            var start = DateTime.UtcNow;

            switch (replyType)
            {
                case CmdReply.No:
                    return true;

                case CmdReply.Short:
                case CmdReply.ShortBool:
                    while ((DateTime.UtcNow - start).TotalMilliseconds < _timeoutMs)
                    {
                        if (_transport.Available > 0)
                        {
                            int b = _transport.ReadByte();
                            if (b >= 0 && recvBuffer != null && bufSize > 0)
                            {
                                recvBuffer[0] = (byte)b;
                                recvLen = 1;
                            }
                            return true;
                        }
                    }
                    return false;

                case CmdReply.Long:
                    int pos = 0;
                    bool hashFound = false;
                    while ((DateTime.UtcNow - start).TotalMilliseconds < _timeoutMs)
                    {
                        if (_transport.Available > 0)
                        {
                            int b = _transport.ReadByte();
                            if (b < 0) break;
                            if (b == '#')
                            {
                                hashFound = true;
                                break;
                            }
                            start = DateTime.UtcNow;
                            if (recvBuffer != null && pos < bufSize - 1)
                                recvBuffer[pos] = (byte)b;
                            pos++;
                        }
                    }
                    recvLen = pos;
                    return hashFound && pos >= 0;
            }

            return false;
        }

        public bool SendReceive(string command, byte[] recvBuffer, out int recvLen)
        {
            CmdReply rt = GetReplyType(command);
            if (rt == CmdReply.Invalid)
            {
                recvLen = 0;
                return false;
            }
            int to = _timeoutMs;
            if (command.StartsWith(":G"))
                to *= 2;
            int saved = _timeoutMs;
            _timeoutMs = to;
            bool ok = SendReceive(command, rt, recvBuffer, out recvLen);
            _timeoutMs = saved;
            return ok;
        }

        /// <summary>Get string response. Does not include #. Trims spaces for text commands.</summary>
        public bool Get(string command, byte[] output, out int length)
        {
            length = 0;
            if (output == null || output.Length == 0)
                return false;
            if (!SendReceive(command, output, out length))
                return false;

            bool isBinary = command == ":GXAS#" || command == ":GXCS#" || command == ":FA#" || command == ":Fa#";
            if (!isBinary)
            {
                // Trim leading spaces
                int start = 0;
                while (start < length && output[start] == (byte)' ')
                    start++;
                if (start > 0)
                {
                    Array.Copy(output, start, output, 0, length - start);
                    length -= start;
                }
                // Trim trailing spaces
                while (length > 0 && output[length - 1] == (byte)' ')
                    length--;
            }
            return length >= 0;
        }

        /// <summary>Send set command. Returns true if success (or 1 for ShortBool).</summary>
        public bool Set(string command)
        {
            byte[] buf = new byte[32];
            CmdReply rt = GetReplyType(command);
            if (rt == CmdReply.Invalid)
                return false;
            bool ok = SendReceive(command, rt, buf, out int len);
            if (!ok) return false;
            if (rt == CmdReply.ShortBool)
                return len > 0 && (char)buf[0] == '1';
            return true;
        }

        public int DefaultTimeoutMs => DefaultTimeout;
    }
}
