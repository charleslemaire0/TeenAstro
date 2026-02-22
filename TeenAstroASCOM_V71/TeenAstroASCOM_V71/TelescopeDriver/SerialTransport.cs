/*
 * SerialTransport.cs - Serial port implementation of IX200Transport
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;
using System.IO.Ports;

namespace ASCOM.TeenAstro.Telescope
{
    internal sealed class SerialTransport : IX200Transport
    {
        private SerialPort _port;
        private int _timeoutMs = 5000;

        public bool IsOpen => _port != null && _port.IsOpen;

        public int Available => _port?.BytesToRead ?? 0;

        public bool Open(string portName, int baudRate = 57600)
        {
            if (IsOpen)
                Close();
            try
            {
                _port = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One);
                _port.ReadTimeout = _timeoutMs;
                _port.WriteTimeout = _timeoutMs;
                _port.Open();
                _port.DiscardInBuffer();
                _port.DiscardOutBuffer();
                return true;
            }
            catch
            {
                _port?.Dispose();
                _port = null;
                return false;
            }
        }

        public void Close()
        {
            _port?.Close();
            _port?.Dispose();
            _port = null;
        }

        public void Flush()
        {
            _port?.DiscardInBuffer();
            _port?.DiscardOutBuffer();
        }

        public int Write(byte[] buf, int offset, int count)
        {
            if (_port == null || !_port.IsOpen || buf == null)
                return 0;
            try
            {
                _port.Write(buf, offset, count);
                return count;
            }
            catch
            {
                return 0;
            }
        }

        public int ReadByte()
        {
            if (_port == null || !_port.IsOpen)
                return -1;
            try
            {
                return _port.ReadByte();
            }
            catch (TimeoutException)
            {
                return -1;
            }
        }

        public void SetTimeout(int timeoutMs)
        {
            _timeoutMs = timeoutMs;
            if (_port != null && _port.IsOpen)
            {
                _port.ReadTimeout = timeoutMs;
                _port.WriteTimeout = timeoutMs;
            }
        }

        public void Dispose()
        {
            Close();
        }
    }
}
