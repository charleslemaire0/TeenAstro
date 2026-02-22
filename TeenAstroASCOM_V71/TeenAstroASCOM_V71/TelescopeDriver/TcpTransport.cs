/*
 * TcpTransport.cs - TCP socket implementation of IX200Transport
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;
using System.IO;
using System.Net.Sockets;
using System.Net;

namespace ASCOM.TeenAstro.Telescope
{
    internal sealed class TcpTransport : IX200Transport
    {
        private TcpClient _client;
        private NetworkStream _stream;
        private byte[] _readBuffer = new byte[1];
        private int _timeoutMs = 5000;

        public bool IsOpen => _client != null && _client.Connected;

        public int Available
        {
            get
            {
                if (_client == null || !_client.Connected)
                    return 0;
                return _client.Available;
            }
        }

        public bool Open(string host, int port)
        {
            if (IsOpen)
                Close();
            try
            {
                _client = new TcpClient();
                _client.Connect(host, port);
                _client.ReceiveTimeout = _timeoutMs;
                _client.SendTimeout = _timeoutMs;
                _stream = _client.GetStream();
                return true;
            }
            catch
            {
                Close();
                return false;
            }
        }

        public void Close()
        {
            _stream?.Close();
            _stream = null;
            _client?.Close();
            _client = null;
        }

        public void Flush()
        {
            // TCP: no local buffer to flush; data already sent
        }

        public int Write(byte[] buf, int offset, int count)
        {
            if (_stream == null || buf == null)
                return 0;
            try
            {
                _stream.Write(buf, offset, count);
                return count;
            }
            catch
            {
                return 0;
            }
        }

        public int ReadByte()
        {
            if (_stream == null)
                return -1;
            try
            {
                int n = _stream.Read(_readBuffer, 0, 1);
                return n == 1 ? (int)_readBuffer[0] : -1;
            }
            catch (IOException)
            {
                return -1;
            }
        }

        public void SetTimeout(int timeoutMs)
        {
            _timeoutMs = timeoutMs;
            if (_client != null && _client.Connected)
            {
                _client.ReceiveTimeout = timeoutMs;
                _client.SendTimeout = timeoutMs;
            }
        }

        public void Dispose()
        {
            Close();
        }
    }
}
