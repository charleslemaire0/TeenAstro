/**
 * GX — TeenAstro-specific get commands (:GXnn#).
 * Split from Command_G.cpp for clarity.  Called from Command_G() via case 'X'.
 */
#include "Command.h"
#include "CommandHelpers.h"
#include "ValueToString.h"

// ----- Reply formatters for coordinates -----
static void PrintAtitude(double& val)
{
  doubleToDms(commandState.reply, &val, false, true, commandState.highPrecision);
  strcat(commandState.reply, "#");
}
static void PrintAzimuth(double& val) {
  val = AzRange(val);
  doubleToDms(commandState.reply, &val, true, false, commandState.highPrecision);
  strcat(commandState.reply, "#");
}

static void PrintDec(double& val) {
  doubleToDms(commandState.reply, &val, false, true, commandState.highPrecision);
  strcat(commandState.reply, "#");
}

static void PrintRa(double& val) {
  doubleToHms(commandState.reply, &val, commandState.highPrecision);
  strcat(commandState.reply, "#");
}

// =============================================================================
//   GX Sub-handlers  (static -- called only from Command_GX)
// =============================================================================

// ---- GX Alignment  :GXAn# --------------------------------------------------
static void Command_GX_Alignment()
{
  float t11 = 0.f, t12 = 0.f, t13 = 0.f, t21 = 0.f, t22 = 0.f, t23 = 0.f, t31 = 0.f, t32 = 0.f, t33 = 0.f;
  if (mount.alignment.hasValid)
  {
    mount.alignment.conv.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
  }
  switch (commandState.command[3])
  {
  case '0': sprintf(commandState.reply, "%f#", t11); break;
  case '1': sprintf(commandState.reply, "%f#", t12); break;
  case '2': sprintf(commandState.reply, "%f#", t13); break;
  case '3': sprintf(commandState.reply, "%f#", t21); break;
  case '4': sprintf(commandState.reply, "%f#", t22); break;
  case '5': sprintf(commandState.reply, "%f#", t23); break;
  case '6': sprintf(commandState.reply, "%f#", t31); break;
  case '7': sprintf(commandState.reply, "%f#", t32); break;
  case '8': sprintf(commandState.reply, "%f#", t33); break;
  default:  replyLongUnknow(); break;
  }
}

// ---- GX Encoders  :GXEn# ---------------------------------------------------
static void Command_GX_Encoders()
{
  switch (commandState.command[3])
  {
  case '1':
  case '2':
    // :GXE1# / :GXE2# get degree encoder 1 / 2
  {
    double f1 = commandState.command[3] == '1' ? mount.motorsEncoders.encoderA1.r_deg() : mount.motorsEncoders.encoderA2.r_deg();
    doubleToDms(commandState.reply, &f1, true, true, commandState.highPrecision);
    strcat(commandState.reply, "#");
  }
  break;
  case 'A':
  case 'Z':
    // :GXEA# / :GXEZ#  get encoder altitude / azimuth
  {
    double f, f1;
    if (mount.motorsEncoders.enableEncoder)
    {
      Coord_HO HO_T = mount.getHorETopo();
      f = degRange(HO_T.Az() * RAD_TO_DEG);
      f1 = HO_T.Alt() * RAD_TO_DEG;
    }
    else
    {
      Coord_HO HO_T = mount.getHorTopo();
      f = degRange(HO_T.Az() * RAD_TO_DEG);
      f1 = HO_T.Alt() * RAD_TO_DEG;
    }
    commandState.command[3] == 'A' ? PrintAtitude(f1) : PrintAzimuth(f);
  }
  break;
  case 'D':
  case 'R':
  {
    //  :GXED# / :GXER#  Get Telescope Encoder Dec / RA
    Coord_EQ EQ_T = mount.motorsEncoders.enableEncoder ?
      mount.getEquE(*localSite.latitude() * DEG_TO_RAD) :
      mount.getEqu(*localSite.latitude() * DEG_TO_RAD);

    if (commandState.command[3] == 'R')
    {
      double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
      PrintRa(f);
    }
    else
    {
      double f1 = EQ_T.Dec() * RAD_TO_DEG;
      PrintDec(f1);
    }
  }
  break;
  case 'O':
    // :GXEO#  get encoder Sync Option
    sprintf(commandState.reply, "%u#", mount.motorsEncoders.EncodeSyncMode);
    break;
  case 'P':
  {
    // :GXEPn#   Get Encoder pulse per 100 deg
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%lu#", (unsigned long)(100.0 * ax->encoder.pulsePerDegree));
    else    replyLongUnknow();
  }
  break;
  case 'r':
  {
    // :GXErn#   Get Encoder reverse Rotation on/off
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", (unsigned int)ax->encoder.reverse);
    else    replyLongUnknow();
  }
  break;
  default:
    replyLongUnknow();
    break;
  }
}

// ---- GX Debug  :GXDn# ------------------------------------------------------
static void Command_GX_Debug()
{
  switch (commandState.command[3])
  {
  case 'B':
  {
    // :GXDBn# Debug Backlash
    switch (commandState.command[4])
    {
    case '0': sprintf(commandState.reply, "%d#", (int)mount.axes.staA1.backlash_correcting); break;
    case '1': sprintf(commandState.reply, "%d#", (int)mount.axes.staA2.backlash_correcting); break;
    case '2': sprintf(commandState.reply, "%d#", (int)mount.axes.staA1.backlash_movedSteps); break;
    case '3': sprintf(commandState.reply, "%d#", (int)mount.axes.staA2.backlash_movedSteps); break;
    default:  replyLongUnknow(); break;
    }
    break;
  }
  case 'R':
  {
    // :GXDRn# Debug Rates
    switch (commandState.command[4])
    {
    case '1': sprintf(commandState.reply, "%f#", mount.axes.staA1.RequestedTrackingRate); break;
    case '2': sprintf(commandState.reply, "%f#", mount.axes.staA2.RequestedTrackingRate); break;
    case '3': sprintf(commandState.reply, "%f#", (double)mount.axes.staA1.CurrentTrackingRate); break;
    case '4': sprintf(commandState.reply, "%f#", (double)mount.axes.staA2.CurrentTrackingRate); break;
    case '5': sprintf(commandState.reply, "%f#", (double)mount.axes.staA1.fstep); break;
    case '6': sprintf(commandState.reply, "%f#", (double)mount.axes.staA2.fstep); break;
    default:  replyLongUnknow(); break;
    }
    break;
  }
  case 'P':
  {
    // :GXDPn# Debug Position and Target
    long temp;
    switch (commandState.command[4])
    {
    case '0': cli(); temp = mount.axes.staA1.pos;    sei(); sprintf(commandState.reply, "%ld#", temp); break;
    case '1': cli(); temp = mount.axes.staA2.pos;    sei(); sprintf(commandState.reply, "%ld#", temp); break;
    case '2': cli(); temp = mount.axes.staA1.target;  sei(); sprintf(commandState.reply, "%ld#", temp); break;
    case '3': cli(); temp = mount.axes.staA2.target;  sei(); sprintf(commandState.reply, "%ld#", temp); break;
    case '4':
      mount.axes.staA1.updateDeltaTarget();
      sprintf(commandState.reply, "%ld#", mount.axes.staA1.deltaTarget); break;
    case '5':
      mount.axes.staA2.updateDeltaTarget();
      sprintf(commandState.reply, "%ld#", mount.axes.staA2.deltaTarget); break;
    case '6': sprintf(commandState.reply, "%lf#", mount.axes.staA1.interval_Step_Sid); break;
    case '7': sprintf(commandState.reply, "%lf#", mount.axes.staA2.interval_Step_Sid); break;
    default:  replyLongUnknow(); break;
    }
    break;
  }
  case 'W':
  {
    // :GXDW# Get workload
    if (commandState.command[4] == 0)
    {
      sprintf(commandState.reply, "%ld%%#", (tlp.getWorstTime() * 100L) / 9970L);
      tlp.resetWorstTime();
    }
    else
      replyLongUnknow();
    break;
  }
  break;
  default:
    replyLongUnknow();
    break;
  }
}

// ---- GX Position  :GXPn# ---------------------------------------------------
static void Command_GX_Position()
{
  switch (commandState.command[3])
  {
  case '1':
  {
    cli();
    double f1 = mount.axes.staA1.pos / mount.axes.geoA1.stepsPerDegree;
    sei();
    doubleToDms(commandState.reply, &f1, true, true, commandState.highPrecision);
    strcat(commandState.reply, "#");
  }
  break;
  case '2':
  {
    cli();
    double f1 = mount.axes.staA2.pos / mount.axes.geoA2.stepsPerDegree;
    sei();
    doubleToDms(commandState.reply, &f1, true, true, commandState.highPrecision);
    strcat(commandState.reply, "#");
  }
  break;
  case '3':
  case '4':
  {
    Coord_IN IN_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD).To_Coord_IN(*localSite.latitude() * DEG_TO_RAD, mount.refrOptForGoto(), mount.alignment.conv.Tinv);
    double f = IN_T.Axis1() * RAD_TO_DEG;
    double f1 = IN_T.Axis2() * RAD_TO_DEG;
    long Axis1_out, Axis2_out;
    mount.angle2Step(f, f1, mount.getPoleSide(), &Axis1_out, &Axis2_out);
    f = Axis1_out / mount.axes.geoA1.stepsPerDegree;
    f1 = Axis2_out / mount.axes.geoA2.stepsPerDegree;
    commandState.command[3] == '3' ? doubleToDms(commandState.reply, &f, true, true, commandState.highPrecision) : doubleToDms(commandState.reply, &f1, true, true, commandState.highPrecision);
    strcat(commandState.reply, "#");
  }
  break;
  default:
    replyLongUnknow();
    break;
  }
}

// ---- GX Refraction  :GXrn# -------------------------------------------------
static void Command_GX_Refraction()
{
  switch (commandState.command[3])
  {
  case 'p': mount.refraction.forPole     ? sprintf(commandState.reply, "y#") : sprintf(commandState.reply, "n#"); break;
  case 'g': mount.refraction.forGoto     ? sprintf(commandState.reply, "y#") : sprintf(commandState.reply, "n#"); break;
  case 't': mount.refraction.forTracking ? sprintf(commandState.reply, "y#") : sprintf(commandState.reply, "n#"); break;
  default:  replyLongUnknow(); break;
  }
}

// ---- GX Rates  :GXRn# ------------------------------------------------------
static void Command_GX_Rates()
{
  switch (commandState.command[3])
  {
  case '0': case '1': case '2': case '3':
  {
    int i = commandState.command[3] - '0';
    dtostrf(mount.guiding.guideRates[i], 2, 2, commandState.reply);
    strcat(commandState.reply, "#");
  }
  break;
  case 'A':
    dtostrf(mount.guiding.DegreesForAcceleration, 2, 1, commandState.reply);
    strcat(commandState.reply, "#");
    break;
  case 'B':
    sprintf(commandState.reply, "%ld#", (long)round(mount.axes.staA1.takeupRate));
    break;
  case 'D':
    sprintf(commandState.reply, "%d#", XEEPROM.read(getMountAddress(EE_DefaultRate)));
    break;
  case 'X':
    sprintf(commandState.reply, "%d#", XEEPROM.readUShort(getMountAddress(EE_maxRate)));
    break;
  case 'r':
  {
    long l1 = 10000l - (long)(mount.tracking.RequestedTrackingRateHA * 10000.0);
    sprintf(commandState.reply, "%ld#", l1);
  }
  break;
  case 'h':
  {
    long l1 = mount.tracking.RequestedTrackingRateHA * 10000.0;
    sprintf(commandState.reply, "%ld#", l1);
  }
  break;
  case 'd':
  {
    long l1 = mount.tracking.RequestedTrackingRateDEC * 10000.0;
    sprintf(commandState.reply, "%ld#", l1);
  }
  break;
  case 'e':
    sprintf(commandState.reply, "%ld#", mount.tracking.storedTrakingRateRA);
    break;
  case 'f':
    sprintf(commandState.reply, "%ld#", mount.tracking.storedTrakingRateDEC);
    break;
  default:
    replyLongUnknow();
    break;
  }
}

// ---- GX Limits  :GXLn# -----------------------------------------------------
static void Command_GX_Limits()
{
  switch (commandState.command[3])
  {
  case 'A': { int i = XEEPROM.readShort(getMountAddress(EE_minAxis1)); sprintf(commandState.reply, "%d#", i); } break;
  case 'B': { int i = XEEPROM.readShort(getMountAddress(EE_maxAxis1)); sprintf(commandState.reply, "%d#", i); } break;
  case 'C': { int i = XEEPROM.readShort(getMountAddress(EE_minAxis2)); sprintf(commandState.reply, "%d#", i); } break;
  case 'D': { int i = XEEPROM.readShort(getMountAddress(EE_maxAxis2)); sprintf(commandState.reply, "%d#", i); } break;
  case 'E': sprintf(commandState.reply, "%ld#", (long)round(mount.limits.getMeridianEastLimit())); break;
  case 'W': sprintf(commandState.reply, "%ld#", (long)round(mount.limits.getMeridianWestLimit())); break;
  case 'U': sprintf(commandState.reply, "%ld#", (long)round(mount.limits.underPoleLimitGOTO * 10)); break;
  case 'O': sprintf(commandState.reply, "%+02d*#", mount.limits.maxAlt); break;
  case 'H': sprintf(commandState.reply, "%+02d*#", mount.limits.minAlt); break;
  case 'S': sprintf(commandState.reply, "%02d*#", mount.limits.distanceFromPoleToKeepTrackingOn); break;
  default:  replyLongUnknow(); break;
  }
}

// ---- GX Mount-type Limits  :GXln# ------------------------------------------
static void Command_GX_MountLimits()
{
  switch (commandState.command[3])
  {
  case 'A': { int i = mount.axes.geoA1.LimMinAxis; sprintf(commandState.reply, "%d#", i); } break;
  case 'B': { int i = mount.axes.geoA1.LimMaxAxis; sprintf(commandState.reply, "%d#", i); } break;
  case 'C': { int i = mount.axes.geoA2.LimMinAxis; sprintf(commandState.reply, "%d#", i); } break;
  case 'D': { int i = mount.axes.geoA2.LimMaxAxis; sprintf(commandState.reply, "%d#", i); } break;
  default:  replyLongUnknow(); break;
  }
}

// ---- GX Time  :GXTn# -------------------------------------------------------
static void Command_GX_Time()
{
  switch (commandState.command[3])
  {
  case '0':
    doubleToHms(commandState.reply, rtk.getUT(), true);
    strcat(commandState.reply, "#");
    break;
  case '1':
  {
    int i, i1, i2, i3, i4, i5;
    rtk.getUTDate(i, i1, i2, i3, i4, i5);
    i = i % 100;
    sprintf(commandState.reply, "%02d/%02d/%02d#", i1, i2, i);
    break;
  }
  case '2':
  {
    unsigned long t = rtk.getTimeStamp();
    sprintf(commandState.reply, "%lu#", t);
  }
  break;
  case '3':
  {
    double tmpLST;
    Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
    tmpLST = EQ_T.Ha() * RAD_TO_HOUR;
    doubleToHms(commandState.reply, &tmpLST, true);
    strcat(commandState.reply, "#");
  }
  break;
  }
}

// ---- GX Status  :GXI# ------------------------------------------------------
static void Command_GX_Status()
{
  PoleSide currentSide = mount.getPoleSide();
  for (int i = 0; i < REPLY_BUFFER_LEN; i++)
    commandState.reply[i] = ' ';
  commandState.reply[0] = '0' + 2 * mount.tracking.movingTo + mount.tracking.sideralTracking;
  commandState.reply[1] = '0' + mount.tracking.sideralMode;
  const char* parkStatusCh = "pIPF";
  commandState.reply[2] = parkStatusCh[mount.parkHome.parkStatus];
  if (mount.isAtHome()) commandState.reply[3] = 'H';
  commandState.reply[4] = '0' + mount.guiding.recenterGuideRate;
  if (mount.tracking.doSpiral) commandState.reply[5] = '@';
  else if (mount.guiding.GuidingState != GuidingOFF)
  {
    commandState.reply[5] = 'G';
  }
  if (mount.isGuidingStar()) commandState.reply[6] = '*';
  else if (mount.guiding.GuidingState == GuidingRecenter) commandState.reply[6] = '+';
  else if (mount.guiding.GuidingState == GuidingAtRate) commandState.reply[6] = '-';
  if (mount.guiding.guideA1.isMFW()) commandState.reply[7] = '>';
  else if (mount.guiding.guideA1.isMBW()) commandState.reply[7] = '<';
  else if (mount.guiding.guideA1.isBraking()) commandState.reply[7] = 'b';

  if (currentSide == POLE_OVER)
  {
    if (mount.guiding.guideA2.isMBW()) commandState.reply[8] = '^';
    else if (mount.guiding.guideA2.isMFW()) commandState.reply[8] = '_';
    else if (mount.guiding.guideA2.isBraking()) commandState.reply[8] = 'b';
  }
  else
  {
    if (mount.guiding.guideA2.isMFW()) commandState.reply[8] = '^';
    else if (mount.guiding.guideA2.isMBW()) commandState.reply[8] = '_';
    else if (mount.guiding.guideA2.isBraking()) commandState.reply[8] = 'b';
  }

  if (mount.axes.staA1.fault || mount.axes.staA2.fault) commandState.reply[9] = 'f';
  commandState.reply[10] = '0' + mount.tracking.trackComp;
  commandState.reply[11] = mount.alignment.hasValid ? '1' : '0';
  if (mount.config.identity.mountType == MOUNT_TYPE_GEM)        commandState.reply[12] = 'E';
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK)  commandState.reply[12] = 'K';
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK_ALT) commandState.reply[12] = 'k';
  else if (mount.config.identity.mountType == MOUNT_TYPE_ALTAZM) commandState.reply[12] = 'A';
  else commandState.reply[12] = 'U';

  if (currentSide == POLE_UNDER) commandState.reply[13] = mount.isAltAZ() || localSite.northHemisphere() ? 'E' : 'W';
  if (currentSide == POLE_OVER) commandState.reply[13] = mount.isAltAZ() || localSite.northHemisphere() ? 'W' : 'E';

  char val = 0;
  bitWrite(val, 0, mount.config.peripherals.hasGNSS);
  if (iSGNSSValid())
  {
    bitWrite(val, 1, true);
    bitWrite(val, 2, isTimeSyncWithGNSS());
    bitWrite(val, 3, isLocationSyncWithGNSS());
    bitWrite(val, 4, isHdopSmall());
  }
  commandState.reply[14] = 'A' + val;
  commandState.reply[15] = '0' + mount.errors.lastError;
  val = 0;
  bitWrite(val, 0, mount.motorsEncoders.enableEncoder);
  bitWrite(val, 1, mount.motorsEncoders.encoderA1.calibrating() && mount.motorsEncoders.encoderA2.calibrating());
  bitWrite(val, 2, mount.config.peripherals.PushtoStatus != PT_OFF);
  bitWrite(val, 3, mount.motorsEncoders.enableMotor);
  commandState.reply[16] = 'A' + val;
  commandState.reply[17] = '#';
  commandState.reply[18] = 0;
}

// ---- GX ASCOM  :GXJn# ------------------------------------------------------
static void Command_GX_ASCOM()
{
  switch (commandState.command[3])
  {
  case 'B':
    mount.tracking.trackComp == TC_BOTH ? replyLongTrue() : replyLongFalse();
    break;
  case 'C':
    replyLongTrue();
    break;
  case 'm':
    mount.motorsEncoders.enableMotor ? replyLongTrue() : replyLongFalse();
    break;
  case 'M':
  {
    if (commandState.command[4] == '1')
      mount.guiding.GuidingState == Guiding::GuidingAtRate && mount.guiding.guideA1.isBusy() ? replyLongTrue() : replyLongFalse();
    else if (commandState.command[4] == '2')
      mount.guiding.GuidingState == Guiding::GuidingAtRate && mount.guiding.guideA2.isBusy() ? replyLongTrue() : replyLongFalse();
    else
      replyLongUnknow();
    break;
  }
  case 'P':
    mount.isGuidingStar() ? replyLongTrue() : replyLongFalse();
    break;
  case 'S':
    (mount.guiding.GuidingState == GuidingRecenter || mount.guiding.GuidingState == GuidingAtRate || mount.tracking.movingTo)
      ? replyLongTrue() : replyLongFalse();
    break;
  case 'T':
    mount.tracking.sideralTracking ? replyLongTrue() : replyLongFalse();
    break;
  }
}

// ---- GX Motors  :GXMn# -----------------------------------------------------
static void Command_GX_Motors()
{
  switch (commandState.command[3])
  {
  case 'B':
  {
    // :GXMB.#   Get Motor backlash
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%d#", ax->motor.backlashAmount);
    else    replyLongUnknow();
  }
  break;
  case 'b':
  {
    // :GXMb.#   Get Motor backlash Rate
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%d#", ax->motor.backlashRate);
    else    replyLongUnknow();
  }
  break;
  case 'G':
  {
    // :GXMG.#   Get Motor Gear
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%lu#", ax->motor.gear);
    else    replyLongUnknow();
  }
  break;
  case 'S':
  {
    // :GXMS.#   Get Stepper Step per Rotation
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", ax->motor.stepRot);
    else    replyLongUnknow();
  }
  break;
  case 'M':
  {
    // :GXMM.#   Get Stepper MicroStep per step
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", (unsigned int)ax->motor.micro);
    else    replyLongUnknow();
  }
  break;
  case 'm':
  {
    // :GXMm.#   Get Stepper Silent mode on/off
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", (unsigned int)ax->motor.silent);
    else    replyLongUnknow();
  }
  break;
  case 'R':
  {
    // :GXMR.#   Get Motor Reverse Rotation on/off
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", (unsigned int)ax->motor.reverse);
    else    replyLongUnknow();
  }
  break;
  case 'C':
  {
    // :GXMC.#   Get Motor HighCurrent in mA
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", ax->motor.highCurr);
    else    replyLongUnknow();
  }
  break;
  case 'c':
  {
    // :GXMc.#   Get Motor LowCurrent in mA
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%u#", (unsigned int)ax->motor.lowCurr);
    else    replyLongUnknow();
  }
  break;
  case 'L':
  {
    // :GXML.#   Get Stall Guard
    AxisRef* ax = selectAxis(commandState.command[4]);
    if (ax) sprintf(commandState.reply, "%d#", ax->motor.driver.getSG());
    else    replyLongUnknow();
  }
  break;
  case 'I':
  {
    // :GXMI.#   Get actual current
    // NOTE: original had D->motorA1, R->motorA2 (swapped) -- preserved as-is
    if (commandState.command[4] == 'D')
      sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA1.driver.getCurrent());
    else if (commandState.command[4] == 'R')
      sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA2.driver.getCurrent());
    else
      replyLongUnknow();
  }
  break;
  default:
    replyLongUnknow();
    break;
  }
}

// ---- GX Options  :GXOn# ----------------------------------------------------
static void Command_GX_Options()
{
  switch (commandState.command[3])
  {
  case 'I': sprintf(commandState.reply, "%d#", midx); break;
  case 'A': sprintf(commandState.reply, "%s#", mount.config.identity.mountName[midx]); break;
  case 'B': sprintf(commandState.reply, "%s#", mount.config.identity.mountName[0]); break;
  case 'C': sprintf(commandState.reply, "%s#", mount.config.identity.mountName[1]); break;
  case 'S':
  {
    int i = XEEPROM.read(getMountAddress(EE_SlewSettleDuration));
    sprintf(commandState.reply, "%d#", i);
  }
  break;
  }
}

// =============================================================================
//   Command_GX  --  :GXnn#  dispatch to sub-handlers
// =============================================================================
void Command_GX() {
  switch (commandState.command[2])
  {
  case 'A': Command_GX_Alignment();   break;
  case 'E': Command_GX_Encoders();    break;
  case 'D': Command_GX_Debug();       break;
  case 'P': Command_GX_Position();    break;
  case 'r': Command_GX_Refraction();  break;
  case 'R': Command_GX_Rates();       break;
  case 'L': Command_GX_Limits();      break;
  case 'l': Command_GX_MountLimits(); break;
  case 'T': Command_GX_Time();        break;
  case 'I': Command_GX_Status();      break;
  case 'J': Command_GX_ASCOM();       break;
  case 'M': Command_GX_Motors();      break;
  case 'O': Command_GX_Options();     break;
  default:  replyLongUnknow();        break;
  }
}
