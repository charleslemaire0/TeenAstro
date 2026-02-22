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
        private readonly GXASState _state;

        public GXASStateWrapper(GXASState state)
        {
            _state = state;
        }

        public bool Valid => _state.Valid != 0;
        public double RightAscensionHours => _state.RightAscensionHours;
        public double DeclinationDegrees => _state.DeclinationDegrees;
        public double AltitudeDegrees => _state.AltitudeDegrees;
        public double AzimuthDegrees => _state.AzimuthDegrees;
        public double TargetRAHours => _state.TargetRAHours;
        public double TargetDecDegrees => _state.TargetDecDegrees;
        public double SiderealTimeHours => _state.SiderealTimeHours;
        public bool Tracking => _state.Tracking != 0;
        public bool Slewing => _state.Slewing != 0;
        public bool AtHome => _state.AtHome != 0;
        public int ParkState => _state.ParkState;
        public bool PierSideWest => _state.PierSideWest != 0;
        public bool IsPulseGuiding => _state.IsPulseGuiding != 0;

        public DateTime GetUTCDate()
        {
            return new DateTime(_state.UtcYear, _state.UtcMonth, _state.UtcDay,
                _state.UtcHour, _state.UtcMin, _state.UtcSec, DateTimeKind.Utc);
        }
    }
}
