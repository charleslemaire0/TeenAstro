/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2024 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Revision History, see GitHub
 *
 * Description: Mount goto, sync, coordinate getters, predictTarget, goTo, flip. See Mount.h.
 */
#include "MainUnit.h"

// -----------------------------------------------------------------------------
// Step–angle conversion and axis sync (delegate to axes)
// -----------------------------------------------------------------------------

void Mount::stepToAngle(long Axis1, long Axis2, double* AngleAxis1, double* AngleAxis2, PoleSide* Side) const
{
  axes.stepToAngle(Axis1, Axis2, AngleAxis1, AngleAxis2, Side);
}

void Mount::angle2Step(double AngleAxis1, double AngleAxis2, PoleSide Side, long* Axis1, long* Axis2) const
{
  axes.angle2Step(AngleAxis1, AngleAxis2, Side, Axis1, Axis2);
}

void Mount::syncAxis(const long* axis1, const long* axis2)
{
  axes.syncAxis(axis1, axis2);
}

// -----------------------------------------------------------------------------
// Goto: set target and start slew (decay mode goto)
// -----------------------------------------------------------------------------

void Mount::gotoAxis(const long* axis1Target, const long* axis2Target)
{
  cli();
  tracking.movingTo = true;
  SetsiderealClockSpeed(tracking.siderealClockSpeed);
  axes.staA1.resetToSidereal();
  axes.staA2.resetToSidereal();
  axes.staA1.start = axes.staA1.pos;
  axes.staA2.start = axes.staA2.pos;
  axes.staA1.target = *axis1Target;
  axes.staA2.target = *axis2Target;
  sei();
  decayModeGoto();
}

// -----------------------------------------------------------------------------
// Sync: set mount position from instrument / equatorial / horizontal coords
// -----------------------------------------------------------------------------

bool Mount::syncInstr(Coord_IN* instr, PoleSide Side)
{
  long axis1, axis2 = 0;
  angle2Step(instr->Axis1() * RAD_TO_DEG, instr->Axis2() * RAD_TO_DEG, Side, &axis1, &axis2);
  syncAxis(&axis1, &axis2);
  syncEwithT();
  parkHome.atHome = false;
  return true;
}

bool Mount::syncEqu(Coord_EQ* EQ_T, PoleSide Side, double Lat)
{
  Coord_IN instr = EQ_T->To_Coord_IN(Lat, refrOptForGoto(), alignment.conv.Tinv);
  return syncInstr(&instr, Side);
}

bool Mount::syncAzAlt(Coord_HO* HO_T, PoleSide Side)
{
  Coord_IN instr = HO_T->To_Coord_IN(alignment.conv.Tinv);
  return syncInstr(&instr, Side);
}

// ----- Encoder ↔ stepper position sync -----

void Mount::syncTwithE()
{
  if (motorsEncoders.enableEncoder)
  {
    long axis1 = (long)(motorsEncoders.encoderA1.r_deg() * axes.geoA1.stepsPerDegree);
    long axis2 = (long)(motorsEncoders.encoderA2.r_deg() * axes.geoA2.stepsPerDegree);
    syncAxis(&axis1, &axis2);
    parkHome.atHome = false;
  }
}

void Mount::syncEwithT()
{
  if (motorsEncoders.enableEncoder)
  {
    long axis1, axis2;
    cli();
    axis1 = axes.staA1.pos;
    axis2 = axes.staA2.pos;
    sei();
    motorsEncoders.encoderA1.w_deg(axis1 / axes.geoA1.stepsPerDegree);
    motorsEncoders.encoderA2.w_deg(axis2 / axes.geoA2.stepsPerDegree);
  }
}

// ----- Auto-sync: if encoder and stepper differ beyond tolerance, snap stepper to encoder -----
bool Mount::autoSyncWithEncoder(EncoderSync mode)
{
  bool synced = false;
  if (motorsEncoders.enableEncoder)
  {
    static EncoderSync lastmode = ES_OFF;
    static double tol = 0.0;

    if (lastmode != mode)
    {
      switch (mode)
      {
      case ES_60:
        tol = 1.0;
        break;
      case ES_30:
        tol = 0.5;
        break;
      case ES_15:
        tol = 0.25;
        break;
      case ES_8:
        tol = 8. / 60.;
        break;
      case ES_4:
        tol = 4. / 60.;
        break;
      case ES_2:
        tol = 2. / 60.;
        break;
      case ES_ALWAYS:
        tol = 0.;
        break;
      case ES_OFF:
        break;
      default:
        break;
      }
      lastmode = mode;
    }
    if (mode == ES_OFF)
      return false;

    long axis1T, axis2T;
    cli();
    axis1T = axes.staA1.pos;
    axis2T = axes.staA2.pos;
    sei();
    long axis1E = (long)(motorsEncoders.encoderA1.r_deg() * axes.geoA1.stepsPerDegree);
    long axis2E = (long)(motorsEncoders.encoderA2.r_deg() * axes.geoA2.stepsPerDegree);
    double tol1 = max(tol, 1. / motorsEncoders.encoderA1.pulsePerDegree);
    if (abs(axis1T - axis1E) >= tol1 * axes.geoA1.stepsPerDegree)
    {
      cli();
      axes.staA1.pos = axis1E;
      axes.staA1.target = axes.staA1.pos;
      sei();
      synced = true;
    }
    double tol2 = max(tol, 1. / motorsEncoders.encoderA2.pulsePerDegree);
    if (abs(axis2T - axis2E) >= tol2 * axes.geoA2.stepsPerDegree)
    {
      cli();
      axes.staA2.pos = axis2E;
      axes.staA2.target = axes.staA2.pos;
      sei();
      synced = true;
    }
  }
  return synced;
}

// -----------------------------------------------------------------------------
// Coordinate getters: instrument (steps → IN), equatorial, horizontal
// -----------------------------------------------------------------------------

void Mount::getInstrDeg(double* A1, double* A2, double* A3) const
{
  axes.getInstrDeg(A1, A2, A3);
}

Coord_IN Mount::getInstr() const
{
  double Axis1, Axis2, Axis3;
  getInstrDeg(&Axis1, &Axis2, &Axis3);
  return Coord_IN(Axis3 * DEG_TO_RAD, Axis2 * DEG_TO_RAD, Axis1 * DEG_TO_RAD);
}

Coord_IN Mount::getInstrE() const
{
  return Coord_IN(0,
    motorsEncoders.encoderA2.r_deg() * DEG_TO_RAD,
    motorsEncoders.encoderA1.r_deg() * DEG_TO_RAD);
}

Coord_IN Mount::getInstrTarget() const
{
  return axes.getInstrTarget();
}

Coord_EQ Mount::getEqu(double Lat) const
{
  return getInstr().To_Coord_EQ(alignment.conv.T, refrOptForGoto(), Lat);
}

Coord_EQ Mount::getEquE(double Lat) const
{
  return getInstrE().To_Coord_EQ(alignment.conv.T, refrOptForGoto(), Lat);
}

Coord_EQ Mount::getEquTarget(double Lat) const
{
  return getInstrTarget().To_Coord_EQ(alignment.conv.T, refrOptForGoto(), Lat);
}

Coord_HO Mount::getHorTopo() const
{
  return getInstr().To_Coord_HO(alignment.conv.T, refrOptForTracking());
}

Coord_HO Mount::getHorETopo() const
{
  if (motorsEncoders.enableEncoder)
    return getInstrE().To_Coord_HO(alignment.conv.T, refrOptForTracking());
  return getHorTopo();
}

Coord_HO Mount::getHorAppTarget() const
{
  return getInstrTarget().To_Coord_HO(alignment.conv.T, refrOptForGoto());
}

// -----------------------------------------------------------------------------
// predictTarget: try preferred pole side, then flip if FLIP_ALWAYS
// -----------------------------------------------------------------------------

bool Mount::predictTarget(const double& Axis1_in, const double& Axis2_in, const PoleSide& inputSide,
  long& Axis1_out, long& Axis2_out, PoleSide& outputSide) const
{
  double Axis1 = Axis1_in;
  double Axis2 = Axis2_in;
  outputSide = inputSide;
  angle2Step(Axis1, Axis2, outputSide, &Axis1_out, &Axis2_out);
  if (limits.withinLimit(Axis1_out, Axis2_out))
    return true;
  if (config.identity.meridianFlip == FLIP_ALWAYS)
  {
    Axis1 = Axis1_in;
    Axis2 = Axis2_in;
    outputSide = (inputSide == POLE_UNDER) ? POLE_OVER : POLE_UNDER;
    angle2Step(Axis1, Axis2, outputSide, &Axis1_out, &Axis2_out);
    if (limits.withinLimit(Axis1_out, Axis2_out))
      return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// goToEqu / goToHor: validate alt limits, convert to IN, predictTarget, goTo
// -----------------------------------------------------------------------------

byte Mount::goToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat)
{
  return goToHor(EQ_T.To_Coord_HO(Lat, refrOptForGoto()), preferedPoleSide);
}

byte Mount::goToHor(Coord_HO HO_T, PoleSide preferedPoleSide)
{
  double Axis1_target = 0.0, Axis2_target = 0.0;
  long axis1_target, axis2_target = 0;
  PoleSide selectedSide = PoleSide::POLE_NOTVALID;

  if (HO_T.Alt() * RAD_TO_DEG < limits.minAlt)
    return ERRGOTO_BELOWHORIZON;
  if (HO_T.Alt() * RAD_TO_DEG > limits.maxAlt)
    return ERRGOTO_ABOVEOVERHEAD;

  Coord_IN instr_T = HO_T.To_Coord_IN(alignment.conv.Tinv);
  Axis1_target = instr_T.Axis1() * RAD_TO_DEG;
  Axis2_target = instr_T.Axis2() * RAD_TO_DEG;
  if (!predictTarget(Axis1_target, Axis2_target, preferedPoleSide, axis1_target, axis2_target, selectedSide))
    return ERRGOTO_LIMITS;

  return (byte)goTo(axis1_target, axis2_target);
}

// ----- goTo: checks (motor, slewing, guiding, park, lastError) then gotoAxis -----
ErrorsGoTo Mount::goTo(long thisTargetAxis1, long thisTargetAxis2)
{
  if (!motorsEncoders.enableMotor)
    return ErrorsGoTo::ERRGOTO_MOTOR_FAULT;
  if (tracking.movingTo)
  {
    abortSlew();
    return ErrorsGoTo::ERRGOTO_SLEWING;
  }
  if (guiding.guideA1.isBusy() || guiding.guideA2.isBusy())
    return ErrorsGoTo::ERRGOTO_GUIDINGBUSY;
  if ((parkHome.parkStatus != PRK_UNPARKED) && (parkHome.parkStatus != PRK_PARKING))
    return ErrorsGoTo::ERRGOTO_PARKED;
  if (errors.lastError != ERRT_NONE)
    return static_cast<ErrorsGoTo>(errors.lastError + 10);

  parkHome.atHome = false;
  gotoAxis(&thisTargetAxis1, &thisTargetAxis2);
  return ErrorsGoTo::ERRGOTO_NONE;
}

// ----- flip: current position → other pier side via predictTarget, then goTo -----
ErrorsGoTo Mount::flip()
{
  long Axis1, Axis2, axis1Flip, axis2Flip;
  double Angle1, Angle2;
  PoleSide selectedSide = POLE_NOTVALID;
  PoleSide CurrentSide = POLE_NOTVALID;

  limits.getAxisPositions(Axis1, Axis2);
  stepToAngle(Axis1, Axis2, &Angle1, &Angle2, &CurrentSide);
  PoleSide preferedPoleSide = (CurrentSide == POLE_UNDER) ? POLE_OVER : POLE_UNDER;
  if (!predictTarget(Angle1, Angle2, preferedPoleSide, axis1Flip, axis2Flip, selectedSide))
    return ErrorsGoTo::ERRGOTO_LIMITS;
  if (selectedSide == getPoleSide())
    return ErrorsGoTo::ERRGOTO_SAMESIDE;
  return goTo(axis1Flip, axis2Flip);
}
