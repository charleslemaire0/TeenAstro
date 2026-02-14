/**
 * Get commands: G (LX200 get) and GX (TeenAstro-specific get).
 * One file per letter (plan). Each :Gx# / :GXnn# documented as LX200 standard or TeenAstro specific.
 */
#include "Command.h"
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

// -----------------------------------------------------------------------------
//   GX - TeenAstro get  :GXnn#  (not LX200 standard)
// -----------------------------------------------------------------------------
void Command_GX() {
  switch (commandState.command[2])
  {
  case 'A':
    // :GXAn# Align Model values
  {
    float t11 = 0.f, t12 = 0.f, t13 = 0.f, t21 = 0.f, t22 = 0.f, t23 = 0.f, t31 = 0.f, t32 = 0.f, t33 = 0.f;
    if (mount.alignment.hasValid)
    {
      mount.alignment.conv.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    }
    switch (commandState.command[3])
    {
    case '0':
      // :GXA0#
      sprintf(commandState.reply, "%f#", t11);
      break;
    case '1':
      // :GXA1#
      sprintf(commandState.reply, "%f#", t12);
      break;
    case '2':
      // :GXA2#
      sprintf(commandState.reply, "%f#", t13);
      break;
    case '3':
      // :GXA3#
      sprintf(commandState.reply, "%f#", t21);
      break;
    case '4':
      // :GXA4#
      sprintf(commandState.reply, "%f#", t22);
      break;
    case '5':
      // :GXA5#
      sprintf(commandState.reply, "%f#", t23);
      break;
    case '6':
      // :GXA6#
      sprintf(commandState.reply, "%f#", t31);
      break;
    case '7':
      // :GXA7#
      sprintf(commandState.reply, "%f#", t32);
      break;
    case '8':
      // :GXA8#
      sprintf(commandState.reply, "%f#", t33);
      break;
      //case 'a':
      //case 'z':
      //case 'w':
      //{
      //  // :GXAa#
      //  // :GXAz# 
      //  // :GXAw#
      //  CoordConv::Err request = CoordConv::Err::POL_W;
      //  i = 1;
      //  switch (commandState.command[3])
      //  {
      //  case'a':
      //    request = CoordConv::Err::EQ_ALT;
      //    break;
      //  case'z':
      //    request = CoordConv::Err::EQ_AZ;
      //    break;
      //  case 'w':
      //    request = CoordConv::Err::POL_W;
      //    break;
      //  default:
      //    i = 0;
      //  }
      //  f1 = alignment.polErrorDeg(*localSite.latitude(), request);
      //  if (i == 0)
      //  {
      //    replyLongUnknow();
      //  }
      //  else
      //  {
      //    doubleToDms(commandState.reply, &f1, false, true, true);
      //    strcat(commandState.reply, "#");
      //  }
      //  break;
      //}
    default:
      replyLongUnknow();
      break;
    }
    break;
  }
  case 'E':
    // :GXEnn# get encoder commands
  {
    switch (commandState.command[3])
    {
    case '1':
    case '2':
      // :GXE1# get degree encoder 1
    {
      double f1 = commandState.command[3] == '1' ? mount.motorsEncoders.encoderA1.r_deg() : mount.motorsEncoders.encoderA2.r_deg();
      doubleToDms(commandState.reply, &f1, true, true, commandState.highPrecision);
      strcat(commandState.reply, "#");
    }
    break;
    case 'A':
    case 'Z':
      // :GXEA#,:GXEZ#  get encoder altitude and azimuth
      // :GXEA Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
      // :GXEZ Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
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
      //  :GD#   Get Telescope Encoder Declination
      //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
      //  :GR#   Get Telescope Encoder RA
      //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
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
    {
      sprintf(commandState.reply, "%u#", mount.motorsEncoders.EncodeSyncMode);
    }
    break;
    case 'P':
    {
      // :GXEP.#   Get Encoder pulse per 100 deg
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%lu#", (unsigned long)(100.0 * mount.motorsEncoders.encoderA2.pulsePerDegree));
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%lu#", (unsigned long)(100.0 * mount.motorsEncoders.encoderA1.pulsePerDegree));
      }
      else
        replyLongUnknow();
    }
    break;

    case 'r':
    {
      // :GXEr.#   Get Encoder reverse Rotation on/off
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.encoderA2.reverse);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.encoderA1.reverse);
      }
      else
        replyLongUnknow();
    }
    break;
    default:
      replyLongUnknow();
      break;
    }
  }
  break;
  case 'D':
    // :GXDnn# for Debug commands
    switch (commandState.command[3])
    {
    case'B':
    {
      // :GXDBn# Debug Backlash
      switch (commandState.command[4])
      {
      case '0':
        // :GXDB0# Debug inbacklashAxis1
        sprintf(commandState.reply, "%d#", (int)mount.axes.staA1.backlash_correcting);
        break;
      case '1':
        // :GXDB1# Debug inbacklashAxis2
        sprintf(commandState.reply, "%d#", (int)mount.axes.staA2.backlash_correcting);
        break;
      case '2':
        // :GXDB2# Debug Backlash blAxis1
        sprintf(commandState.reply, "%d#", (int)mount.axes.staA1.backlash_movedSteps);
        break;
      case '3':
        // :GXDB3# Debug Backlash blAxis1
        sprintf(commandState.reply, "%d#", (int)mount.axes.staA2.backlash_movedSteps);
        break;
      default:
        replyLongUnknow();
        break;
      }
      break;
    }
    case 'R':
    {
      // :GXDRn# Debug Rates
      switch (commandState.command[4])
      {
      case '1':
        // :GXDR1# axis1 requested tracking rate in sideral
        sprintf(commandState.reply, "%f#", mount.axes.staA1.RequestedTrackingRate);
        break;
      case '2':
        // :GXDR2# axis2 requested tracking rate in sideral 
        sprintf(commandState.reply, "%f#", mount.axes.staA2.RequestedTrackingRate);
        break;
      case '3':
        sprintf(commandState.reply, "%f#", (double)mount.axes.staA1.CurrentTrackingRate);
        break;
      case '4':
        sprintf(commandState.reply, "%f#", (double)mount.axes.staA2.CurrentTrackingRate);
        break;
      case '5':
        sprintf(commandState.reply, "%f#", (double)mount.axes.staA1.fstep);
        break;
      case '6':
        sprintf(commandState.reply, "%f#", (double)mount.axes.staA2.fstep);
        break;
      default:
        replyLongUnknow();
        break;
      }
      break;
    }
    case 'P':
    {
      // :GXDPn# Debug Position and Target
      long    temp;
      switch (commandState.command[4])
      {
      case '0':
        cli();
        temp = mount.axes.staA1.pos;
        sei();
        sprintf(commandState.reply, "%ld#", temp);
        break;  // Debug8, HA motor position
      case '1':
        cli();
        temp = mount.axes.staA2.pos;
        sei();
        sprintf(commandState.reply, "%ld#", temp);
        break;  // Debug9, Dec motor position
      case '2':
        cli();
        temp = mount.axes.staA1.target;
        sei();
        sprintf(commandState.reply, "%ld#", temp);
        break;  // Debug6, HA target position
      case '3':
        cli();
        temp = mount.axes.staA2.target;
        sei();
        sprintf(commandState.reply, "%ld#", temp);
        break;  // Debug7, Dec target position
      case '4':
        mount.axes.staA1.updateDeltaTarget();
        sprintf(commandState.reply, "%ld#", mount.axes.staA1.deltaTarget);
        break;  // Debug0, true vs. target RA position
      case '5':
        mount.axes.staA2.updateDeltaTarget();
        sprintf(commandState.reply, "%ld#", mount.axes.staA2.deltaTarget);
        break;
      case '6':
        sprintf(commandState.reply, "%lf#", mount.axes.staA1.interval_Step_Sid);
        break;
      case '7':
        sprintf(commandState.reply, "%lf#", mount.axes.staA2.interval_Step_Sid);
        break;
      default:
        replyLongUnknow();
        break;
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
    break;
  case 'P':
    // :GXPn# Intrument position
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
      //Other way to get intsrument angle to verify the pipeline
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
    break;
  case 'r':
    // :GXrn# user defined refraction settings
    switch (commandState.command[3])
    {
    case 'p':
      mount.refraction.forPole ? sprintf(commandState.reply, "y#") : sprintf(commandState.reply, "n#");
      break;
    case 'g':
      mount.refraction.forGoto ? sprintf(commandState.reply, "y#") : sprintf(commandState.reply, "n#");
      break;
    case 't':
      mount.refraction.forTracking ? sprintf(commandState.reply, "y#") : sprintf(commandState.reply, "n#");
      break;
    default:
      replyLongUnknow();
      break;
    }
    break;
  case 'R':
    // :GXRn# user defined rates
    switch (commandState.command[3])
    {
    case '0':
    case '1':
    case '2':
    case '3':
    {
      int i = commandState.command[3] - '0';
      // :GXRn# return user defined rate
      dtostrf(mount.guiding.guideRates[i], 2, 2, commandState.reply);
      strcat(commandState.reply, "#");
    }
    break;
    case 'A':
      // :GXRA# returns the Degrees For Acceleration
      dtostrf(mount.guiding.DegreesForAcceleration, 2, 1, commandState.reply);
      strcat(commandState.reply, "#");
      break;
    case 'B':
      // :GXRB# returns the Backlash Take up Rate
      sprintf(commandState.reply, "%ld#", (long)round(mount.axes.staA1.takeupRate));
      break;
    case 'D':
      // :GXRD# returns the Default Rate
      sprintf(commandState.reply, "%d#", XEEPROM.read(getMountAddress(EE_DefaultRate)));
      break;
    case 'X':
      // :GXRX# return Max Slew rate
      sprintf(commandState.reply, "%d#", XEEPROM.readUShort(getMountAddress(EE_maxRate)));
      break;
    case 'r':
      // :GXRr# Requested RA traking rate in sideral
    {
      long l1 = 10000l - (long)(mount.tracking.RequestedTrackingRateHA * 10000.0);
      sprintf(commandState.reply, "%ld#", l1);
    }
    break;
    case 'h':
      // :GXRh# Requested HA traking rate in sideral
    {
      long l1 = mount.tracking.RequestedTrackingRateHA * 10000.0;
      sprintf(commandState.reply, "%ld#", l1);
    }
    break;
    case 'd':
      // :GXRd# Requested DEC traking rate in sideral
    {
      long l1 = mount.tracking.RequestedTrackingRateDEC * 10000.0;
      sprintf(commandState.reply, "%ld#", l1);
    }
    break;
    case 'e':
      // :GXRe,VVVVVVVVVV# get stored Rate for RA
      sprintf(commandState.reply, "%ld#", mount.tracking.storedTrakingRateRA);
      break;
    case 'f':
      // :GXRf,VVVVVVVVVV# get stored Rate for DEC
      sprintf(commandState.reply, "%ld#", mount.tracking.storedTrakingRateDEC);
      break;
    default:
      replyLongUnknow();
      break;
    }
    break;
  case 'L':
    // :GXLn user defined limits
    switch (commandState.command[3])
    {
    case 'A':
      // :GXLA# get user defined minAXIS1 
    {
      int i = XEEPROM.readShort(getMountAddress(EE_minAxis1));
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'B':
      // :GXLB# get user defined maxAXIS1 
    {
      int i = XEEPROM.readShort(getMountAddress(EE_maxAxis1));
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'C':
      // :GXLC# get user defined minAXIS2
    {
      int i = XEEPROM.readShort(getMountAddress(EE_minAxis2));
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'D':
      // :GXLD# get user defined maxAXIS2
    {
      int i = XEEPROM.readShort(getMountAddress(EE_maxAxis2));
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'E':
      // :GXLE# return user defined Meridian East Limit
      sprintf(commandState.reply, "%ld#", (long)round(mount.limits.getMeridianEastLimit()));
      break;
    case 'W':
      // :GXLW# return user defined Meridian West Limit
      sprintf(commandState.reply, "%ld#", (long)round(mount.limits.getMeridianWestLimit()));
      break;
    case 'U':
      // :GXLU# return user defined Under pole Limit
      sprintf(commandState.reply, "%ld#", (long)round(mount.limits.underPoleLimitGOTO * 10));
      break;
    case 'O':
      // :GXLO# return user defined horizon Limit
      // NB: duplicate with :Go#
      sprintf(commandState.reply, "%+02d*#", mount.limits.maxAlt);
      break;
    case 'H':
      // :GXLH# return user defined horizon Limit
      // NB: duplicate with :Gh#
      sprintf(commandState.reply, "%+02d*#", mount.limits.minAlt);
      break;
    case 'S':
      // :GXLS# return user defined minimum distance in degreee from pole to keep tracking on for 6 hours after transit
      sprintf(commandState.reply, "%02d*#", mount.limits.distanceFromPoleToKeepTrackingOn);
      break;
    default:
      replyLongUnknow();
      break;
    }
    break;
  case 'l':
    // :GXln Mount type defined limits
    switch (commandState.command[3])
    {
    case 'A':
      // :GXlA#  Mount type defined minAXIS1
    {
      int i = mount.axes.geoA1.LimMinAxis;
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'B':
      // :GXlB#  Mount type defined maxAXIS1
    {
      int i = mount.axes.geoA1.LimMaxAxis;
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'C':
      // :GXlC#  Mount type defined minAXIS2
    {
      int i = mount.axes.geoA2.LimMinAxis;
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    case 'D':
      // :GXlD#  Mount type defined maxAXIS2
    {
      int i = mount.axes.geoA2.LimMaxAxis;
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    default:
      replyLongUnknow();
      break;
    }
    break;
  case 'T':
    // :GXTn# Date/Time definition
    switch (commandState.command[3])
    {
    case '0':
      // :GXT0# UTC time
      doubleToHms(commandState.reply, rtk.getUT(), true);
      strcat(commandState.reply, "#");
      break;
    case '1':
      // :GXT1# UTC date 
    {
      int i, i1, i2, i3, i4, i5;
      rtk.getUTDate(i, i1, i2, i3, i4, i5);
      i = i % 100;
      sprintf(commandState.reply, "%02d/%02d/%02d#", i1, i2, i);
      break;
    }
    // :GXT2# return seconds since 01/01/1970/00:00:00
    case '2':
    {
      unsigned long t = rtk.getTimeStamp();
      sprintf(commandState.reply, "%lu#", t);
    }
    break;
    case '3':
    {
      // :GXT3# LHA time
      double tmpLST;
      Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
      tmpLST = EQ_T.Ha() * RAD_TO_HOUR;
      doubleToHms(commandState.reply, &tmpLST, true);
      strcat(commandState.reply, "#");
    }
    break;
    }
    break;
  case 'I':
  {
    // :GXI#   Get telescope Status
    PoleSide currentSide = mount.getPoleSide();
    for (int i = 0; i < REPLY_BUFFER_LEN; i++)
      commandState.reply[i] = ' ';
    commandState.reply[0] = '0' + 2 * mount.tracking.movingTo + mount.tracking.sideralTracking;
    commandState.reply[1] = '0' + mount.tracking.sideralMode;
    const char* parkStatusCh = "pIPF";
    commandState.reply[2] = parkStatusCh[mount.parkHome.parkStatus];  // not [p]arked, parking [I]n-progress, [P]arked, Park [F]ailed
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
    // provide mount type
    if (mount.config.identity.mountType == MOUNT_TYPE_GEM)
    {
      commandState.reply[12] = 'E';
    }
    else if (mount.config.identity.mountType == MOUNT_TYPE_FORK)
      commandState.reply[12] = 'K';
    else if (mount.config.identity.mountType == MOUNT_TYPE_FORK_ALT)
      commandState.reply[12] = 'k';
    else if (mount.config.identity.mountType == MOUNT_TYPE_ALTAZM)
      commandState.reply[12] = 'A';
    else
      commandState.reply[12] = 'U';


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
  break;
  case 'J':
  {
    //specific command for ASCOM
    switch (commandState.command[3])
    {
    case 'B':
      // :GXJB# get if Both rate Axis are enable
    {
      mount.tracking.trackComp == TC_BOTH ? replyLongTrue(): replyLongFalse();
    }
    break;
    case 'C':
      // :GXJC# get if connected
    {
      replyLongTrue();
    }
    break;
    case 'm':
      // :GXJm# get if mount has motor and can slew track etc...
    {
      mount.motorsEncoders.enableMotor ? replyLongTrue() : replyLongFalse();
    }
    break;
    case 'M':
      // :GXJMn# get if axis is still moving
    {

      if (commandState.command[4] == '1')
      {
        mount.guiding.GuidingState == Guiding::GuidingAtRate && mount.guiding.guideA1.isBusy() ? replyLongTrue() : replyLongFalse();
      }
      else if (commandState.command[4] == '2')
      {
        mount.guiding.GuidingState == Guiding::GuidingAtRate && mount.guiding.guideA2.isBusy() ? replyLongTrue() : replyLongFalse();
      }
      else
      {
        replyLongUnknow();
      }
      break;
    }
    case 'P':
      // :GXJP# get if pulse guiding
    {
      if (mount.isGuidingStar())
      {
        replyLongTrue();
      }
      else
      {
        replyLongFalse();
      }
    }
    break;
    case 'S':
      // :GXJS# get if Slewing
    {
      if (mount.guiding.GuidingState == GuidingRecenter || mount.guiding.GuidingState == GuidingAtRate || mount.tracking.movingTo)
      {
        replyLongTrue();
      }
      else
      {
        replyLongFalse();
      }
    }
    break;
    case 'T':
      // :GXJT# get if tracking
    {
      if (mount.tracking.sideralTracking)
      {
        replyLongTrue();
      }
      else
      {
        replyLongFalse();
      }
    }
    break;
    }
  }
  break;
  case 'M':
  {
    // :GXM..#   Get Motor Settings
    switch (commandState.command[3])
    {
    case 'B':
    {
      // :GXMB.#   Get Motor backlash
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%d#", mount.motorsEncoders.motorA2.backlashAmount);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%d#", mount.motorsEncoders.motorA1.backlashAmount);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'b':
    {
      // :GXMb.#   Get Motor backlash Rate
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%d#", mount.motorsEncoders.motorA2.backlashRate);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%d#", mount.motorsEncoders.motorA1.backlashRate);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'G':
    {
      // :GXMG.#   Get Motor Gear
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%lu#", mount.motorsEncoders.motorA2.gear);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%lu#", mount.motorsEncoders.motorA1.gear);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'S':
    {
      // :GXMS.#   Get Stepper Step per Rotation
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA2.stepRot);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA1.stepRot);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'M':
    {
      // :GXMM.#   Get Stepper MicroStep per step
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.motorA2.micro);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.motorA1.micro);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'm':
    {
      // :GXMm.#   Get Stepper Silent mode on/off
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.motorA2.silent);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.motorA1.silent);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'R':
    {
      // :GXMR.#   Get Motor Reverse Rotation on/off
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.motorA2.reverse);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", (unsigned  int)mount.motorsEncoders.motorA1.reverse);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'C':
    {
      // :GXMC.#   Get Motor HighCurrent in mA
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA2.highCurr);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA1.highCurr);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'c':
    {
      // :GXMR.#   Get Motor LowCurrent in mA
      if (commandState.command[4] == 'D')
      {
        sprintf(commandState.reply, "%u#", (unsigned int)mount.motorsEncoders.motorA2.lowCurr);
      }
      else if (commandState.command[4] == 'R')
      {
        sprintf(commandState.reply, "%u#", (unsigned int)mount.motorsEncoders.motorA1.lowCurr);
      }
      else
        replyLongUnknow();
    }
    break;
    case 'L':
    {
      if ((commandState.command[4] == 'D' || commandState.command[4] == 'R'))
      {
        if (commandState.command[4] == 'D')
        {
          sprintf(commandState.reply, "%d#", mount.motorsEncoders.motorA2.driver.getSG());
        }
        else if (commandState.command[4] == 'R')
        {
          sprintf(commandState.reply, "%d#", mount.motorsEncoders.motorA1.driver.getSG());
        }
        else
          replyLongUnknow();
      }
      else
        replyLongUnknow();
    }
    break;
    case 'I':
    {
      if ((commandState.command[4] == 'D' || commandState.command[4] == 'R'))
      {
        if (commandState.command[4] == 'D')
        {
          sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA1.driver.getCurrent());
        }
        else if (commandState.command[4] == 'R')
        {
          sprintf(commandState.reply, "%u#", mount.motorsEncoders.motorA2.driver.getCurrent());
        }
        else replyLongUnknow();
      }
      else replyLongUnknow();
    }
    break;
    default:
      replyLongUnknow();
      break;
    }
  }
  break;
  case 'O':
    // :GXOn# Options
  {
    switch (commandState.command[3])
    {
    case 'I':
      // :GXOI# Mount idx
      sprintf(commandState.reply, "%d#", midx);
      break;
    case 'A':
      // :GXOA# Mount Name
      sprintf(commandState.reply, "%s#", mount.config.identity.mountName[midx]);
      break;
    case 'B':
      // :GXOB# first Mount Name
      sprintf(commandState.reply, "%s#", mount.config.identity.mountName[0]);
      break;
    case 'C':
      // :GXOC# second Mount Name
      sprintf(commandState.reply, "%s#", mount.config.identity.mountName[1]);
      break;
    case 'S':
      // :GXOS# get Slew Settle Duration in seconds 
    {
      int i = XEEPROM.read(getMountAddress(EE_SlewSettleDuration));
      sprintf(commandState.reply, "%d#", i);
    }
    break;
    }
  }
  break;
  default:
    replyLongUnknow();
    break;
  }
}

//----------------------------------------------------------------------------------
//   G - Get Telescope Information
// Intentionally long: central handler for Get (LX200) command variants.
void  Command_G()
{
  int i;

  switch (commandState.command[1])
  {
  case 'A':
  case 'Z':
  case ')':
    //  :GA#   Get Telescope Altitude, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //  :GZ#   Get telescope azimuth, Native LX200 command
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
    //  :G)#   Get Telescope Field in horizontal system rotation, TeenAstro LX200 command
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
  {
    Coord_HO HO_T = mount.getHorTopo();
    if (commandState.command[1] == 'A')
    {
      double f1 = HO_T.Alt() * RAD_TO_DEG;
      PrintAtitude(f1);
    }
    else if (commandState.command[1] == 'Z')
    {
      double f = degRange(HO_T.Az() * RAD_TO_DEG);
      PrintAzimuth(f);
    }
    else if (commandState.command[1] == ')')
    {
      double f1 = HO_T.FrH() * RAD_TO_DEG;
      PrintAzimuth(f1);
    }
  }
  break;
  case 'a':
    //  :Ga#   Get Local Time in 12 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    doubleToHms(commandState.reply, rtk.getLT(localSite.toff()), true);
    strcat(commandState.reply, "#");
    break;
  case 'C':
    //  :GC#   Get the current date, Native LX200 command
    //         Returns: MM/DD/YY#
    //         The current local calendar date
  {
    int i1, i2, i3, i4, i5;
    rtk.getULDate(i2, i, i1, i3, i4, i5, localSite.toff());
    i2 = i2 % 100;
    sprintf(commandState.reply, "%02d/%02d/%02d#", i, i1, i2);
  }
  break;
  case 'c':
    //  :Gc#   Get the current time format, Native LX200 command
    //         Returns: 24#
    //         The current local time format
    strcpy(commandState.reply, "24#");
    break;
  case 'D':
  case 'R':
  case '(':
    //  :GD#   Get Telescope Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //  :GR#   Get Telescope RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS# (based on precision setting)
    //  :G(#   Get Telescope Field rotation in EQ system, TeenAstro LX200 command
    //         Returns: DDD*MM# or DDD*MM'SS# (based on precision setting)
    //  :GDL#   Get Telescope Declination, TeenAstro LX200 command
    //         Returns: sDD,VVVVV#
    //  :GRL#   Get Telescope RA, TeenAstro LX200 command
    //         Returns: DDD,VVVVV#
  {
    Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
    if (commandState.command[1] == 'R')
    {
      
      if (commandState.command[2] == 'L')
      {
        double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_DEG;
        sprintf(commandState.reply, "%08.5f#", f);
      }
      else
      {
        double f = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
        PrintRa(f);
      }
    }
    else if (commandState.command[1] == 'D')
    {
      double f1 = EQ_T.Dec() * RAD_TO_DEG;
      if (commandState.command[2] == 'L')
      {
        sprintf(commandState.reply, "%+08.5f#", f1);
      }
      else
      {
        PrintDec(f1);
      }
    }
    else if (commandState.command[1] == '(')
    {
      double f1 = EQ_T.FrE() * RAD_TO_DEG;
      PrintAzimuth(f1);
    }
  }
  break;
  case 'd':
    //  :Gd#   Get Currently Selected Target Declination, Native LX200 command
    //         Returns: sDD*MM# or sDD*MM'SS# (based on precision setting)
    //  :GdL#   Get Currently Selected Target Declination, TeenAstro LX200 command
    //          Returns: sDD, VVVVV#
    if (commandState.command[2] == 'L')
    {
      sprintf(commandState.reply, "%+08.5f#", mount.targetCurrent.newTargetDec);
    }
    else
    {
      doubleToDms(commandState.reply, &mount.targetCurrent.newTargetDec, false, true, commandState.highPrecision);
      strcat(commandState.reply, "#");
    }
    break;
  case 'e':
    //  :Ge#   Get Current Site Elevation above see level in meter, TeenAstro LX200 command
    //         Returns: sDDDD#
    sprintf(commandState.reply, "%+04d#", *localSite.elevation());
    break;
  case 'f':
    //  :Gf#   Get master sidereal clock (tunable by :T+# and :T-# / reset by :TR#)
    //         Returns: dd#
    char    tmp[10];
    dtostrf(mount.tracking.siderealClockSpeed, 0, 0, tmp);
    strcpy(commandState.reply, tmp);
    strcat(commandState.reply, "#");
    break;
  case 'G':
    //  :GG#   Get UTC offset time, Native LX200 command
    //         Returns: sHH.H#
    //         The number of decimal hours to add to local time to convert it to UTC 
    sprintf(commandState.reply, "%+05.1f#", *localSite.toff());
    break;
  case 'g':
  {
    //  :Gg#   Get Current Site Longitude, Native LX200 command
    //         Returns: sDDD*MM#
    //  :Ggf#  Returns: sDDD*MM'SS#
    //         The current site Longitude. East Longitudes are negative
    doubleToDms(commandState.reply, localSite.longitude(), true, true, commandState.command[2] == 'f');
    strcat(commandState.reply, "#");
  }
  break;
  case 'h':
  {
    //  :Gh#   Get Horizon Limit, Native LX200 command
    //         Returns: sDD*#
    //         The minimum elevation of an object above the horizon required for a mount goto
    sprintf(commandState.reply, "%+02d*#", mount.limits.minAlt);
  }
  break;
  case 'L':
  {
    //  :GL#   Get Local Time in 24 hour format, Native LX200 command
    //         Returns: HH:MM:SS#
    doubleToHms(commandState.reply, rtk.getLT(localSite.toff()), true);
    strcat(commandState.reply, "#");
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
    i = commandState.command[1] - 'M';
    bool ok = XEEPROM.readString(EE_sites + i * SiteSize + EE_site_name, commandState.reply, siteNameLen);
    ok = ok && commandState.reply[0] != 0;
    if (!ok)
    {
      sprintf(commandState.reply, "Site %d", i);
      XEEPROM.writeString(EE_sites + i * SiteSize + EE_site_name, commandState.reply, siteNameLen);
    }
    strcat(commandState.reply, "#");
    break;
  }
  case 'm':
  {
    //  :Gm#   Gets the meridian pier-side, TeenAstro LX200 command
    //         Returns: E#, W#, N# (none/parked), ?# (Meridian flip in progress)
    //         A # terminated string with the pier side.
    PoleSide currentSide = mount.getPoleSide();
    strcpy(commandState.reply, "?#");
    if (currentSide == POLE_UNDER) commandState.reply[0] = mount.isAltAZ() || localSite.northHemisphere() ? 'E' : 'W';
    if (currentSide == POLE_OVER) commandState.reply[0] = mount.isAltAZ() || localSite.northHemisphere() ? 'W' : 'E';
    strcat(commandState.reply, "#");
    break;
  }
  case 'n':
    //  :Gn#   Get Current Site name, TeenAstro LX200 command
    //         Returns: <string>#
    sprintf(commandState.reply, "%s#", localSite.siteName());
    break;
  case 'o':
    //  :Go#   Get Overhead Limit, TeenAstro LX200 command
    //         Returns: DD*#
    //         The highest elevation above the horizon that the telescope will goto
    sprintf(commandState.reply, "%02d*#", mount.limits.maxAlt);
    break;
  case 'r':
    //  :Gr#   Get current/target object RA, Native LX200 command
    //         Returns: HH:MM.T# or HH:MM:SS (based on precision setting)
    //  :GrL#  Get current/target object Ra, TeenAstro LX200 command
    //         Returns: sDDD.VVVVV#
    if (commandState.command[2] == 'L')
    {
      sprintf(commandState.reply, "%+08.5f#", mount.targetCurrent.newTargetRA);
    }
    else
    {
      double f = mount.targetCurrent.newTargetRA;
      f /= 15.0;
      doubleToHms(commandState.reply, &f, commandState.highPrecision);
      strcat(commandState.reply, "#");
    }
  break;
  case 'S':
    //  :GS#   Get the Sidereal Time, Native LX200 command
    //         Returns: HH:MM:SS#
    //         The Sidereal Time as an ASCII Sexidecimal value in 24 hour format
    //  :GSL#  Get the Sidereal Time, TeenAstro LX200 command
    //         Returns: HH.VVVVVV#
  {
    double f = rtk.LST();
    if (commandState.command[2] == 'L')
    {
      sprintf(commandState.reply, "%+08.6f#", f);
    }
    else
    {
      doubleToHms(commandState.reply, &f, true);
      strcat(commandState.reply, "#");
    }
  }
  break;
  case 'T':
    //  :GT#   Get tracking rate, Native LX200 command
    //         Returns: dd.ddddd#
    //         Returns the tracking rate if siderealTracking, 0.0 otherwise
  {
    double f = 0.0;
    if (mount.tracking.sideralTracking && !mount.tracking.movingTo)
    {
      f = mount.tracking.RequestedTrackingRateHA;
      f *= 60 * 1.00273790935;
    }
    char    temp[10];
    dtostrf(f, 0, 5, temp);
    strcpy(commandState.reply, temp);
    strcat(commandState.reply, "#");
  }
  break;
  case 't':
    //  :Gt#   Get Current Site Latitude, Native LX200 command
    //         Returns: sDD*MM#
    //  :Gtf#  Returns: sDD*MM'SS#
    //         The latitude of the current site. Positive for North latitudes
    doubleToDms(commandState.reply, localSite.latitude(), false, true, commandState.command[2] == 'f');
    strcat(commandState.reply, "#");
    break;
  case 'V':
  {
    switch (commandState.command[2])
    {
    case 'D':
      //  :GVD#   Get Firmware Date, Native LX200 command
      strcpy(commandState.reply, FirmwareDate);
      strcat(commandState.reply, "#");
      break;
    case 'N':
      //  :GVN#   Get Firmware number, Native LX200 command
      strcpy(commandState.reply, FirmwareNumber);
      strcat(commandState.reply, "#");
      break;
    case 'P':
      //  :GVP#   Get Firmware Name, Native LX200 command
      strcpy(commandState.reply, FirmwareName);
      strcat(commandState.reply, "#");
      break;
    case 'p':
      //  :GVp#   Get Firmware SubName
      strcpy(commandState.reply, FirmwareSubName);
      strcat(commandState.reply, "#");
      break;
    case 'T':
      //  :GVT#   Get Firmware Time, Native LX200 command
      strcpy(commandState.reply, FirmwareTime);
      strcat(commandState.reply, "#");
      break;
    case 'B':
      //  :GVB#   Get Firmware board version, extended LX200 command
      sprintf(commandState.reply, "%d#", VERSION);
      break;
    case 'b':
      //  :GVb#   Get Firmware Stepper driver board, extended LX200 command
      sprintf(commandState.reply, "%d#", AxisDriver);
      break;
    default:
      replyLongUnknow();
      break;
    }
  }
  break;
  case 'W':
  {
    switch (mount.config.identity.mountType)
    {
    case MOUNT_TYPE_ALTAZM:
    case MOUNT_TYPE_FORK_ALT:
      strcat(commandState.reply, "A");
      break;
    case MOUNT_TYPE_FORK:
      strcat(commandState.reply, "P");
      break;
    case MOUNT_TYPE_GEM:
      strcat(commandState.reply, "G");
      break;
    case MOUNT_UNDEFINED:
    default:
      strcat(commandState.reply, "L");
      break;
    }
    mount.tracking.sideralTracking ? strcat(commandState.reply, "T") : strcat(commandState.reply, "N");
    if (mount.isAtHome())
    {
      strcat(commandState.reply, "H");
    }
    else if (mount.isParked())
    {
      strcat(commandState.reply, "P");
    }
    else if (mount.alignment.hasValid)
    {
      strcat(commandState.reply, "2");
    }
    else
    {
      strcat(commandState.reply, "1");
    }
    strcat(commandState.reply, "#");
  }


  break;

  case 'X':
    Command_GX();
    break;
  default:
    replyLongUnknow();
    break;
  }
}