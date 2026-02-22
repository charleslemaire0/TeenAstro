/*
 * GXASStateWrapper.cs - Wrapper for native GXAS state, replacing GXASStateParser
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */

using System;

namespace ASCOM.TeenAstro.Telescope
{
    /// <summary>
    /// Wraps the native GXAS state struct for use by TelescopeHardware.
    /// Replaces the former GXASStateParser.
    /// </summary>
    internal class GXASStateWrapper
    {
        private readonly NativeInterop.GXASState _state;

        public GXASStateWrapper(NativeInterop.GXASState state)
        {
            _state = state;
        }

        public bool Valid => _state.valid != 0;
        public double RightAscensionHours => _state.rightAscensionHours;
        public double DeclinationDegrees => _state.declinationDegrees;
        public double AltitudeDegrees => _state.altitudeDegrees;
        public double AzimuthDegrees => _state.azimuthDegrees;
        public double TargetRAHours => _state.targetRAHours;
        public double TargetDecDegrees => _state.targetDecDegrees;
        public double SiderealTimeHours => _state.siderealTimeHours;
        public bool Tracking => _state.tracking != 0;
        public bool Slewing => _state.slewing != 0;
        public bool AtHome => _state.atHome != 0;
        public int ParkState => _state.parkState;
        public bool PierSideWest => _state.pierSideWest != 0;
        public bool IsPulseGuiding => _state.isPulseGuiding != 0;

        public DateTime GetUTCDate()
        {
            return new DateTime(_state.utcYear, _state.utcMonth, _state.utcDay,
                _state.utcHour, _state.utcMin, _state.utcSec, DateTimeKind.Utc);
        }
    }
}
