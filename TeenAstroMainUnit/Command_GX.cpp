/**
 * GX — TeenAstro-specific get commands (:GXnn#).
 * Split from Command_G.cpp for clarity.  Called from Command_G() via case 'X'.
 */
#include "Command.h"
#include "CommandHelpers.h"
#include "ValueToString.h"
#include <cstring>

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

// ---- GX All State  :GXAS# --------------------------------------------------
// Returns a base64-encoded 102-byte binary snapshot of all mount state.
// 102 bytes → 136 base64 chars + '#'.  Padding: 2 bytes (102 % 3 == 0, no pad).
//
// Packet layout (little-endian). All float fields are float64 (double) for full precision.
//   Bytes 0-5:   Status (tracking, sidereal, park, atHome, pierSide, guidingRate, aligned, mountType, spiralRunning, guidingEW/NS, trackComp, fault, pulse, gnssFlags, error, enableFlags, hasFocuser)
//   Bytes 6-11:  UTC hour,min,sec,month,day,year(2-digit)
//   Bytes 12-83: Positions and rates (9 × float64 LE: RA, Dec, Alt, Az, LST, Target RA, Target Dec, TrackRate RA, TrackRate Dec)
//   Bytes 84-91: Stored rates (int32 LE at 84, 88 — same as :GXRe#/:GXRf#)
//   Bytes 92-97: Focuser (optional; when hasFocuser): position uint32 LE, speed uint16 LE. Otherwise zero.
//   Byte  98:   Timezone offset (int8_t, toff × 10; subtract to get local from UTC)
//   Byte  99:   Alignment ref count (0–2, from CoordConv::getRefs())
//   Byte  100:  Alignment phase (bits 0-1: 0=idle,1=select,2=slew,3=recenter) + star number (bits 2-4: 0–7)
//   Byte  101:  XOR checksum of bytes 0-100

static constexpr int GXAS_PKT_LEN = 102;

static const char GX_B64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void gxasB64Encode(const uint8_t* in, char* out, int len)
{
  int o = 0;
  for (int i = 0; i < len; i += 3)
  {
    uint32_t b = ((uint32_t)in[i] << 16) | ((uint32_t)in[i + 1] << 8) | in[i + 2];
    out[o++] = GX_B64[(b >> 18) & 0x3F];
    out[o++] = GX_B64[(b >> 12) & 0x3F];
    out[o++] = GX_B64[(b >>  6) & 0x3F];
    out[o++] = GX_B64[ b        & 0x3F];
  }
  out[o] = 0;
}

static void gxasPackF64(uint8_t* pkt, int off, double v)
{
  memcpy(pkt + off, &v, 8);
}

static void Command_GX_AllState()
{
  uint8_t pkt[GXAS_PKT_LEN];
  memset(pkt, 0, sizeof(pkt));
  PoleSide currentSide = mount.getPoleSide();

  // ── Byte 0 ───────────────────────────────────────────────────────────────
  uint8_t tracking     = (uint8_t)(2 * (mount.isMovingTo() ? 1 : 0) + (mount.tracking.sideralTracking ? 1 : 0));
  uint8_t sidereal     = (uint8_t)(mount.tracking.sideralMode  & 0x3);
  uint8_t park         = (uint8_t)(mount.parkHome.parkStatus    & 0x3);
  uint8_t atHome       = mount.isAtHome() ? 1u : 0u;
  uint8_t pierSide     = (currentSide == POLE_OVER) ? 1u : 0u;
  pkt[0] = tracking | (sidereal << 2) | (park << 4) | (atHome << 6) | (pierSide << 7);

  // ── Byte 1 ───────────────────────────────────────────────────────────────
  uint8_t guidingRate  = (uint8_t)(mount.guiding.recenterGuideRate & 0x7);
  uint8_t aligned      = mount.alignment.hasValid ? 1u : 0u;
  uint8_t mountType    = 0u;
  if      (mount.config.identity.mountType == MOUNT_TYPE_GEM)      mountType = 1u;
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK)     mountType = 2u;
  else if (mount.config.identity.mountType == MOUNT_TYPE_ALTAZM)   mountType = 3u;
  else if (mount.config.identity.mountType == MOUNT_TYPE_FORK_ALT) mountType = 4u;
  uint8_t spiralRunning = mount.tracking.doSpiral ? 1u : 0u;
  pkt[1] = guidingRate | (aligned << 3) | (mountType << 4) | (spiralRunning << 7);

  // ── Byte 2 ───────────────────────────────────────────────────────────────
  uint8_t guidingEW = 0;
  if      (mount.guiding.guideA1.isMFW())     guidingEW = 1;
  else if (mount.guiding.guideA1.isMBW())     guidingEW = 2;
  else if (mount.guiding.guideA1.isBraking()) guidingEW = 3;

  uint8_t guidingNS = 0;
  if (currentSide == POLE_OVER)
  {
    if      (mount.guiding.guideA2.isMBW())     guidingNS = 1;
    else if (mount.guiding.guideA2.isMFW())     guidingNS = 2;
    else if (mount.guiding.guideA2.isBraking()) guidingNS = 3;
  }
  else
  {
    if      (mount.guiding.guideA2.isMFW())     guidingNS = 1;
    else if (mount.guiding.guideA2.isMBW())     guidingNS = 2;
    else if (mount.guiding.guideA2.isBraking()) guidingNS = 3;
  }
  uint8_t trackComp    = (uint8_t)(mount.tracking.trackComp & 0x3);
  uint8_t fault        = (mount.axes.staA1.fault || mount.axes.staA2.fault) ? 1u : 0u;
  uint8_t pulseGuiding = mount.isGuidingStar() ? 1u : 0u;
  pkt[2] = guidingEW | (guidingNS << 2) | (trackComp << 4) | (fault << 6) | (pulseGuiding << 7);

  // ── Byte 3: gnssFlags ────────────────────────────────────────────────────
  uint8_t gFlags = 0;
  bitWrite(gFlags, 0, mount.config.peripherals.hasGNSS);
  if (iSGNSSValid())
  {
    bitWrite(gFlags, 1, true);
    bitWrite(gFlags, 2, isTimeSyncWithGNSS());
    bitWrite(gFlags, 3, isLocationSyncWithGNSS());
    bitWrite(gFlags, 4, isHdopSmall());
  }
  pkt[3] = gFlags;

  // ── Byte 4: error ────────────────────────────────────────────────────────
  pkt[4] = (uint8_t)(mount.errors.lastError);

  // ── Byte 5: enableFlags | hasFocuser ─────────────────────────────────────
  uint8_t enableFlags = 0;
  bitWrite(enableFlags, 0, mount.motorsEncoders.enableEncoder);
  bitWrite(enableFlags, 1, mount.motorsEncoders.encoderA1.calibrating() &&
                           mount.motorsEncoders.encoderA2.calibrating());
  bitWrite(enableFlags, 2, mount.config.peripherals.PushtoStatus != PT_OFF);
  bitWrite(enableFlags, 3, mount.motorsEncoders.enableMotor);
  bitWrite(enableFlags, 4, mount.config.peripherals.hasFocuser);
  pkt[5] = enableFlags;

  // ── Bytes 6-11: UTC date/time ─────────────────────────────────────────────
  {
    int y, m, d, h, mi, s;
    rtk.getUTDate(y, m, d, h, mi, s);
    pkt[6]  = (uint8_t)h;
    pkt[7]  = (uint8_t)mi;
    pkt[8]  = (uint8_t)s;
    pkt[9]  = (uint8_t)m;
    pkt[10] = (uint8_t)d;
    pkt[11] = (uint8_t)(y % 100);
  }

  // ── Bytes 12-83: positions and tracking rates (9 × float64 LE) ─────────────
  {
    Coord_EQ EQ_T = mount.getEqu(*localSite.latitude() * DEG_TO_RAD);
    double ra  = EQ_T.Ra(rtk.LST() * HOUR_TO_RAD) * RAD_TO_HOUR;
    double dec = EQ_T.Dec() * RAD_TO_DEG;
    Coord_HO HO_T = mount.getHorTopo();
    double alt = HO_T.Alt() * RAD_TO_DEG;
    double az  = degRange(HO_T.Az() * RAD_TO_DEG);
    double lst = rtk.LST();
    double tgtRA  = mount.targetCurrent.newTargetRA  / 15.0;
    double tgtDec = mount.targetCurrent.newTargetDec;
    double trackRateRA = 1.0 - mount.tracking.RequestedTrackingRateHA;
    double trackRateDec = mount.tracking.RequestedTrackingRateDEC;
    gxasPackF64(pkt, 12, ra);
    gxasPackF64(pkt, 20, dec);
    gxasPackF64(pkt, 28, alt);
    gxasPackF64(pkt, 36, az);
    gxasPackF64(pkt, 44, lst);
    gxasPackF64(pkt, 52, tgtRA);
    gxasPackF64(pkt, 60, tgtDec);
    gxasPackF64(pkt, 68, trackRateRA);
    gxasPackF64(pkt, 76, trackRateDec);
  }

  // ── Bytes 84-91: stored tracking rates (int32 LE) ────────────────────────
  int32_t storedRateRA  = (int32_t)mount.tracking.storedTrakingRateRA;
  int32_t storedRateDec = (int32_t)mount.tracking.storedTrakingRateDEC;
  memcpy(pkt + 84, &storedRateRA, 4);
  memcpy(pkt + 88, &storedRateDec, 4);

  // ── Bytes 92-97: focuser (optional; when hasFocuser). Otherwise zero. ─────
  if (mount.config.peripherals.hasFocuser)
  {
    Focus_Serial.flush();
    while (Focus_Serial.available() > 0) Focus_Serial.read();
    Focus_Serial.print(":F?#");
    Focus_Serial.flush();
    delay(20);
    char fc[50] = "";
    int pos = 0;
    while (Focus_Serial.available() > 0 && pos < 48)
    {
      char b = (char)Focus_Serial.read();
      fc[pos++] = b;
      if (b == '#') break;
    }
    fc[pos] = 0;
    if (fc[0] == '?')
    {
      uint32_t fPos  = (uint32_t)atol(fc + 1);
      uint16_t fSpd  = 0;
      const char* sp = strchr(fc + 1, ' ');
      if (sp) fSpd   = (uint16_t)atoi(sp + 1);
      memcpy(pkt + 92, &fPos, 4);
      pkt[96] = (uint8_t)(fSpd & 0xFF);
      pkt[97] = (uint8_t)(fSpd >> 8);
    }
  }
  // ── Byte 98: timezone offset (toff × 10, int8_t) ───────────────────────────
  pkt[98] = (uint8_t)((int8_t)round(*localSite.toff() * 10.0f));

  // Byte 99: alignment reference count (0, 1, or 2)
  pkt[99] = (uint8_t)(mount.alignment.conv.getRefs() & 0x3);

  // Byte 100: alignment phase (bits 0-1) + star number (bits 2-4)
  // Auto-detect slew/recenter transitions from mount motion state.
  {
    AlignPhase phase = mount.alignment.alignPhase;
    if (phase == ALIGN_SELECT && mount.isMovingTo()) {
      phase = ALIGN_SLEW;
      mount.alignment.alignPhase = ALIGN_SLEW;
    }
    if (phase == ALIGN_SLEW && !mount.isMovingTo()) {
      phase = ALIGN_RECENTER;
      mount.alignment.alignPhase = ALIGN_RECENTER;
    }
    pkt[100] = (phase & 0x3) | ((mount.alignment.alignStarNum & 0x7) << 2);
  }

  // Byte 101: XOR checksum of bytes 0-100
  uint8_t xorChk = 0;
  for (int i = 0; i < 101; i++) xorChk ^= pkt[i];
  pkt[101] = xorChk;

  // ── Base64 encode (102 bytes → 136 chars) → reply ────────────────────────
  gxasB64Encode(pkt, commandState.reply, GXAS_PKT_LEN);
  strcat(commandState.reply, "#");
}

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
  case 's':
    // :GXAs#  Return alignment star name (set by app via :SXAs,name#)
    sprintf(commandState.reply, "%s#", mount.alignment.alignStarName);
    break;
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
    case '7': sprintf(commandState.reply, "%f#", (double)mount.guiding.guideA2.absRate); break;
    case '8': sprintf(commandState.reply, "%f#", (double)mount.guiding.guideA2.speedMultiplier); break;
    case '9': sprintf(commandState.reply, "%f#", mount.guiding.guideA2.getAmount()); break;
    case 'A': sprintf(commandState.reply, "%lf#", mount.axes.staA1.interval_Step_Cur); break;
    case 'B': sprintf(commandState.reply, "%lf#", mount.axes.staA2.interval_Step_Cur); break;
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
    // :GXDW# Get workload   :GXDW1# Get missed sidereal ticks (debug)
    if (commandState.command[4] == 0)
    {
      sprintf(commandState.reply, "%ld%%#", (tlp.getWorstTime() * 100L) / 9970L);
      tlp.resetWorstTime();
    }
    else if (commandState.command[4] == '1')
    {
      sprintf(commandState.reply, "%ld#", rtk.m_missedTicks);
    }
    else if (commandState.command[4] == '2')
    {
      rtk.m_missedTicks = 0;
      replyLongTrue();
    }
    else if (commandState.command[4] == '3')
    {
      sprintf(commandState.reply, "%ld#", rtk.m_peakGap);
    }
    else if (commandState.command[4] == '4')
    {
      rtk.m_peakGap = 0;
      replyLongTrue();
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
  case '4':
    // Effective max MoveAxis/M1/M2 rate (arcsec/s); matches :M1#/:M2# limit. :GXRX# is EEPROM and can be slightly higher.
    dtostrf(mount.guiding.guideRates[4], 2, 2, commandState.reply);
    strcat(commandState.reply, "#");
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
    // :GXRr# — ASCOM RA tracking offset (RA sec/sidereal sec) as 16 lowercase hex chars (IEEE754 LE) + '#'
  {
    double trackRateRA = 1.0 - mount.tracking.RequestedTrackingRateHA;
    uint8_t b[8];
    memcpy(b, &trackRateRA, 8);
    static const char* hx = "0123456789abcdef";
    for (int i = 0; i < 8; i++)
    {
      commandState.reply[i * 2]     = hx[b[i] >> 4];
      commandState.reply[i * 2 + 1] = hx[b[i] & 0x0F];
    }
    commandState.reply[16] = '#';
    commandState.reply[17] = '\0';
  }
  break;
  case 'd':
    // :GXRd# — Dec tracking offset (arcsec/s), same 16-hex LE encoding + '#'
  {
    double trackRateDec = mount.tracking.RequestedTrackingRateDEC;
    uint8_t b[8];
    memcpy(b, &trackRateDec, 8);
    static const char* hx = "0123456789abcdef";
    for (int i = 0; i < 8; i++)
    {
      commandState.reply[i * 2]     = hx[b[i] >> 4];
      commandState.reply[i * 2 + 1] = hx[b[i] & 0x0F];
    }
    commandState.reply[16] = '#';
    commandState.reply[17] = '\0';
  }
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
    // :GXT1# UTC date (MM/DD/YY#). Reply may be slow if RTC read (getUTDate) blocks.
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

// ---- GX Config Settings  :GXCS# -------------------------------------------
// Returns a base64-encoded 90-byte binary snapshot of all mount configuration:
// motor parameters (both axes), speed/rates, limits, encoders, and refraction
// flags.  This replaces ~40 individual :GXMx#/:GXRx#/:GXLx# queries.
//
// 90 bytes (divisible by 3) → 120 base64 chars + '#' = 121 chars on wire.
//
// Packet layout (little-endian):
//   ── Axis 1 Motor (16 bytes, offset 0) ──────────────────────────────────
//   Bytes  0– 3: gear        uint32 LE  (raw, ÷1000 = gear ratio float)
//   Bytes  4– 5: stepRot     uint16 LE  (steps per motor rotation)
//   Bytes  6– 7: backlash    uint16 LE  (steps)
//   Bytes  8– 9: backlashRate uint16 LE (steps/s)
//   Bytes 10–11: lowCurr     uint16 LE  (mA)
//   Bytes 12–13: highCurr    uint16 LE  (mA)
//   Byte  14:    micro       uint8      (microstep divider, 0–8)
//   Byte  15:    flags       uint8      [bit0=reverse, bit1=silent]
//   ── Axis 2 Motor (16 bytes, offset 16) ─────────────────────────────────
//   Bytes 16–31: same layout as Axis 1
//   ── Rates / Speed (24 bytes, offset 32) ────────────────────────────────
//   Bytes 32–35: guideRate   float32 LE (arcsec/sec, index 0)
//   Bytes 36–39: slowRate    float32 LE (arcsec/sec, index 1)
//   Bytes 40–43: mediumRate  float32 LE (arcsec/sec, index 2)
//   Bytes 44–47: fastRate    float32 LE (arcsec/sec, index 3)
//   Bytes 48–51: acceleration float32 LE (degrees for acceleration)
//   Bytes 52–53: maxRate     uint16 LE  (µs/step from EE_maxRate)
//   Byte  54:    defaultRate uint8      (0–4, from EE_DefaultRate)
//   Byte  55:    settleTime  uint8      (seconds, from EE_SlewSettleDuration)
//   ── Limits (18 bytes, offset 56) ───────────────────────────────────────
//   Bytes 56–57: meridianE   int16 LE  (arcmin)
//   Bytes 58–59: meridianW   int16 LE  (arcmin)
//   Bytes 60–61: axis1min    int16 LE  (×10 degrees from EEPROM)
//   Bytes 62–63: axis1max    int16 LE
//   Bytes 64–65: axis2min    int16 LE
//   Bytes 66–67: axis2max    int16 LE
//   Bytes 68–69: underPole   uint16 LE (underPoleLimitGOTO × 10)
//   Byte  70:    minAlt      int8      (degrees)
//   Byte  71:    maxAlt      int8      (degrees)
//   Byte  72:    minDistPole uint8     (degrees)
//   Byte  73:    refrFlags   uint8     [bit0=tracking, bit1=goto, bit2=pole]
//   ── Encoders (10 bytes, offset 74) ─────────────────────────────────────
//   Bytes 74–77: ppd1×100    uint32 LE (pulsePerDegree×100, Axis1)
//   Bytes 78–81: ppd2×100    uint32 LE (pulsePerDegree×100, Axis2)
//   Byte  82:    encSyncMode uint8     (EncoderSync enum)
//   Byte  83:    encFlags    uint8     [bit0=rev1, bit1=rev2]
//   ── Options (5 bytes, offset 84) ────────────────────────────────────────
//   Byte  84:    mountIdx    uint8     (active mount index, midx)
//   Bytes 85–88: reserved    uint8[4]
//   ── Checksum (1 byte, offset 89) ────────────────────────────────────────
//   Byte  89:    XOR of bytes 0–88

// Pack a uint16 into `pkt` at `off` (little-endian).
static void gxcsPackU16(uint8_t* pkt, int off, uint16_t v)
{
  pkt[off]     = (uint8_t)(v & 0xFF);
  pkt[off + 1] = (uint8_t)(v >> 8);
}

// Pack an int16 into `pkt` at `off` (little-endian).
static void gxcsPackI16(uint8_t* pkt, int off, int16_t v)
{
  gxcsPackU16(pkt, off, (uint16_t)v);
}

// Pack a uint32 into `pkt` at `off` (little-endian).
static void gxcsPackU32(uint8_t* pkt, int off, uint32_t v)
{
  pkt[off]     = (uint8_t)(v & 0xFF);
  pkt[off + 1] = (uint8_t)((v >> 8)  & 0xFF);
  pkt[off + 2] = (uint8_t)((v >> 16) & 0xFF);
  pkt[off + 3] = (uint8_t)((v >> 24) & 0xFF);
}

// Pack one motor axis into `pkt` starting at `base`.
static void gxcsPackAxis(uint8_t* pkt, int base, const AxisRef& ax)
{
  gxcsPackU32(pkt, base +  0, (uint32_t)ax.motor.gear);
  gxcsPackU16(pkt, base +  4, (uint16_t)ax.motor.stepRot);
  gxcsPackU16(pkt, base +  6, (uint16_t)ax.motor.backlashAmount);
  gxcsPackU16(pkt, base +  8, (uint16_t)ax.motor.backlashRate);
  gxcsPackU16(pkt, base + 10, (uint16_t)ax.motor.lowCurr);
  gxcsPackU16(pkt, base + 12, (uint16_t)ax.motor.highCurr);
  pkt[base + 14] = (uint8_t)ax.motor.micro;
  uint8_t flags  = 0;
  bitWrite(flags, 0, ax.motor.reverse);
  bitWrite(flags, 1, ax.motor.silent);
  pkt[base + 15] = flags;
}

static void Command_GX_AllConfig()
{
  uint8_t pkt[90];
  memset(pkt, 0, sizeof(pkt));

  // ── Axis 1 Motor (offset 0) ──────────────────────────────────────────────
  AxisRef* ax1 = selectAxis('R');
  if (ax1) gxcsPackAxis(pkt, 0, *ax1);

  // ── Axis 2 Motor (offset 16) ─────────────────────────────────────────────
  AxisRef* ax2 = selectAxis('D');
  if (ax2) gxcsPackAxis(pkt, 16, *ax2);

  // ── Rates / Speed (offset 32) ────────────────────────────────────────────
  float fv;
  fv = (float)mount.guiding.guideRates[0]; memcpy(pkt + 32, &fv, 4);
  fv = (float)mount.guiding.guideRates[1]; memcpy(pkt + 36, &fv, 4);
  fv = (float)mount.guiding.guideRates[2]; memcpy(pkt + 40, &fv, 4);
  fv = (float)mount.guiding.guideRates[3]; memcpy(pkt + 44, &fv, 4);
  fv = (float)mount.guiding.DegreesForAcceleration; memcpy(pkt + 48, &fv, 4);
  gxcsPackU16(pkt, 52, (uint16_t)XEEPROM.readUShort(getMountAddress(EE_maxRate)));
  pkt[54] = (uint8_t)XEEPROM.read(getMountAddress(EE_DefaultRate));
  pkt[55] = (uint8_t)XEEPROM.read(getMountAddress(EE_SlewSettleDuration));

  // ── Limits (offset 56) ───────────────────────────────────────────────────
  gxcsPackI16(pkt, 56, (int16_t)round(mount.limits.getMeridianEastLimit()));
  gxcsPackI16(pkt, 58, (int16_t)round(mount.limits.getMeridianWestLimit()));
  gxcsPackI16(pkt, 60, (int16_t)XEEPROM.readShort(getMountAddress(EE_minAxis1)));
  gxcsPackI16(pkt, 62, (int16_t)XEEPROM.readShort(getMountAddress(EE_maxAxis1)));
  gxcsPackI16(pkt, 64, (int16_t)XEEPROM.readShort(getMountAddress(EE_minAxis2)));
  gxcsPackI16(pkt, 66, (int16_t)XEEPROM.readShort(getMountAddress(EE_maxAxis2)));
  gxcsPackU16(pkt, 68, (uint16_t)round(mount.limits.underPoleLimitGOTO * 10.0));
  pkt[70] = (uint8_t)(int8_t)mount.limits.minAlt;
  pkt[71] = (uint8_t)(int8_t)mount.limits.maxAlt;
  pkt[72] = (uint8_t)mount.limits.distanceFromPoleToKeepTrackingOn;
  uint8_t refrFlags = 0;
  bitWrite(refrFlags, 0, mount.refraction.forTracking);
  bitWrite(refrFlags, 1, mount.refraction.forGoto);
  bitWrite(refrFlags, 2, mount.refraction.forPole);
  pkt[73] = refrFlags;

  // ── Encoders (offset 74) ─────────────────────────────────────────────────
  gxcsPackU32(pkt, 74, (uint32_t)round(mount.motorsEncoders.encoderA1.pulsePerDegree * 100.0));
  gxcsPackU32(pkt, 78, (uint32_t)round(mount.motorsEncoders.encoderA2.pulsePerDegree * 100.0));
  pkt[82] = (uint8_t)mount.motorsEncoders.EncodeSyncMode;
  uint8_t encFlags = 0;
  bitWrite(encFlags, 0, mount.motorsEncoders.encoderA1.reverse);
  bitWrite(encFlags, 1, mount.motorsEncoders.encoderA2.reverse);
  pkt[83] = encFlags;

  // ── Options (offset 84) ──────────────────────────────────────────────────
  pkt[84] = (uint8_t)midx;
  // pkt[85-88] remain 0

  // ── XOR checksum (byte 89) ───────────────────────────────────────────────
  uint8_t xorChk = 0;
  for (int i = 0; i < 89; i++) xorChk ^= pkt[i];
  pkt[89] = xorChk;

  // ── Base64 encode (90 bytes → 120 chars) + '#' ───────────────────────────
  gxasB64Encode(pkt, commandState.reply, 90);
  strcat(commandState.reply, "#");
}

// =============================================================================
//   Command_GX  --  :GXnn#  dispatch to sub-handlers
// =============================================================================
void Command_GX() {
  switch (commandState.command[2])
  {
  case 'A':
    if (commandState.command[3] == 'S') Command_GX_AllState();
    else                                Command_GX_Alignment();
    break;
  case 'C':
    if (commandState.command[3] == 'S') Command_GX_AllConfig();
    else                                replyLongUnknow();
    break;
  case 'E': Command_GX_Encoders();    break;
  case 'D': Command_GX_Debug();       break;
  case 'P': Command_GX_Position();    break;
  case 'r': Command_GX_Refraction();  break;
  case 'R': Command_GX_Rates();       break;
  case 'L': Command_GX_Limits();      break;
  case 'l': Command_GX_MountLimits(); break;
  case 'T': Command_GX_Time();        break;
  case 'M': Command_GX_Motors();      break;
  case 'O': Command_GX_Options();     break;
  default:  replyLongUnknow();        break;
  }
}
