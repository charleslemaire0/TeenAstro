/*
 * IX200Transport.cs - Abstract transport for LX200 protocol
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;

namespace ASCOM.TeenAstro.Telescope
{
    /// <summary>
    /// Abstract transport for LX200 serial/TCP communication.
    /// </summary>
    internal interface IX200Transport : IDisposable
    {
        void Flush();
        int Write(byte[] buf, int offset, int count);
        int Available { get; }
        int ReadByte();
        void SetTimeout(int timeoutMs);
        bool IsOpen { get; }
    }
}
