/*
 * GXASDecoder.cs - Decode :GXAS# base64 reply to GXASState
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;

namespace ASCOM.TeenAstro.Telescope
{
    /// <summary>
    /// Decoded mount state from :GXAS# (88-char base64 -> 66 bytes).
    /// Layout matches TeenAstroMountStatus.cpp.
    /// </summary>
    internal struct GXASState
    {
        public int Valid;
        public double RightAscensionHours;
        public double DeclinationDegrees;
        public double AltitudeDegrees;
        public double AzimuthDegrees;
        public double TargetRAHours;
        public double TargetDecDegrees;
        public double SiderealTimeHours;
        public int Tracking;
        public int Slewing;
        public int AtHome;
        public int ParkState;
        public int PierSideWest;
        public int IsPulseGuiding;
        public int UtcYear;
        public int UtcMonth;
        public int UtcDay;
        public int UtcHour;
        public int UtcMin;
        public int UtcSec;
    }

    internal static class GXASDecoder
    {
        private static readonly int[] B64Dec = new int[128];

        static GXASDecoder()
        {
            for (int i = 0; i < 128; i++) B64Dec[i] = -1;
            for (int i = 0; i < 26; i++) B64Dec['A' + i] = i;
            for (int i = 0; i < 26; i++) B64Dec['a' + i] = 26 + i;
            for (int i = 0; i < 10; i++) B64Dec['0' + i] = 52 + i;
            B64Dec['+'] = 62;
            B64Dec['/'] = 63;
        }

        private static bool B64Decode(string input, int inLen, byte[] output)
        {
            if (inLen % 4 != 0 || output == null || output.Length < inLen / 4 * 3)
                return false;
            int o = 0;
            for (int i = 0; i < inLen; i += 4)
            {
                int c0 = input[i];
                int c1 = i + 1 < inLen ? input[i + 1] : 0;
                int c2 = i + 2 < inLen ? input[i + 2] : 0;
                int c3 = i + 3 < inLen ? input[i + 3] : 0;
                if (c0 > 127 || c1 > 127 || c2 > 127 || c3 > 127) return false;
                int v0 = c0 < 128 ? B64Dec[c0] : -1;
                int v1 = c1 < 128 ? B64Dec[c1] : -1;
                int v2 = c2 < 128 ? B64Dec[c2] : -1;
                int v3 = c3 < 128 ? B64Dec[c3] : -1;
                if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;
                uint b = (uint)((v0 << 18) | (v1 << 12) | (v2 << 6) | v3);
                output[o++] = (byte)(b >> 16);
                output[o++] = (byte)(b >> 8);
                output[o++] = (byte)b;
            }
            return true;
        }

        private static float ReadFloat32LE(byte[] pkt, int off)
        {
            if (pkt == null || off + 4 > pkt.Length)
                return 0;
            return BitConverter.ToSingle(pkt, off);
        }

        private static int ReadInt32LE(byte[] pkt, int off)
        {
            if (pkt == null || off + 4 > pkt.Length)
                return 0;
            return BitConverter.ToInt32(pkt, off);
        }

        /// <summary>Decode 88-char base64 string to GXASState. Returns false on error.</summary>
        public static bool TryDecode(string base64, out GXASState state)
        {
            state = default;
            if (string.IsNullOrEmpty(base64) || base64.Length != 88)
                return false;

            byte[] pkt = new byte[66];
            if (!B64Decode(base64, 88, pkt))
                return false;

            // XOR checksum: bytes 0-64 XOR must equal byte 65
            byte xorChk = 0;
            for (int i = 0; i < 65; i++)
                xorChk ^= pkt[i];
            if (xorChk != pkt[65])
                return false;

            // Status bytes 0-5
            byte b0 = pkt[0], b1 = pkt[1], b2 = pkt[2];
            state.Tracking = (b0 & 0x3) == 1 ? 1 : ((b0 & 0x3) >= 2) ? 1 : 0;  // TRK_ON or TRK_SLEWING -> 1
            state.Slewing = ((b0 & 0x3) == 2 || (b0 & 0x3) == 3) ? 1 : 0;
            int parkV = (b0 >> 4) & 0x3;
            state.ParkState = parkV == 2 ? 2 : parkV == 0 ? 0 : 1;  // 2=parked
            state.AtHome = ((b0 >> 6) & 0x1) != 0 ? 1 : 0;
            state.PierSideWest = ((b0 >> 7) & 0x1) != 0 ? 1 : 0;
            state.IsPulseGuiding = ((b2 >> 7) & 0x1) != 0 ? 1 : 0;

            // UTC 6-11
            state.UtcHour = pkt[6];
            state.UtcMin = pkt[7];
            state.UtcSec = pkt[8];
            state.UtcMonth = pkt[9];
            state.UtcDay = pkt[10];
            state.UtcYear = pkt[11] + 2000;

            // Positions 12-39 (7 x float32 LE)
            state.RightAscensionHours = ReadFloat32LE(pkt, 12);
            state.DeclinationDegrees = ReadFloat32LE(pkt, 16);
            state.AltitudeDegrees = ReadFloat32LE(pkt, 20);
            state.AzimuthDegrees = ReadFloat32LE(pkt, 24);
            state.SiderealTimeHours = ReadFloat32LE(pkt, 28);
            state.TargetRAHours = ReadFloat32LE(pkt, 32);
            state.TargetDecDegrees = ReadFloat32LE(pkt, 36);

            state.Valid = 1;
            return true;
        }
    }
}
