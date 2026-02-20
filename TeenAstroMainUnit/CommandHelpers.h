/*
 * CommandHelpers.h - Shared utilities for command handlers.
 *
 * Provides:
 *   AxisRef   -- bundles references to one axis's motor, encoder, stepper,
 *                geo, and EEPROM offsets.  Eliminates D/R axis duplication.
 *   selectAxis()  -- returns an AxisRef* for 'R' (axis1) or 'D' (axis2).
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include "Command.h"

// ---------------------------------------------------------------------------
//  AxisRef -- one axis's objects + EEPROM offsets
// ---------------------------------------------------------------------------
struct AxisRef {
  MotorAxis&    motor;
  EncoderAxis&  encoder;
  StatusAxis&   stepper;
  GeoAxis&      geo;
  // EEPROM offsets
  int eeGear, eeStepRot, eeMicro, eeReverse, eeSilent;
  int eeHighCurr, eeLowCurr;
  int eeBacklashAmount, eeBacklashRate;
  int eePulsePerDegree, eeEncoderReverse;
};

// ---------------------------------------------------------------------------
//  selectAxis -- pick axis1 ('R') or axis2 ('D'), nullptr for anything else
// ---------------------------------------------------------------------------
inline AxisRef* selectAxis(char axisChar)
{
  static AxisRef axis1 = {
    mount.motorsEncoders.motorA1,
    mount.motorsEncoders.encoderA1,
    mount.axes.staA1,
    mount.axes.geoA1,
    EE_motorA1gear, EE_motorA1stepRot, EE_motorA1micro,
    EE_motorA1reverse, EE_motorA1silent,
    EE_motorA1highCurr, EE_motorA1lowCurr,
    EE_motorA1backlashAmount, EE_motorA1backlashRate,
    EE_encoderA1pulsePerDegree, EE_encoderA1reverse
  };
  static AxisRef axis2 = {
    mount.motorsEncoders.motorA2,
    mount.motorsEncoders.encoderA2,
    mount.axes.staA2,
    mount.axes.geoA2,
    EE_motorA2gear, EE_motorA2stepRot, EE_motorA2micro,
    EE_motorA2reverse, EE_motorA2silent,
    EE_motorA2highCurr, EE_motorA2lowCurr,
    EE_motorA2backlashAmount, EE_motorA2backlashRate,
    EE_encoderA2pulsePerDegree, EE_encoderA2reverse
  };
  if (axisChar == 'R') return &axis1;
  if (axisChar == 'D') return &axis2;
  return nullptr;
}
