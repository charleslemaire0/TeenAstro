/*
 * NativeInterop.cs - P/Invoke declarations for TeenAstroAscomNative.dll
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;
using System.Runtime.InteropServices;

namespace ASCOM.TeenAstro.Telescope
{
    /// <summary>
    /// P/Invoke bindings for TeenAstroAscomNative.dll.
    /// </summary>
    internal static class NativeInterop
    {
        private const string DllName = "TeenAstroAscomNative";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern IntPtr TeenAstroAscom_ConnectSerial(string port);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern IntPtr TeenAstroAscom_ConnectTcp(string ip, int port);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void TeenAstroAscom_Disconnect(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_CommandBlind(IntPtr handle, string cmd, int raw);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_CommandBool(IntPtr handle, string cmd, int raw);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_CommandString(IntPtr handle, string cmd, int raw,
            [Out] byte[] outBuf, int outBufSize);

        [StructLayout(LayoutKind.Sequential)]
        public struct GXASState
        {
            public int valid;
            public double rightAscensionHours;
            public double declinationDegrees;
            public double altitudeDegrees;
            public double azimuthDegrees;
            public double targetRAHours;
            public double targetDecDegrees;
            public double siderealTimeHours;
            public int tracking;
            public int slewing;
            public int atHome;
            public int parkState;
            public int pierSideWest;
            public int isPulseGuiding;
            public int utcYear;
            public int utcMonth;
            public int utcDay;
            public int utcHour;
            public int utcMin;
            public int utcSec;
        }

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_GetMountState(IntPtr handle, out GXASState outState);

        /* Semantic API - no raw command strings in C# */
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_AbortSlew(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_HasSite(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_HasMotors(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_Park(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_Unpark(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_SetPark(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_PulseGuide(IntPtr handle, int direction, int durationMs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_MoveAxis(IntPtr handle, int axis, double rateArcsecPerSec);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_SyncToEquatorial(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_SyncToAltAz(IntPtr handle);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_SlewToEquatorial(IntPtr handle, byte[] outReply, int outSize);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_SlewToAltAz(IntPtr handle, byte[] outReply, int outSize);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_SetTargetRA(IntPtr handle, double raHours);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_SetTargetDec(IntPtr handle, double decDeg);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_SetTargetAz(IntPtr handle, string azStr);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_SetTargetAlt(IntPtr handle, string altStr);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_GetSiteLatitude(IntPtr handle, out double outLat);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_GetSiteLongitude(IntPtr handle, out double outLon);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_SetSiteLatitude(IntPtr handle, string latStr);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int TeenAstroAscom_SetSiteLongitude(IntPtr handle, string lonStr);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_GetUTCTimestamp(IntPtr handle, out double outSecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_SetUTCTimestamp(IntPtr handle, long unixSecs);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int TeenAstroAscom_EnableTracking(IntPtr handle, int on);
    }
}
