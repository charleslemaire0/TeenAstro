/*
 * Command_GX
 * TeenAstro specific commands
 */
#include "Global.h"
#include "Command_GNSS.h"



static constexpr int GXAS_PKT_LEN = 102;

static const char GX_B64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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
  static void gxcsPackAxis(uint8_t* pkt, int base, MotorDriver *mP)
  {
    gxcsPackU32(pkt, base +  0, (uint32_t) mP->gear);
    gxcsPackU16(pkt, base +  4, (uint16_t) mP->stepRot);
    gxcsPackU16(pkt, base +  6, (uint16_t) 0);  // mP->backlashAmount
    gxcsPackU16(pkt, base +  8, (uint16_t) 0);  // mP->backlashRate
    gxcsPackU16(pkt, base + 10, (uint16_t) mP->lowCurr);
    gxcsPackU16(pkt, base + 12, (uint16_t) mP->highCurr);
    pkt[base + 14] = (uint8_t) mP->micro;
    uint8_t flags  = 0;
    bitWrite(flags, 0, mP->reverse);
    bitWrite(flags, 1, mP->silent);
    pkt[base + 15] = flags;
  }




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

void Command_GX_AllState()
{
    uint8_t pkt[GXAS_PKT_LEN];
    memset(pkt, 0, sizeof(pkt));

    // ── Byte 0 ───────────────────────────────────────────────────────────────
    uint8_t tracking     = (uint8_t)(2 * (isSlewing() ? 1 : 0) + (isTracking() ? 1 : 0));
    uint8_t sidereal     = (uint8_t)(siderealMode  & 0x3);
    uint8_t park         = (uint8_t)(parkStatus()    & 0x3);
    uint8_t at_home       = atHome() ? 1u : 0u;
    uint8_t pierSide     = (mount.mP->GetPierSide() == PIER_WEST) ? 1u : 0u;
    pkt[0] = tracking | (sidereal << 2) | (park << 4) | (at_home << 6) | (pierSide << 7);

    // ── Byte 1 ───────────────────────────────────────────────────────────────
    uint8_t guidingRate  = (uint8_t)(RXX & 0x7);        // not sure about this
    uint8_t aligned      = mount.mP->pm.alignment.isReady() ? 1u : 0u;
    uint8_t mountType    = 0u;
    if      (mount.mP->type == MOUNT_TYPE_GEM)      mountType = 1u;
    else if (mount.mP->type == MOUNT_TYPE_FORK)     mountType = 2u;
    else if (mount.mP->type == MOUNT_TYPE_ALTAZM)   mountType = 3u;
    else if (mount.mP->type == MOUNT_TYPE_FORK_ALT) mountType = 4u;
    uint8_t spiralRunning = getEvent(EV_SPIRAL) ? 1u : 0u;
    pkt[1] = guidingRate | (aligned << 3) | (mountType << 4) | (spiralRunning << 7);

    // ── Byte 2 ───────────────────────────────────────────────────────────────
    uint8_t guidingEW = 0;
    if (getEvent(EV_GUIDING_AXIS1))
    {
        if (getEvent(EV_WEST))
            guidingEW = 1;
        else if (getEvent(EV_EAST))
            guidingEW = 2;
    }

    uint8_t guidingNS = 0;
    if (getEvent(EV_GUIDING_AXIS1))
    {
        if  (getEvent(EV_NORTH))
            guidingNS = 1;
        else if (getEvent(EV_SOUTH))
            guidingNS = 2;
    }
    uint8_t trackComp    = (uint8_t)(0);
    uint8_t fault        = (uint8_t)(0);
    uint8_t pulseGuiding = (GuidingState == GuidingPulse || GuidingState == GuidingST4) ? 1u : 0u;
    pkt[2] = guidingEW | (guidingNS << 2) | (trackComp << 4) | (fault << 6) | (pulseGuiding << 7);

    // ── Byte 3: gnssFlags ────────────────────────────────────────────────────
    uint8_t gFlags = 0;
    bitWrite(gFlags, 0, hasGNSS);
    if (iSGNSSValid())
    {
      bitWrite(gFlags, 1, true);
      bitWrite(gFlags, 2, isTimeSyncWithGNSS());
      bitWrite(gFlags, 3, isLocationSyncWithGNSS());
      bitWrite(gFlags, 4, isHdopSmall());
    }
    pkt[3] = gFlags;

    // ── Byte 4: error ────────────────────────────────────────────────────────
    pkt[4] = (uint8_t)(lastError());

    // ── Byte 5: enableFlags | hasFocuser ─────────────────────────────────────
    uint8_t enableFlags = 0;
    bitWrite(enableFlags, 0, 0);
    bitWrite(enableFlags, 1, 0);
    bitWrite(enableFlags, 2, 0);
    bitWrite(enableFlags, 3, 1);    // enable Motors
    bitWrite(enableFlags, 4, 0);
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
       double dec = 0;
       double ra = 0;
       mount.mP->getEqu(&ra, &dec, localSite.cosLat(), localSite.sinLat(), false);

       HorCoords horCoords;
       mount.mP->getHorApp(&horCoords);

      double alt = horCoords.alt;
      double az  = AzRange(horCoords.az);
      double lst = rtk.LST();
      double trackRateRA = 1.0 - RequestedTrackingRateHA;
      double trackRateDec = RequestedTrackingRateDEC;
      gxasPackF64(pkt, 12, ra);
      gxasPackF64(pkt, 20, dec);
      gxasPackF64(pkt, 28, alt);
      gxasPackF64(pkt, 36, az);
      gxasPackF64(pkt, 44, lst);
      gxasPackF64(pkt, 52, newTargetRA);
      gxasPackF64(pkt, 60, newTargetDec);
      gxasPackF64(pkt, 68, trackRateRA);
      gxasPackF64(pkt, 76, trackRateDec);
    }

    // ── Bytes 84-91: stored tracking rates (int32 LE) ────────────────────────
    int32_t storedRateRA  = (int32_t) 0;    // not sure
    int32_t storedRateDec = (int32_t) 0;
    memcpy(pkt + 84, &storedRateRA, 4);
    memcpy(pkt + 88, &storedRateDec, 4);

    // ── Bytes 92-97: focuser (optional; when hasFocuser). Otherwise zero. ─────
    pkt[96] = (uint8_t) 0;
    pkt[97] = (uint8_t) 0;
    // ── Byte 98: timezone offset (toff × 10, int8_t) ───────────────────────────
    pkt[98] = (uint8_t)((int8_t)round(*localSite.toff() * 10.0f));

    // pkt[99-100] stay 0 (reserved)

    // Byte 101: XOR checksum of bytes 0-100
    uint8_t xorChk = 0;
    for (int i = 0; i < 101; i++) xorChk ^= pkt[i];
    pkt[101] = xorChk;

    // ── Base64 encode (102 bytes → 136 chars) → reply ────────────────────────
    gxasB64Encode(pkt, reply, GXAS_PKT_LEN);
    strcat(reply, "#");
}

void Command_GX_AllConfig()
{
    uint8_t pkt[90];
    memset(pkt, 0, sizeof(pkt));

    // ── Axis 1 Motor (offset 0) ──────────────────────────────────────────────
    gxcsPackAxis(pkt, 0, &motorA1);

    // ── Axis 2 Motor (offset 16) ─────────────────────────────────────────────
    gxcsPackAxis(pkt, 16, &motorA2);

    // ── Rates / Speed (offset 32) ────────────────────────────────────────────
    float fv;
    fv = (float)guideRates[0]; memcpy(pkt + 32, &fv, 4);
    fv = (float)guideRates[1]; memcpy(pkt + 36, &fv, 4);
    fv = (float)guideRates[2]; memcpy(pkt + 40, &fv, 4);
    fv = (float)guideRates[3]; memcpy(pkt + 44, &fv, 4);
    fv = (float)mount.DegreesForAcceleration; memcpy(pkt + 48, &fv, 4);
    gxcsPackU16(pkt, 52, (uint16_t)XEEPROM.readInt(getMountAddress(EE_maxRate)));
    pkt[54] = (uint8_t)XEEPROM.read(getMountAddress(EE_DefaultRate));
    pkt[55] = (uint8_t)XEEPROM.read(getMountAddress(EE_SlewSettleDuration));

    // ── Limits (offset 56) ───────────────────────────────────────────────────
    gxcsPackI16(pkt, 56, (int16_t)round(limits.minutesPastMeridianGOTOE));
    gxcsPackI16(pkt, 58, (int16_t)round(limits.minutesPastMeridianGOTOW));
    gxcsPackI16(pkt, 60, (int16_t)XEEPROM.readInt(getMountAddress(EE_minAxis1)));
    gxcsPackI16(pkt, 62, (int16_t)XEEPROM.readInt(getMountAddress(EE_maxAxis1)));
    gxcsPackI16(pkt, 64, (int16_t)XEEPROM.readInt(getMountAddress(EE_minAxis2)));
    gxcsPackI16(pkt, 66, (int16_t)XEEPROM.readInt(getMountAddress(EE_maxAxis2)));
    gxcsPackU16(pkt, 68, (uint16_t)round(limits.underPoleLimitGOTO * 10.0));
    pkt[70] = (uint8_t)(int8_t)limits.minAlt;
    pkt[71] = (uint8_t)(int8_t)limits.maxAlt;
    pkt[72] = (uint8_t) 0;
    uint8_t refrFlags = 0;
    bitWrite(refrFlags, 0, doesRefraction.forTracking);
    bitWrite(refrFlags, 1, doesRefraction.forGoto);
    bitWrite(refrFlags, 2, doesRefraction.forPole);
    pkt[73] = refrFlags;

    // ── Encoders (offset 74) ─────────────────────────────────────────────────
    gxcsPackU32(pkt, 74, (uint32_t) 100.0);
    gxcsPackU32(pkt, 78, (uint32_t) 100.0);
    pkt[82] = (uint8_t) 0;
    uint8_t encFlags = 0;
    bitWrite(encFlags, 0, 0);
    bitWrite(encFlags, 1, 0);
    pkt[83] = encFlags;

    // ── Options (offset 84) ──────────────────────────────────────────────────
    pkt[84] = (uint8_t)currentMount;
    // pkt[85-88] remain 0

    // ── XOR checksum (byte 89) ───────────────────────────────────────────────
    uint8_t xorChk = 0;
    for (int i = 0; i < 89; i++) xorChk ^= pkt[i];
    pkt[89] = xorChk;

    // ── Base64 encode (90 bytes → 120 chars) + '#' ───────────────────────────
    gxasB64Encode(pkt, reply, 90);
    strcat(reply, "#");
}

// :GXI#   Get telescope Status - legacy command
void Command_GX_Status()
{
  for (int i = 0; i < 50; i++)
    reply[i] = ' ';

  reply[0] = '0' + 2 * isSlewing() + isTracking();
  reply[1] = '0' + siderealMode;
  const char* parkStatusCh = "pIPF";
  reply[2] = parkStatusCh[parkStatus()];  // not [p]arked, parking [I]n-progress, [P]arked, Park [F]ailed
  if (atHome()) reply[3] = 'H';
  reply[4] = '0' + activeGuideRate;

  if (getEvent(EV_GUIDING_AXIS1) || getEvent(EV_GUIDING_AXIS2))
  {
    reply[5] = 'G';
    reply[6] = '*';
  }

  if (getEvent(EV_SPIRAL))
  {
    reply[5] = '@';
  }
  else if (getEvent(EV_CENTERING))
  {
    reply[5] = 'G';
    reply[6] = '+';
  }

  if (getEvent(EV_EAST))
    reply[7] = '<';
  else if (getEvent(EV_WEST))
    reply[7] = '>';
  if (getEvent(EV_NORTH))
    reply[8] = '^';
  else if (getEvent(EV_SOUTH))
    reply[8] = '_';

  reply[10] = '0' + trackComp;

  reply[11] = mount.mP->pm.alignment.isReady()? '1' : '0';
  if (mount.mP->type == MOUNT_TYPE_GEM)
  {
    reply[12] = 'E';
  }
  else if (mount.mP->type == MOUNT_TYPE_FORK)
    reply[12] = 'K';
  else if (mount.mP->type == MOUNT_TYPE_FORK_ALT)
    reply[12] = 'k';
  else if (mount.mP->type == MOUNT_TYPE_ALTAZM)
    reply[12] = 'A';
  else
    reply[12] = 'U';
  PierSide currentSide = mount.mP->GetPierSide();
  switch (currentSide)
  {
    case PIER_EAST: reply[13] = 'E'; break;
    case PIER_WEST: reply[13] = 'W'; break;
    default: reply[13] = '?'; break;
  }
  char val = 0;
  bitWrite(val, 0, hasGNSS);
  if (iSGNSSValid())
  {
    bitWrite(val, 1, true);
    bitWrite(val, 2, isTimeSyncWithGNSS());
    bitWrite(val, 3, isLocationSyncWithGNSS());
    bitWrite(val, 4, isHdopSmall());
  }
  reply[14] = 'A' + val;      // GNSS
  reply[15] = '0' + lastError();

  val = 0;
  bitWrite(val, 0, 0);    // no encoders
  bitWrite(val, 1, 0);    // not calibrating
  bitWrite(val, 2, 0);    // no pushto
  bitWrite(val, 3, 1);    // motors enabled

  reply[16] = 'A' + val;
  reply[17] = '#';
  reply[18] = 0;
}




void Command_GX_Alignment()
{
    float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
    if (mount.mP->pm.alignment.isReady())
    {
      mount.mP->pm.alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    }
    switch (command[3])
    {
        case '0': sprintf(reply, "%f#", t11); break;
        case '1': sprintf(reply, "%f#", t12); break;
        case '2': sprintf(reply, "%f#", t13); break;
        case '3': sprintf(reply, "%f#", t21); break;
        case '4': sprintf(reply, "%f#", t22); break;
        case '5': sprintf(reply, "%f#", t23); break;
        case '6': sprintf(reply, "%f#", t31); break;
        case '7': sprintf(reply, "%f#", t32); break;
        case '8': sprintf(reply, "%f#", t33); break;
        default:  replyLongUnknown(); break;
    }
}

void Command_GX_Encoders()
{
}
void Command_GX_Debug()
{
    switch (command[3])
    {
    case 'B':
    {
      // :GXDBn# Debug Backlash
      switch (command[4])
      {
        case '0': sprintf(reply, "%d#", (int)mount.backlashA1.correcting); break;
        case '1': sprintf(reply, "%d#", (int)mount.backlashA2.correcting); break;
        case '2': sprintf(reply, "%d#", (int)mount.backlashA1.movedSteps); break;
        case '3': sprintf(reply, "%d#", (int)mount.backlashA2.movedSteps); break;
        default:  replyLongUnknown(); break;
      }
      break;
    }
    case 'R':
    {
      // :GXDRn# Debug Rates
      switch (command[4])
      {
        case '1': sprintf(reply, "%f#", 0.0); break;
        case '2': sprintf(reply, "%f#", 0.0); break;
        case '3': sprintf(reply, "%f#", motorA1.getSpeed()); break;
        case '4': sprintf(reply, "%f#", motorA2.getSpeed()); break;
        case '5': sprintf(reply, "%f#", (double)0); break;
        case '6': sprintf(reply, "%f#", (double)0); break;
        case '7': sprintf(reply, "%f#", (double)0); break;
        case '8': sprintf(reply, "%f#", (double)0); break;
        case '9': sprintf(reply, "%f#", 0.0); break;
        case 'A': sprintf(reply, "%lf#", 0.0); break;
        case 'B': sprintf(reply, "%lf#", 0.0); break;
        default:  replyLongUnknown(); break;
      }
      break;
    }
    case 'P':
    {
      // :GXDPn# Debug Position and Target
      long temp;
      switch (command[4])
      {
        case '0': temp = motorA1.getCurrentPos();  sprintf(reply, "%ld#", temp); break;
        case '1': temp = motorA2.getCurrentPos();  sprintf(reply, "%ld#", temp); break;
        case '2': temp = motorA1.getTargetPos();  sprintf(reply, "%ld#", temp); break;
        case '3': temp = motorA2.getTargetPos();  sprintf(reply, "%ld#", temp); break;
        default:  replyLongUnknown(); break;
      }
      break;
    }
    case 'W':
    {
      // :GXDW# Get workload   :GXDW1# Get missed sidereal ticks (debug)
      if (command[4] == 0)
      {
        sprintf(reply, "%ld%%#", 0L);
      }
      else if (command[4] == '1')
      {
        sprintf(reply, "%ld#", 0L);
      }
      else if (command[4] == '2')
      {
        replyLongTrue();
      }
      else
        replyLongUnknown();
      break;
    }
    break;
    default:
      replyLongUnknown();
      break;
    }
}

void Command_GX_Position()
{
    double f1;
    switch (command[3])
    {
        case '1':
            f1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;
            doubleToDms(reply, &f1, true, true, highPrecision);
            strcat(reply, "#");
            break;
        case '2':
            f1 = motorA2.getCurrentPos() / geoA2.stepsPerDegree;
            doubleToDms(reply, &f1, true, true, highPrecision);
            strcat(reply, "#");
            break;
        default:
            replyLongUnknown();
        break;
    }
}
void Command_GX_Refraction()
{
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
        replyLongUnknown();
        break;
    }
}

void Command_GX_Clock5160()
{
    sprintf(reply, "%ld#", (long) mount.clk5160);
}

void Command_GX_Rates()
{
    switch (command[3])
    {
        case '0':
        case '1':
        case '2':
        case '3':
        {
            int i = command[3] - '0';
            // :GXRn# return user defined rate
            dtostrf(guideRates[i], 2, 2, reply);
            strcat(reply, "#");

        }
        break;
        case 'A':
            // :GXRA# returns the Degrees For Acceleration
            dtostrf(mount.DegreesForAcceleration, 2, 1, reply);
            strcat(reply, "#");
            break;
        case 'B':
            // :GXRB# returns the Backlash Take up Rate
            sprintf(reply, "%ld#", (long)round(BacklashTakeupRate));
            break;
        case 'D':
            // :GXRD# returns the Default Rate
            sprintf(reply, "%d#", XEEPROM.read(getMountAddress(EE_DefaultRate)));
            break;
        case 'X':
            // :GXRX# return Max Slew rate
            sprintf(reply, "%d#", XEEPROM.readInt(getMountAddress(EE_maxRate)));
            break;
        default:
            replyLongUnknown();
        break;
    }
}

void Command_GX_Limits()
{
    int i;
    switch (command[3])
    {
        case 'A':
        // :GXLA# get user defined minAXIS1 (always negative, store absolute value)
        i = XEEPROM.readInt(getMountAddress(EE_minAxis1));
        sprintf(reply, "%d#", -i);
        break;
        case 'B':
        // :GXLB# get user defined maxAXIS1 (always positive)
        i = XEEPROM.readInt(getMountAddress(EE_maxAxis1));
        sprintf(reply, "%d#", i);
        break;
        case 'C':
        // :GXLC# get user defined minAXIS2 (always negative)
        i = XEEPROM.readInt(getMountAddress(EE_minAxis2));
        sprintf(reply, "%d#", -i);
        break;
        case 'D':
        // :GXLD# get user defined maxAXIS2 (always positive)
        i = XEEPROM.readInt(getMountAddress(EE_maxAxis2));
        sprintf(reply, "%d#", i);
        break;
        case 'E':
        // :GXLE# return user defined Meridian East Limit
        sprintf(reply, "%ld#", (long)round(limits.minutesPastMeridianGOTOE));
        break;
        case 'W':
        // :GXLW# return user defined Meridian West Limit
        sprintf(reply, "%ld#", (long)round(limits.minutesPastMeridianGOTOW));
        break;
        case 'U':
        // :GXLU# return user defined Under pole Limit
        sprintf(reply, "%ld#", (long)round(limits.underPoleLimitGOTO * 10));
        break;
        case 'O':
        // :GXLO# return user defined horizon Limit
        // NB: duplicate with :Go#
        sprintf(reply, "%+02d*#", limits.maxAlt);
        break;
        case 'H':
        // :GXLH# return user defined horizon Limit
        // NB: duplicate with :Gh#
        sprintf(reply, "%+02d*#", limits.minAlt);
        break;
        default:
            replyLongUnknown();
        break;
    }
}

void Command_GX_MountLimits()
{
    int i;
    switch (command[3])
    {
    case 'A':  i = -360;  sprintf(reply, "%d#", i);   break;
    case 'B':  i = 360;   sprintf(reply, "%d#", i);   break;
    case 'C':  i = -360;  sprintf(reply, "%d#", i);   break;
    case 'D':  i = 360;   sprintf(reply, "%d#", i);   break;
    default: replyLongUnknown();      break;
    }
}

void Command_GX_Time()
{
    int i;
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
        mount.mP->getEqu(&f, &f1, localSite.cosLat(), localSite.sinLat(), false);
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
}

void Command_GX_Motors()
{
    switch (command[3])
    {
        case 'B':
        {
        // :GXMB.#   Get Motor backlash
        if (command[4] == 'D')
        {
            sprintf(reply, "%d#", mount.backlashA1.inSeconds);
        }
        else if (command[4] == 'R')
        {
            sprintf(reply, "%d#", mount.backlashA2.inSeconds);
        }
        else
            replyLongUnknown();
        }
        break;
        case 'G':
        {
        // :GXMG.#   Get Motor Gear
        if (command[4] == 'D')
        {
            sprintf(reply, "%lu#", motorA2.gear);
        }
        else if (command[4] == 'R')
        {
            sprintf(reply, "%lu#", motorA1.gear);
        }
        else
            replyLongUnknown();
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
            replyLongUnknown();
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
            replyLongUnknown();
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
            replyLongUnknown();
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
            replyLongUnknown();
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
            replyLongUnknown();
        }
        break;
        case 'c':
        {
          // :GXMc.#   Get Motor LowCurrent in mA
          if (command[4] == 'D')
          {
            sprintf(reply, "%u#", (unsigned int)motorA2.lowCurr);
          }
          else if (command[4] == 'R')
          {
            sprintf(reply, "%u#", (unsigned int)motorA1.lowCurr);
          }
          else
            replyLongUnknown();
        }
        break;
        default:
          replyLongUnknown();
          break;
    }
}

void Command_GX_Options()
{
    switch (command[3])
    {
        case 'I': sprintf(reply, "%d#", currentMount); break;               // :GXOI# Mount idx
        case 'A': sprintf(reply, "%s#", mountNames[currentMount]); break;   // :GXOA# Mount Name
        case 'B': sprintf(reply, "%s#", mountNames[0]);  break;             // :GXOB# first Mount Name
        case 'C': sprintf(reply, "%s#", mountNames[1]);  break;
        case 'S': // :GXOS# get Slew Settle Duration in seconds
        {
            int i = XEEPROM.read(getMountAddress(EE_SlewSettleDuration));
            sprintf(reply, "%d#", i);
        }
        break;
    }
}

void Command_GX() {
  switch (command[2])
  {
  case 'A':
    if (command[3] == 'S') Command_GX_AllState();
    else                                Command_GX_Alignment();
    break;
  case 'C':
    if (command[3] == 'S') Command_GX_AllConfig();
    else                                replyLongUnknown();
    break;
  case 'E': Command_GX_Encoders();    break;
  case 'D': Command_GX_Debug();       break;
  case 'I': Command_GX_Status();      break;
  case 'P': Command_GX_Position();    break;
  case 'r': Command_GX_Refraction();  break;
  case 'K': Command_GX_Clock5160();   break;
  case 'R': Command_GX_Rates();       break;
  case 'L': Command_GX_Limits();      break;
  case 'l': Command_GX_MountLimits(); break;
  case 'T': Command_GX_Time();        break;
  case 'M': Command_GX_Motors();      break;
  case 'O': Command_GX_Options();     break;
  default:  replyLongUnknown();        break;
  }
}
