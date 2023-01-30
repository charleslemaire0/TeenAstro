#include "ValueToString.h"
#include "Command.h"

//----------------------------------------------------------------------------------
// :GXnn#  Get TeenAstro value
//         Returns: value
//         All these command are not part of the LX200 Standard.

void PrintAtitude(double& val)
{
  if (!doubleToDms(reply, &val, false, true, highPrecision))
    replyFailed();
  else
    strcat(reply, "#");
}
void PrintAzimuth(double& val)
{
  val = AzRange(val);
  if (!doubleToDms(reply, &val, true, false, highPrecision))
    replyFailed();
  else
    strcat(reply, "#");
}
void PrintDec(double& val)
{
  if (!doubleToDms(reply, &val, false, true, highPrecision))
    replyFailed();
  else
    strcat(reply, "#");
}
void PrintRa(double& val)
{
  if (!doubleToHms(reply, &val, highPrecision))
    replyFailed();
  else
    strcat(reply, "#");
}


void Command_GX()
{
  int i;
  double f, f1;
  long   l1;
  //  :GXnn#   Get TeenAstro Specific value
  switch (command[2])
  {
  case 'A':
    // :GXAn# Align Model values
  {
    float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
    if (hasStarAlignment)
    {
      alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    }
    switch (command[3])
    {
    case '0':
      // :GXA0#
      sprintf(reply, "%f#", t11);
      break;
    case '1':
      // :GXA1#
      sprintf(reply, "%f#", t12);
      break;
    case '2':
      // :GXA2#
      sprintf(reply, "%f#", t13);
      break;
    case '3':
      // :GXA3#
      sprintf(reply, "%f#", t21);
      break;
    case '4':
      // :GXA4#
      sprintf(reply, "%f#", t22);
      break;
    case '5':
      // :GXA5#
      sprintf(reply, "%f#", t23);
      break;
    case '6':
      // :GXA6#
      sprintf(reply, "%f#", t31);
      break;
    case '7':
      // :GXA7#
      sprintf(reply, "%f#", t32);
      break;
    case '8':
      // :GXA8#
      sprintf(reply, "%f#", t33);
      break;
    case 'c':
      // :GXAc#
      TrackingCompForAlignment ? sprintf(reply, "y#") : sprintf(reply, "n#");
      break;
    case 'a':
    case 'z':
    case 'w':
    {
      // :GXAa#
      // :GXAz# 
      // :GXAw#
      CoordConv::Err request = CoordConv::Err::POL_W;
      i = 1;
      switch (command[3])
      {
      case'a':
        request = CoordConv::Err::EQ_ALT;
        break;
      case'z':
        request = CoordConv::Err::EQ_AZ;
        break;
      case 'w':
        request = CoordConv::Err::POL_W;
        break;
      default:
        i = 0;
      }
      f1 = alignment.polErrorDeg(*localSite.latitude(), request);
      if (!doubleToDms(reply, &f1, false, true, true) || i == 0)
        replyFailed();
      else
        strcat(reply, "#");
      break;
    }
    default:
      replyFailed();
      break;
    }
    break;
  }
  case 'E':
    // :GXEnn# get encoder commands
  {
    switch (command[3])
    {
    case '1':
    case '2':
      // :GXE1# get degree encoder 1
    {
#if HASEncoder
      f1 = command[3] == '1' ? encoderA1.r_deg() : encoderA2.r_deg();
      if (!doubleToDms(reply, &f1, true, true, highPrecision))
        replyFailed();
      else
        strcat(reply, "#");
#endif // HASEncoder
    }
    break;
    case 'A':
    case 'Z':
      // :GXEA#,:GXEZ#  get encoder altitude and azimuth
      // :GXEA Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
      // :GXEZ Returns: DDD* MM# or DDD * MM'SS# (based on precision setting)
    {
#if HASEncoder
      getHorAppE(&f, &f1);
#else
      getHorApp(&f, &f1);
#endif // HASEncoder
      command[3] == 'A' ? PrintAtitude(f1) : PrintAzimuth(f);
    }
    break;
    case 'D':
    case 'R':
    {
      //  :GD#   Get Telescope Encoder Declination
      //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
      //  :GR#   Get Telescope Encoder RA
      //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
      static unsigned long _coord_t = 0;
      static double _dec = 0;
      static double _ra = 0;
      if (millis() - _coord_t < 100)
      {
        f = _ra;
        f1 = _dec;
      }
      else
      {
#if HASEncoder
        getEquE(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
#else
        getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
#endif
        f /= 15.0;
        _ra = f;
        _dec = f1;
        _coord_t = millis();
      }
      command[3] == 'D' ? PrintDec(f1) : PrintRa(f);
    }
    break;
    case 'O':
      // :GXEO#  get encoder Sync Option
    {
#if HASEncoder
      sprintf(reply, "%u#", EncodeSyncMode);
#endif // HASEncoder
    }
    break;

    case 'P':
    {
      // :GXEP.#   Get Encoder pulse per 100 deg
      if (command[4] == 'D')
      {
#if HASEncoder
        sprintf(reply, "%lu#", (unsigned long)(100.0 * encoderA2.pulsePerDegree));
#endif // HASEncoder
      }
      else if (command[4] == 'R')
      {
#if HASEncoder
        sprintf(reply, "%lu#", (unsigned long)(100.0 * encoderA1.pulsePerDegree));
#endif // HASEncoder
      }
      else
        replyFailed();
    }
    break;

    case 'r':
    {
      // :GXMr.#   Get Encoder reverse Rotation on/off
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.reverse);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.reverse);
      }
      else
        replyFailed();
    }
    break;
    default:
      replyFailed();
      break;
    }
  }
  break;
  case 'D':
    // :GXDnn# for Debug commands
    switch (command[3])
    {
    case'B':
    {
      // :GXDBn# Debug Backlash
      switch (command[4])
      {
      case '0':
        // :GXDB0# Debug inbacklashAxis1
        sprintf(reply, "%d#", (int)backlashA1.correcting);
        break;
      case '1':
        // :GXDB1# Debug inbacklashAxis2
        sprintf(reply, "%d#", (int)backlashA2.correcting);
        break;
      case '2':
        // :GXDB2# Debug Backlash blAxis1
        sprintf(reply, "%d#", (int)backlashA1.movedSteps);
        break;
      case '3':
        // :GXDB3# Debug Backlash blAxis1
        sprintf(reply, "%d#", (int)backlashA2.movedSteps);
        break;
      default:
        replyFailed();
        break;
      }
      break;
    }
    case 'R':
    {
      // :GXDRn# Debug Rates
      switch (command[4])
      {
      case '1':
        // :GXDR1# axis1 requested tracking rate in sideral
        sprintf(reply, "%f#", staA1.RequestedTrackingRate);
        break;
      case '2':
        // :GXDR2# axis2 requested tracking rate in sideral 
        sprintf(reply, "%f#", staA2.RequestedTrackingRate);
        break;
      case '3':
        sprintf(reply, "%f#", (double)staA1.CurrentTrackingRate);
        break;
      case '4':
        sprintf(reply, "%f#", (double)staA1.fstep);
        break;
      default:
        replyFailed();
        break;
      }
      break;
    }
    case 'P':
    {
      // :GXDPn# Debug Position and Target
      long    temp;
      switch (command[4])
      {
      case '0':
        cli();
        temp = staA1.pos;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug8, HA motor position
      case '1':
        cli();
        temp = staA2.pos;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug9, Dec motor position
      case '2':
        cli();
        temp = staA1.target;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug6, HA target position
      case '3':
        cli();
        temp = staA2.target;
        sei();
        sprintf(reply, "%ld#", temp);
        break;  // Debug7, Dec target position
      case '4':
        staA1.updateDeltaTarget();
        sprintf(reply, "%ld#", staA1.deltaTarget);
        break;  // Debug0, true vs. target RA position
      case '5':
        staA2.updateDeltaTarget();
        sprintf(reply, "%ld#", staA2.deltaTarget);
        break;
      default:
        replyFailed();
        break;
      }
      break;
    }
    case 'W':
    {
      // :GXDW# Get workload
      if (command[4] == 0)
      {
        sprintf(reply, "%ld%%#", (tlp.getWorstTime() * 100L) / 9970L);
        tlp.resetWorstTime();
      }
      else
        replyFailed();
      break;
    }
    break;
    default:
      replyFailed();
      break;
    }
    break;
  case 'P':
    // :GXPn# Intrument position
    switch (command[3])
    {
    case '1':
      cli();
      f1 = staA1.pos / geoA1.stepsPerDegree;
      sei();
      if (!doubleToDms(reply, &f1, true, true, highPrecision))
        replyFailed();
      else
        strcat(reply, "#");
      break;
    case '2':
      cli();
      f1 = staA2.pos / geoA2.stepsPerDegree;
      sei();
      if (!doubleToDms(reply, &f1, true, true, highPrecision))
        replyFailed();
      else
        strcat(reply, "#");
      break;
    }
    break;
  case 'r':
    // :GXrn# user defined refraction settings
    switch (command[3])
    {
    case 'p':
      doesRefraction.forPole ? sprintf(reply, "y#") : sprintf(reply, "n#");
      break;
    case 'g':
      doesRefraction.forGoto ? sprintf(reply, "y#") : sprintf(reply, "n#");
      break;
    case 't':
      doesRefraction.forTracking ? sprintf(reply, "y#") : sprintf(reply, "n#");
      break;
    default:
      replyFailed();
      break;
    }
    break;
  case 'R':
    // :GXRn# user defined rates
    switch (command[3])
    {
    case '0':
    case '1':
    case '2':
    case '3':
      i = command[3] - '0';
      // :GXRn# return user defined rate
      dtostrf(guideRates[i], 2, 2, reply);
      strcat(reply, "#");
      break;
    case 'A':
      // :GXRA# returns the Degrees For Acceleration
      dtostrf(DegreesForAcceleration, 2, 1, reply);
      strcat(reply, "#");
      break;
    case 'B':
      // :GXRB# returns the Backlash Take up Rate
      sprintf(reply, "%ld#", (long)round(staA1.takeupRate));
      break;
    case 'D':
      // :GXRD# returns the Default Rate
      sprintf(reply, "%d#", XEEPROM.read(getMountAddress(EE_DefaultRate)));
      break;
    case 'X':
      // :GXRX# return Max Slew rate
      sprintf(reply, "%d#", XEEPROM.readInt(getMountAddress(EE_maxRate)));
      break;
    case 'r':
      // :GXRr# Requested RA traking rate in sideral
      l1 = -(RequestedTrackingRateHA - 1.0) * 10000.0;
      sprintf(reply, "%ld#", l1);
      break;
    case 'h':
      l1 = RequestedTrackingRateHA * 10000.0;
      // :GXRh# Requested HA traking rate in sideral
      sprintf(reply, "%ld#", l1);
      break;
    case 'd':
      // :GXRd# Requested DEC traking rate in sideral
      l1 = RequestedTrackingRateDEC * 10000.0;
      sprintf(reply, "%ld#", l1);
      break;
    case 'e':
      // :GXRe,VVVVVVVVVV# get stored Rate for RA
      sprintf(reply, "%ld#", storedTrakingRateRA);
      break;
    case 'f':
      // :GXRf,VVVVVVVVVV# get stored Rate for DEC
      sprintf(reply, "%ld#", storedTrakingRateDEC);
      break;
    default:
      replyFailed();
      break;
    }
    break;
  case 'L':
    // :GXLn user defined limits
    switch (command[3])
    {
    case 'A':
      // :GXLA# get user defined minAXIS1 (always negatif)
      i = XEEPROM.readInt(getMountAddress(EE_minAxis1));
      sprintf(reply, "%d#", i);
      break;
    case 'B':
      // :GXLB# get user defined maxAXIS1 (always positf)
      i = XEEPROM.readInt(getMountAddress(EE_maxAxis1));
      sprintf(reply, "%d#", i);
      break;
    case 'C':
      // :GXLC# get user defined minAXIS2 (always positf)
      i = XEEPROM.readInt(getMountAddress(EE_minAxis2));
      sprintf(reply, "%d#", i);
      break;
    case 'D':
      // :GXLD# get user defined maxAXIS2 (always positf)
      i = XEEPROM.readInt(getMountAddress(EE_maxAxis2));
      sprintf(reply, "%d#", i);
      break;
    case 'E':
      // :GXLE# return user defined Meridian East Limit
      sprintf(reply, "%ld#", (long)round(minutesPastMeridianGOTOE));
      break;
    case 'W':
      // :GXLW# return user defined Meridian West Limit
      sprintf(reply, "%ld#", (long)round(minutesPastMeridianGOTOW));
      break;
    case 'U':
      // :GXLU# return user defined Under pole Limit
      sprintf(reply, "%ld#", (long)round(underPoleLimitGOTO * 10));
      break;
    case 'O':
      // :GXLO# return user defined horizon Limit
      // NB: duplicate with :Go#
      sprintf(reply, "%+02d*#", maxAlt);
      break;
    case 'H':
      // :GXLH# return user defined horizon Limit
      // NB: duplicate with :Gh#
      sprintf(reply, "%+02d*#", minAlt);
      break;
    default:
      replyFailed();
      break;
    }
    break;
  case 'T':
    // :GXTn# Date/Time definition
    switch (command[3])
    {
    case '0':
      // :GXT0# UTC time
      doubleToHms(reply, rtk.getUT(), true);
      strcat(reply, "#");
      break;
    case '1':
      // :GXT1# UTC date 
    {
      int i1, i2, i3, i4, i5;
      rtk.getUTDate(i, i1, i2, i3, i4, i5);
      i = i % 100;
      sprintf(reply, "%02d/%02d/%02d#", i1, i2, i);
      break;
    }
    // :GXT2# return seconds since 01/01/1970/00:00:00
    case '2':
    {
      unsigned long t = rtk.getTimeStamp();
      sprintf(reply, "%lu#", t);
    }
    break;
    case '3':
    {
      // :GXT3# LHA time

      static unsigned long _coord_t1 = 0;
      static double _dec1 = 0;
      static double _ra1 = 0;

      double tmpLST, f, f1;
      tmpLST = rtk.LST();

      if (millis() - _coord_t1 < 100)
      {
        f = _ra1;
        f1 = _dec1;
      }
      else
      {
        getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
        f /= 15.0;
        _ra1 = f;
        _dec1 = f1;
        _coord_t1 = millis();
      }

      tmpLST -= f;
      if (tmpLST < -12)
        tmpLST += 24;
      else if (tmpLST > 12)
        tmpLST -= 24;

      if (!doubleToHms(reply, &tmpLST, true))
        strcpy(reply, "0#");
      else
        strcat(reply, "#");
    }
    break;
    }
    break;
  case 'I':
  {
    // :GXI#   Get telescope Status
    PierSide currentSide = GetPierSide();
    for (i = 0; i < 50; i++)
      reply[i] = ' ';
    i = 0;
    reply[0] = '0' + 2 * movingTo + sideralTracking;
    reply[1] = '0' + sideralMode;
    const char* parkStatusCh = "pIPF";
    reply[2] = parkStatusCh[parkStatus];  // not [p]arked, parking [I]n-progress, [P]arked, Park [F]ailed
    if (atHome) reply[3] = 'H';
    reply[4] = '0' + activeGuideRate;
    if (doSpiral) reply[5] = '@';
    else if (GuidingState != GuidingOFF)
    {
      reply[5] = 'G';
    }
    if (GuidingState == GuidingPulse || GuidingState == GuidingST4) reply[6] = '*';
    else if (GuidingState == GuidingRecenter) reply[6] = '+';
    else if (GuidingState == GuidingAtRate) reply[6] = '-';
    if (guideA1.isMFW()) reply[7] = '>';
    else if (guideA1.isMBW()) reply[7] = '<';
    else if (guideA1.isBraking()) reply[7] = 'b';

    if (currentSide == PIER_WEST)
    {
      if (guideA2.isMBW()) reply[8] = '^';
      else if (guideA2.isMFW()) reply[8] = '_';
      else if (guideA2.isBraking()) reply[8] = 'b';
    }
    else
    {
      if (guideA2.isMFW()) reply[8] = '^';
      else if (guideA2.isMBW()) reply[8] = '_';
      else if (guideA2.isBraking()) reply[8] = 'b';
    }

    if (staA1.fault || staA2.fault) reply[9] = 'f';
    reply[10] = '0';
    if (isAltAZ())
      reply[10] += doesRefraction.forTracking ? RC_FULL_BOTH : RC_ALIGN_BOTH;
    else if (tc == TC_NONE || (!doesRefraction.forTracking && !TrackingCompForAlignment))
    {
      reply[10] += RC_NONE;
    }
    else
    {
      if (doesRefraction.forTracking)
      {
        reply[10] += RC_FULL_RA;
      }
      else if (TrackingCompForAlignment)
      {
        reply[10] += RC_ALIGN_RA;
      }
      if (tc == TC_BOTH)
        reply[10] += 1;
    }
    reply[11] = hasStarAlignment ? '1' : '0';
    // provide mount type
    if (mountType == MOUNT_TYPE_GEM)
    {
      reply[12] = 'E';
    }
    else if (mountType == MOUNT_TYPE_FORK)
      reply[12] = 'K';
    else if (mountType == MOUNT_TYPE_FORK_ALT)
      reply[12] = 'k';
    else if (mountType == MOUNT_TYPE_ALTAZM)
      reply[12] = 'A';
    else
      reply[12] = 'U';

    if (currentSide == PIER_EAST) reply[13] = 'E';
    if (currentSide == PIER_WEST) reply[13] = 'W';

    char val = 0;
    bitWrite(val, 0, hasGNSS);
    if (iSGNSSValid())
    {
      bitWrite(val, 1, true);
      bitWrite(val, 2, isTimeSyncWithGNSS());
      bitWrite(val, 3, isLocationSyncWithGNSS());
    }
    reply[14] = 'A' + val;
    reply[15] = '0' + lastError;
    val = 0;
#if HASEncoder
    bitWrite(val, 0, 1);
    bitWrite(val, 1, encoderA1.calibrating() && encoderA2.calibrating());
    bitWrite(val, 2, PushtoStatus != PT_OFF);
#endif
    reply[16] = 'A' + val;
    reply[17] = '#';
    reply[18] = 0;
    i = 17;
  }
  break;
  case 'M':
  {
    // :GXM..#   Get Motor Settings
    switch (command[3])
    {
    case 'B':
    {
      // :GXMB.#   Get Motor backlash
      if (command[4] == 'D')
      {
        sprintf(reply, "%d#", backlashA2.inSeconds);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%d#", backlashA1.inSeconds);
      }
      else
        replyFailed();
    }
    break;
    case 'G':
    {
      // :GXMG.#   Get Motor Gear
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.gear);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.gear);
      }
      else
        replyFailed();
    }
    break;
    case 'S':
    {
      // :GXMS.#   Get Stepper Step per Rotation
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.stepRot);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.stepRot);
      }
      else
        replyFailed();
    }
    break;
    case 'M':
    {
      // :GXMM.#   Get Stepper MicroStep per step
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.micro);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.micro);
      }
      else
        replyFailed();
    }
    break;
    case 'm':
    {
      // :GXMm.#   Get Stepper Silent mode on/off
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.silent);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.silent);
      }
      else
        replyFailed();
    }
    break;
    case 'R':
    {
      // :GXMR.#   Get Motor Reverse Rotation on/off
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA2.reverse);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned  int)motorA1.reverse);
      }
      else
        replyFailed();
    }
    break;
    case 'C':
    {
      // :GXMC.#   Get Motor HighCurrent in mA
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", motorA2.highCurr);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", motorA1.highCurr);
      }
      else
        replyFailed();
    }
    break;
    case 'c':
    {
      // :GXMR.#   Get Motor LowCurrent in mA
      if (command[4] == 'D')
      {
        sprintf(reply, "%u#", (unsigned int)motorA2.lowCurr);
      }
      else if (command[4] == 'R')
      {
        sprintf(reply, "%u#", (unsigned int)motorA1.lowCurr);
      }
      else
        replyFailed();
    }
    break;
    case 'L':
    {
      if ((command[4] == 'D' || command[4] == 'R'))
      {
        if (command[4] == 'D')
        {
          sprintf(reply, "%d#", motorA2.driver.getSG());
        }
        else if (command[4] == 'R')
        {
          sprintf(reply, "%d#", motorA1.driver.getSG());

        }
        else
          replyFailed();
      }
      else
        replyFailed();
    }
    break;
    case 'I':
    {
      if ((command[4] == 'D' || command[4] == 'R'))
      {
        if (command[4] == 'D')
        {
          sprintf(reply, "%u#", motorA1.driver.getCurrent());
        }
        else if (command[4] == 'R')
        {
          sprintf(reply, "%u#", motorA2.driver.getCurrent());
        }
        else replyFailed();
      }
      else replyFailed();
    }
    break;
    default:
      replyFailed();
      break;
    }
  }
  break;
  case 'O':
    // :GXOn# Options
  {
    switch (command[3])
    {
    case 'I':
      // :GXOI# Mount idx
      sprintf(reply, "%d#", midx);
      break;
    case 'A':
      // :GXOA# Mount Name
      sprintf(reply, "%s#", mountName[midx]);
      break;
    case 'B':
      // :GXOB# first Mount Name
      sprintf(reply, "%s#", mountName[0]);
      break;
    case 'C':
      // :GXOC# second Mount Name
      sprintf(reply, "%s#", mountName[1]);
      break;
    }
  }
  break;
  default:
    replyFailed();
    break;
  }
}

//----------------------------------------------------------------------------------
//   G - Get Telescope Information
void  Command_G()
{
  double f, f1;
  int i;

  switch (command[1])
  {
  case 'A':
  case 'Z':
    //  :GA#   Get Telescope Altitude, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //  :GZ#   Get telescope azimuth, Native LX200 command
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
    getHorAppE(&f, &f1);
    command[1] == 'A' ? PrintAtitude(f1) : PrintAzimuth(f);
    break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    highPrecision = i;
    break;

  case 'C':
    //  :GC#   Get the current date, Native LX200 command
    //         Returns: MM/DD/YY#
    //         The current local calendar date
  {
    int i1, i2, i3, i4, i5;
    rtk.getULDate(i2, i, i1, i3, i4, i5, localSite.toff());
    i2 = i2 % 100;
    sprintf(reply, "%02d/%02d/%02d#", i, i1, i2);
    break;
  }
  case 'c':
    //  :Gc#   Get the current time format, Native LX200 command
    //         Returns: 24#
    //         The current local time format
    strcpy(reply, "24#");
    break;
  case 'D':
  case 'R':
    //  :GD#   Get Telescope Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //  :GR#   Get Telescope RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
  {
    static unsigned long _coord_t = 0;
    static double _dec = 0;
    static double _ra = 0;

    if (millis() - _coord_t < 100)
    {
      f = _ra;
      f1 = _dec;
    }
    else
    {
      getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
      f /= 15.0;
      _ra = f;
      _dec = f1;
      _coord_t = millis();
    }
    command[1] == 'D' ? PrintDec(f1) : PrintRa(f);
    break;
  }
  case 'd':
    //  :Gd#   Get Currently Selected Target Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    if (!doubleToDms(reply, &newTargetDec, false, true, highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    break;
  case 'e':
    //  :Ge#   Get Current Site Elevation above see level in meter, TeenAstro LX200 command
    //         Returns: sDDDD#
    sprintf(reply, "%+04d#", *localSite.elevation());
    break;
  case 'f':
    //  :Gf#   Get master sidereal clock (tunable by :T+# and :T-# / reset by :TR#)
    //         Returns: dd#
    char    tmp[10];
    dtostrf(siderealClockSpeed, 0, 0, tmp);
    strcpy(reply, tmp);
    strcat(reply, "#");
    break;
  case 'G':
    //  :GG#   Get UTC offset time, Native LX200 command
    //         Returns: sHH.H#
    //         The number of decimal hours to add to local time to convert it to UTC 
    sprintf(reply, "%+05.1f#", *localSite.toff());
    break;
  case 'g':
  {
    //  :Gg#   Get Current Site Longitude, Native LX200 command
    //         Returns: sDDD*MM#
    //  :Ggf#  Returns: sDDD*MM'SS#
    //         The current site Longitude. East Longitudes are negative
    int i = highPrecision;
    highPrecision = command[2] == 'f';
    if (!doubleToDms(reply, localSite.longitude(), true, true, highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    highPrecision = i;
  }
  break;
  case 'h':
  {
    //  :Gh#   Get Horizon Limit, Native LX200 command
    //         Returns: sDD*#
    //         The minimum elevation of an object above the horizon required for a mount goto
    sprintf(reply, "%+02d*#", minAlt);
  }
  break;

  case 'L':
  {
    //  :GL#   Get Local Time in 24 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    i = highPrecision;
    highPrecision = true;
    if (!doubleToHms(reply, rtk.getLT(localSite.toff()), highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    highPrecision = i;
  }
  break;
  //  :GM#   Get Site 1 Name, Native LX200 command
  //  :GN#   Get Site 2 Name, Native LX200 command
  //  :GO#   Get Site 3 Name, Native LX200 command
  //         Returns: <string>#
  //         A # terminated string with the name of the requested site.
  case 'M':
  case 'N':
  case 'O':
  {
    i = command[1] - 'M';
    bool ok = XEEPROM.readString(EE_sites + i * SiteSize + EE_site_name, reply, siteNameLen);
    ok = ok && reply[0] != 0;
    if (!ok)
    {
      sprintf(reply, "Site %d", i);
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, reply, siteNameLen);
    }
    strcat(reply, "#");
    break;
  }
  case 'm':
  {
    //  :Gm#   Gets the meridian pier-side, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    PierSide currentSide = GetPierSide();
    strcpy(reply, "?#");
    if (currentSide == PIER_EAST) reply[0] = 'E';
    if (currentSide == PIER_WEST) reply[0] = 'W';
    break;
  }
  case 'n':
    //  :Gn#   Get Current Site name, TeenAstro LX200 command
    //         Returns: <string>#
    sprintf(reply, "%s#", localSite.siteName());
    break;
  case 'o':
    //  :Go#   Get Overhead Limit, TeenAstro LX200 command
    //         Returns: DD*#
    //         The highest elevation above the horizon that the telescope will goto
    sprintf(reply, "%02d*#", maxAlt);
    break;
  case 'r':
    //  :Gr#   Get current/target object RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS (based on precision setting)
    f = newTargetRA;
    f /= 15.0;
    if (!doubleToHms(reply, &f, highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    break;
  case 'S':
    //  :GS#   Get the Sidereal Time, Native LX200 command
    //         Returns: HH:MM:SS#
    //         The Sidereal Time as an ASCII Sexidecimal value in 24 hour format
    i = highPrecision;
    highPrecision = true;
    f = rtk.LST();
    if (!doubleToHms(reply, &f, highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    highPrecision = i;
    break;
  case 'T':
    //  :GT#   Get tracking rate, Native LX200 command
    //         Returns: dd.ddddd#
    //         Returns the tracking rate if siderealTracking, 0.0 otherwise
    if (sideralTracking && !movingTo)
    {
      f = RequestedTrackingRateHA;
      f *= 60 * 1.00273790935;
    }
    else
      f = 0.0;

    char    temp[10];
    dtostrf(f, 0, 5, temp);
    strcpy(reply, temp);
    strcat(reply, "#");
    break;
  case 't':
    //  :Gt#   Get Current Site Latitude, Native LX200 command
    //         Returns: sDD*MM#
    //  :Gtf#  Returns: sDD*MM'SS#
    //         The latitude of the current site. Positive for North latitudes
    i = highPrecision;
    highPrecision = command[2] == 'f';
    if (!doubleToDms(reply, localSite.latitude(), false, true, highPrecision))
      replyFailed();
    else
      strcat(reply, "#");
    highPrecision = i;
    break;
  case 'V':
  {
    if (command[3] != (char)0)
    {
      strcpy(reply, "0#");
      return;
    }
    switch (command[2])
    {
    case 'D':
      //  :GVD#   Get Firmware Date, Native LX200 command
      strcpy(reply, FirmwareDate);
      break;
    case 'N':
      //  :GVN#   Get Firmware number, Native LX200 command
      strcpy(reply, FirmwareNumber);
      break;
    case 'P':
      //  :GVP#   Get Firmware Name, Native LX200 command
      strcpy(reply, FirmwareName);
      break;
    case 'p':
      //  :GVp#   Get Firmware SubName
      strcpy(reply, FirmwareSubName);
      break;
    case 'T':
      //  :GVT#   Get Firmware Time, Native LX200 command
      strcpy(reply, FirmwareTime);
      break;
    case 'B':
      //  :GVB#   Get Firmware board version, extended LX200 command
      sprintf(reply, "%d", VERSION);
      break;
    case 'b':
      //  :GVb#   Get Firmware Stepper driver board, extended LX200 command
      sprintf(reply, "%d", AxisDriver);
      break;
    default:
      replyFailed();
      break;
    }
    strcat(reply, "#");
  }
  break;
  case 'X':
    Command_GX();
    break;
  default:
    replyFailed();
    break;
  }
}