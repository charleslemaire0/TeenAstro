#include <TeenAstroMountStatus.h>

#define UPDATERATE 500

// Reject NaN/Inf and out-of-range values so format functions don't crash (e.g. on embedded snprintf).
static bool safeFloat(float f)
{
  if (f != f) return false;  /* NaN */
  if (f > 1e15f || f < -1e15f) return false;  /* Inf or absurd */
  return true;
}


// ===========================================================================
//  Helpers for updateAllState()
// ===========================================================================

// Base64 decode table (index by ASCII value; -1 = invalid).
static const int8_t B64DEC[128] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
  52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
  15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
  -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
};

// Decode `inLen` base64 chars into `out` (caller must supply inLen/4*3 bytes).
// Returns false on invalid input character.
static bool b64Decode(const char* in, int inLen, uint8_t* out)
{
  if (inLen % 4 != 0) return false;
  int o = 0;
  for (int i = 0; i < inLen; i += 4)
  {
    uint8_t c0 = (uint8_t)in[i];
    uint8_t c1 = (uint8_t)in[i + 1];
    uint8_t c2 = (uint8_t)in[i + 2];
    uint8_t c3 = (uint8_t)in[i + 3];
    if (c0 > 127 || c1 > 127 || c2 > 127 || c3 > 127) return false;
    int8_t v0 = B64DEC[c0], v1 = B64DEC[c1], v2 = B64DEC[c2], v3 = B64DEC[c3];
    if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;
    uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) |
                 ((uint32_t)v2 << 6)  | (uint32_t)v3;
    out[o++] = (uint8_t)(b >> 16);
    out[o++] = (uint8_t)(b >> 8);
    out[o++] = (uint8_t)(b);
  }
  return true;
}

// Format RA float (hours) into "HH:MM:SS.ss" (2 decimal places for seconds).
// Safe: normalises to [0,24) using fmod even for very out-of-range values.
static void formatRaStr(float h, char* out, int len)
{
  if (len < 1) return;
  h = fmodf(h, 24.0f);
  if (h < 0.0f) h += 24.0f;
  int ih = (int)h;
  float mf = (h - ih) * 60.0f;
  int im = (int)mf;
  float secs = (mf - im) * 60.0f;
  if (secs >= 59.995f) { secs = 0.0f; im++; }
  if (im >= 60) { im -= 60; ih++; }
  snprintf(out, len, "%02d:%02d:%05.2f", ih % 24, im % 60, (double)secs);
}

// Format signed degrees (Dec/Alt) into "±DD*MM:SS.s" (1 decimal for seconds).
// Uses '*' for compatibility with SHC display (single-byte); web can substitute ° when rendering.
// Safe: clamps absolute value to [0,360) so output never overflows the buffer.
static void formatDegStr(float deg, char* out, int len)
{
  if (len < 1) return;
  char sign = (deg >= 0.0f) ? '+' : '-';
  float ad = fabsf(deg);
  ad = fmodf(ad, 360.0f);
  int id = (int)ad;
  float mf = (ad - id) * 60.0f;
  int im = (int)mf;
  float secs = (mf - im) * 60.0f;
  if (secs >= 59.95f) { secs = 0.0f; im++; }
  if (im >= 60) { im -= 60; id++; }
  snprintf(out, len, "%c%02d*%02d:%04.1f", sign, id % 360, im % 60, (double)secs);
}

// Format azimuth (degrees) into "DDD*MM:SS.s" (1 decimal for seconds).
// Uses '*' for compatibility with SHC display (single-byte); web can substitute ° when rendering.
// Safe: uses fmod instead of while-loop so can't hang on extreme values.
static void formatAzStr(float deg, char* out, int len)
{
  if (len < 1) return;
  deg = fmodf(deg, 360.0f);
  if (deg < 0.0f) deg += 360.0f;
  int id = (int)deg;
  float mf = (deg - id) * 60.0f;
  int im = (int)mf;
  float secs = (mf - im) * 60.0f;
  if (secs >= 59.95f) { secs = 0.0f; im++; }
  if (im >= 60) { im -= 60; id++; }
  snprintf(out, len, "%03d*%02d:%04.1f", id % 360, im % 60, (double)secs);
}

// Read float32 LE from packet at offset.
static float pktF32(const uint8_t* pkt, int off)
{
  float v;
  memcpy(&v, pkt + off, 4);
  return v;
}

// Read uint32 LE from packet at offset.
static uint32_t pktU32(const uint8_t* pkt, int off)
{
  return (uint32_t)pkt[off] | ((uint32_t)pkt[off+1] << 8) |
         ((uint32_t)pkt[off+2] << 16) | ((uint32_t)pkt[off+3] << 24);
}

// Read uint16 LE from packet at offset.
static uint16_t pktU16(const uint8_t* pkt, int off)
{
  return (uint16_t)pkt[off] | ((uint16_t)pkt[off+1] << 8);
}

// ===========================================================================
//  MountState — parse from raw :GXI# string
// ===========================================================================
//
//  Position map of the raw status string:
//    [0]  tracking state   '0'=off '1'=on '2'/'3'=slewing
//    [1]  sidereal mode    '0'..'3'
//    [2]  park state       'P'=parked 'p'=unparked 'I'=parking 'F'=failed
//    [3]  at home          'H' = yes
//    [4]  guiding rate     '0'..'4'
//    [5]  spiral           '@' = running
//    [6]  pulse guiding    '*' = active
//    [7]  guiding E/W      '>' east  '<' west
//    [8]  guiding N/S      '^' north '_' south
//    [9]  (reserved)
//    [10] rate comp / corrected  '1'=RA '2'=BOTH or 'c'=corrected
//    [11] aligned           '1' = yes
//    [12] mount type        'E'=GEM 'K'=Fork 'A'=AltAz 'k'=ForkAlt 'U'=undef
//    [13] pier side         'E' / 'W' / ' '
//    [14] GNSS flags        bitfield encoded as char - 'A'
//    [15] error code        '0'..'8'
//    [16] enable flags      bitfield encoded as char - 'A'

void MountState::parseFrom(const char* raw)
{
  valid = (raw != nullptr && strlen(raw) >= 17);
  if (!valid) return;

  // [0] tracking
  switch (raw[0]) {
    case '3': case '2': tracking = TRK_SLEWING; break;
    case '1': tracking = TRK_ON;  break;
    case '0': tracking = TRK_OFF; break;
    default:  tracking = TRK_UNKNOW; break;
  }

  // [1] sidereal mode
  if (raw[1] >= '0' && raw[1] <= '3')
    sidereal = static_cast<SiderealMode>(raw[1] - '0');
  else
    sidereal = SID_UNKNOWN;

  // [2] park state
  switch (raw[2]) {
    case 'P': parkState = PRK_PARKED;   break;
    case 'p': parkState = PRK_UNPARKED; break;
    case 'I': parkState = PRK_PARKING;  break;
    case 'F': parkState = PRK_FAILED;   break;
    default:  parkState = PRK_UNKNOW;   break;
  }

  // [3] at home
  atHome = (raw[3] == 'H');

  // [4] guiding rate
  { int v = raw[4] - '0';
    guidingRate = (v >= 0 && v <= 4)
      ? static_cast<GuidingRate>(v) : UNKNOW;
  }

  // [5] spiral
  spiralRunning = (raw[5] == '@');

  // [6] pulse guiding
  pulseGuiding = (raw[6] == '*');

  // [7] guiding E/W
  guidingEW = raw[7];

  // [8] guiding N/S
  guidingNS = raw[8];

  // [10] rate comp / track corrected
  trackCorrected = (raw[10] == 'c');
  { int v = raw[10] - '0';
    rateComp = (v > 0 && v < 3)
      ? static_cast<RateCompensation>(v) : RC_UNKNOWN;
  }

  // [11] aligned
  aligned = (raw[11] == '1');

  // [12] mount type
  switch (raw[12]) {
    case 'E': mountType = MOUNT_TYPE_GEM;      break;
    case 'K': mountType = MOUNT_TYPE_FORK;     break;
    case 'A': mountType = MOUNT_TYPE_ALTAZM;   break;
    case 'k': mountType = MOUNT_TYPE_FORK_ALT; break;
    default:  mountType = MOUNT_UNDEFINED;     break;
  }

  // [13] pier side
  switch (raw[13]) {
    case 'E': pierSide = PIER_E;      break;
    case 'W': pierSide = PIER_W;      break;
    default:  pierSide = PIER_UNKNOW; break;
  }

  // [14] GNSS flags
  gnssFlags = (uint8_t)(raw[14] - 'A');

  // [15] error code
  { int e = raw[15] - '0';
    if (e >= 1 && e <= 8)
      error = static_cast<Errors>(e);
    else
      error = ERR_NONE;
  }

  // [16] enable flags
  enableFlags = (uint8_t)(raw[16] - 'A');
}

// ===========================================================================
//  Alignment state machine
// ===========================================================================

void TeenAstroMountStatus::nextStepAlign()
{
  if (!isAligning()) return;
  if (isAlignSelect())   { m_align = ALI_SLEW;     return; }
  if (isAlignSlew())     { m_align = ALI_RECENTER;  return; }
  if (isAlignRecenter()) { m_alignStar++; m_align = ALI_SELECT; return; }
}

void TeenAstroMountStatus::backStepAlign()
{
  if (!isAligning()) return;
  if (isAlignSelect())   { stopAlign();             return; }
  if (isAlignSlew())     { m_align = ALI_SELECT;    return; }
  if (isAlignRecenter()) { m_align = ALI_SELECT;    return; }
}

TeenAstroMountStatus::AlignReply TeenAstroMountStatus::addStar()
{
  if (!isAlignRecenter())
  {
    stopAlign();
    return AlignReply::ALIR_FAILED2;
  }

  // OnStepX-style: :A1# first star, :A2# second, :A3# third, etc.
  uint8_t n = (uint8_t)getAlignStar();
  if (n < 1 || n > 9) { stopAlign(); return AlignReply::ALIR_FAILED2; }

  if (m_client->alignSelectStar(n) == LX200_VALUESET)
  {
    if (isLastStarAlign()) { stopAlign(); return AlignReply::ALIR_DONE; }
    else                   { nextStepAlign(); return AlignReply::ALIR_ADDED; }
  }
  else
  {
    stopAlign();
    return AlignReply::ALIR_FAILED1;
  }
}

// ===========================================================================
//  Update methods
// ===========================================================================

void TeenAstroMountStatus::updateV()
{
  if (!m_version.valid)
  {
    bool ok = m_vp.fetch(*m_client, &LX200Client::getProductName);
    ok = ok && !strcmp(m_vp.data, "TeenAstro");
    ok = ok && m_vn.fetch(*m_client, &LX200Client::getVersionNumber);
    ok = ok && m_vd.fetch(*m_client, &LX200Client::getVersionDate);
    ok = ok && m_vb.fetch(*m_client, &LX200Client::getBoardVersion);
    ok = ok && m_vbb.fetch(*m_client, &LX200Client::getDriverType);
    m_version.valid = ok;
    if (!ok) m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateRaDec()
{
  if (m_timerRaDec.needsUpdate(UPDATERATE))
  {
    m_ra.fetch(*m_client, &LX200Client::getRaStr);
    m_dec.fetch(*m_client, &LX200Client::getDecStr);
    if (m_ra.valid && m_dec.valid)
      m_timerRaDec.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateHaDec()
{
  if (m_timerHaDec.needsUpdate(UPDATERATE))
  {
    m_ha.fetch(*m_client, &LX200Client::getHaStr);
    m_dec.fetch(*m_client, &LX200Client::getDecStr);
    if (m_ha.valid && m_dec.valid)
      m_timerHaDec.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateRaDecT()
{
  if (m_timerRaDecT.needsUpdate(UPDATERATE))
  {
    m_raT.fetch(*m_client, &LX200Client::getTargetRaStr);
    m_decT.fetch(*m_client, &LX200Client::getTargetDecStr);
    if (m_raT.valid && m_decT.valid)
      m_timerRaDecT.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateAzAlt()
{
  if (m_timerAzAlt.needsUpdate(UPDATERATE))
  {
    m_az.fetch(*m_client, &LX200Client::getAzStr);
    m_alt.fetch(*m_client, &LX200Client::getAltStr);
    if (m_az.valid && m_alt.valid)
      m_timerAzAlt.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updatePush()
{
  if (encodersEnable() && m_timerPush.needsUpdate(UPDATERATE))
  {
    m_push.fetch(*m_client, &LX200Client::getEncoderDelta);
    if (m_push.valid)
    {
      m_push.data[1] = 0;
      m_push.data[8] = 0;
      m_timerPush.markUpdated();
    }
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateAxisStep()
{
  if (m_timerAxisStep.needsUpdate(UPDATERATE))
  {
    m_axis1Step.valid = (m_client->getAxisSteps(1, m_axis1Step.data, sizeof(m_axis1Step.data)) == LX200_VALUEGET);
    m_axis2Step.valid = (m_client->getAxisSteps(2, m_axis2Step.data, sizeof(m_axis2Step.data)) == LX200_VALUEGET);
    if (m_axis1Step.valid && m_axis2Step.valid)
      m_timerAxisStep.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateAxisDeg()
{
  if (m_timerAxisDeg.needsUpdate(UPDATERATE))
  {
    m_axis1Deg.valid = (m_client->getAxisDegrees(1, m_axis1Deg.data, sizeof(m_axis1Deg.data)) == LX200_VALUEGET);
    m_axis2Deg.valid = (m_client->getAxisDegrees(2, m_axis2Deg.data, sizeof(m_axis2Deg.data)) == LX200_VALUEGET);

    if (encodersEnable())
    {
      m_axis1EDeg.valid = (m_client->getEncoderDegrees(1, m_axis1EDeg.data, sizeof(m_axis1EDeg.data)) == LX200_VALUEGET);
      m_axis2EDeg.valid = (m_client->getEncoderDegrees(2, m_axis2EDeg.data, sizeof(m_axis2EDeg.data)) == LX200_VALUEGET);
      if (m_axis1Deg.valid && m_axis2Deg.valid && m_axis1EDeg.valid && m_axis2EDeg.valid)
        m_timerAxisDeg.markUpdated();
      else
        m_connectionFailure++;
    }
    else
    {
      m_axis1Degc.valid = (m_client->getAxisDegreesCorr(1, m_axis1Degc.data, sizeof(m_axis1Degc.data)) == LX200_VALUEGET);
      m_axis2Degc.valid = (m_client->getAxisDegreesCorr(2, m_axis2Degc.data, sizeof(m_axis2Degc.data)) == LX200_VALUEGET);
      if (m_axis1Deg.valid && m_axis2Deg.valid && m_axis1Degc.valid && m_axis2Degc.valid)
        m_timerAxisDeg.markUpdated();
      else
        m_connectionFailure++;
    }
  }
}

void TeenAstroMountStatus::updateTime()
{
  if (m_timerTime.needsUpdate(UPDATERATE))
  {
    m_utc.fetch(*m_client, &LX200Client::getUTCTimeStr);
    m_utcDate.fetch(*m_client, &LX200Client::getUTCDateStr);
    m_sidereal.fetch(*m_client, &LX200Client::getSiderealStr);
    if (m_utc.valid && m_utcDate.valid && m_sidereal.valid)
      m_timerTime.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateLHA()
{
  if (m_timerTime.needsUpdate(UPDATERATE))
  {
    m_utc.fetch(*m_client, &LX200Client::getUTCTimeStr);
    m_lha.fetch(*m_client, &LX200Client::getHaStr);
    if (m_utc.valid && m_lha.valid)
      m_timerTime.markUpdated();
    else
      m_connectionFailure++;
  }
}

void TeenAstroMountStatus::updateMount(bool force)
{
  if (m_timerMount.needsUpdate(UPDATERATE) || force)
  {
    char raw[20];
    if (m_client->getMountStateRaw(raw, sizeof(raw)) == LX200_VALUEGET)
    {
      m_mount.parseFrom(raw);
      m_timerMount.markUpdated();
    }
    else
    {
      m_mount.valid = false;
      m_connectionFailure++;
    }
  }
}

// ===========================================================================
//  All-state bulk update  (:GXAS#)
// ===========================================================================

void TeenAstroMountStatus::invalidatePositionTimeCaches()
{
  m_ra.valid = m_dec.valid = m_alt.valid = m_az.valid = false;
  m_sidereal.valid = m_raT.valid = m_decT.valid = false;
  m_utc.valid = m_utcDate.valid = false;
  strncpy(m_ra.data, "?", sizeof(m_ra.data) - 1);      m_ra.data[sizeof(m_ra.data) - 1] = '\0';
  strncpy(m_dec.data, "?", sizeof(m_dec.data) - 1);    m_dec.data[sizeof(m_dec.data) - 1] = '\0';
  strncpy(m_alt.data, "?", sizeof(m_alt.data) - 1);    m_alt.data[sizeof(m_alt.data) - 1] = '\0';
  strncpy(m_az.data, "?", sizeof(m_az.data) - 1);      m_az.data[sizeof(m_az.data) - 1] = '\0';
  strncpy(m_sidereal.data, "?", sizeof(m_sidereal.data) - 1); m_sidereal.data[sizeof(m_sidereal.data) - 1] = '\0';
  strncpy(m_raT.data, "?", sizeof(m_raT.data) - 1);   m_raT.data[sizeof(m_raT.data) - 1] = '\0';
  strncpy(m_decT.data, "?", sizeof(m_decT.data) - 1); m_decT.data[sizeof(m_decT.data) - 1] = '\0';
  strncpy(m_utc.data, "?", sizeof(m_utc.data) - 1);    m_utc.data[sizeof(m_utc.data) - 1] = '\0';
  strncpy(m_utcDate.data, "?", sizeof(m_utcDate.data) - 1); m_utcDate.data[sizeof(m_utcDate.data) - 1] = '\0';
}

void TeenAstroMountStatus::updateAllState(bool force)
{
  if (!m_timerAllState.needsUpdate(UPDATERATE) && !force) return;

  // Fetch the 88-char base64 response (no '#' in returned string).
  char raw[92] = "";
  if (m_client->get(":GXAS#", raw, sizeof(raw)) != LX200_VALUEGET || strlen(raw) != 88)
  {
    m_mount.valid = false;
    invalidatePositionTimeCaches();
    m_connectionFailure++;
    return;
  }

  // Decode to 66 bytes.
  uint8_t pkt[66];
  if (!b64Decode(raw, 88, pkt))
  {
    m_mount.valid = false;
    invalidatePositionTimeCaches();
    m_connectionFailure++;
    return;
  }

  // Verify XOR checksum: XOR of bytes 0–64 must equal byte 65.
  uint8_t xorChk = 0;
  for (int i = 0; i < 65; i++) xorChk ^= pkt[i];
  if (xorChk != pkt[65])
  {
    m_mount.valid = false;
    invalidatePositionTimeCaches();
    m_connectionFailure++;
    return;
  }

  // Cache the raw string (add '#' for WiFi bridge forwarding).
  snprintf(m_allStateB64, sizeof(m_allStateB64), "%s#", raw);
  m_timerAllState.markUpdated();

  // ── Status bytes 0-5 → m_mount ────────────────────────────────────────
  uint8_t b0 = pkt[0], b1 = pkt[1], b2 = pkt[2];
  switch (b0 & 0x3) {
    case 0: m_mount.tracking = MountState::TRK_OFF;     break;
    case 1: m_mount.tracking = MountState::TRK_ON;      break;
    default: m_mount.tracking = MountState::TRK_SLEWING; break;
  }
  { int v = (b0 >> 2) & 0x3;
    m_mount.sidereal = (v >= 0 && v <= 3)
      ? static_cast<MountState::SiderealMode>(v) : MountState::SID_UNKNOWN; }
  { int v = (b0 >> 4) & 0x3;
    switch (v) {
      case 0: m_mount.parkState = MountState::PRK_UNPARKED; break;
      case 1: m_mount.parkState = MountState::PRK_PARKING;  break;
      case 2: m_mount.parkState = MountState::PRK_PARKED;   break;
      case 3: m_mount.parkState = MountState::PRK_FAILED;   break;
      default: m_mount.parkState = MountState::PRK_UNKNOW;  break;
    }
  }
  m_mount.atHome        = (b0 >> 6) & 0x1;
  m_mount.pierSide      = ((b0 >> 7) & 0x1) ? MountState::PIER_W : MountState::PIER_E;

  { int gr = (int)(b1 & 0x7);
    m_mount.guidingRate = (gr >= 0 && gr <= 4)
      ? static_cast<MountState::GuidingRate>(gr) : MountState::UNKNOW; }
  m_mount.aligned       = (b1 >> 3) & 0x1;
  { uint8_t mt = (b1 >> 4) & 0x7;
    switch (mt) {
      case 1: m_mount.mountType = MountState::MOUNT_TYPE_GEM;      break;
      case 2: m_mount.mountType = MountState::MOUNT_TYPE_FORK;     break;
      case 3: m_mount.mountType = MountState::MOUNT_TYPE_ALTAZM;   break;
      case 4: m_mount.mountType = MountState::MOUNT_TYPE_FORK_ALT; break;
      default: m_mount.mountType = MountState::MOUNT_UNDEFINED;    break;
    }
  }
  m_mount.spiralRunning = (b1 >> 7) & 0x1;

  { uint8_t ew = b2 & 0x3;
    m_mount.guidingEW = (ew == 1) ? '>' : (ew == 2) ? '<' : (ew == 3) ? 'b' : ' '; }
  { uint8_t ns = (b2 >> 2) & 0x3;
    m_mount.guidingNS = (ns == 1) ? '^' : (ns == 2) ? '_' : (ns == 3) ? 'b' : ' '; }
  { int rc = (int)((b2 >> 4) & 0x3);
    m_mount.rateComp = (rc == 1) ? MountState::RC_RA : (rc == 2) ? MountState::RC_BOTH : MountState::RC_UNKNOWN; }
  m_mount.trackCorrected = (m_mount.rateComp != MountState::RC_UNKNOWN);
  m_mount.pulseGuiding  = (b2 >> 7) & 0x1;

  m_mount.gnssFlags   = pkt[3];
  { int e = pkt[4];
    m_mount.error = (e >= 0 && e <= 8)
      ? static_cast<MountState::Errors>(e) : MountState::ERR_NONE; }
  uint8_t eFlags   = pkt[5];
  m_mount.enableFlags = eFlags & 0x0F;
  m_hasFocuser      = (eFlags >> 4) & 0x1;
  m_mount.valid     = true;
  m_timerMount.markUpdated();

  // ── UTC bytes 6-11 ────────────────────────────────────────────────────
  m_utcH     = pkt[6];
  m_utcM     = pkt[7];
  m_utcS     = pkt[8];
  m_utcMonth = pkt[9];
  m_utcDay   = pkt[10];
  m_utcYear  = pkt[11];
  snprintf(m_utc.data,     sizeof(m_utc.data),     "%02d:%02d:%02d", m_utcH, m_utcM, m_utcS);
  snprintf(m_utcDate.data, sizeof(m_utcDate.data), "%02d/%02d/%02d", m_utcMonth, m_utcDay, m_utcYear);
  m_utc.valid = m_utcDate.valid = true;

  // ── Byte 62: timezone offset ────────────────────────────────────────
  m_tzOff10 = (int8_t)pkt[62];
  m_tzValid = true;

  // ── Positions bytes 12-39 ─────────────────────────────────────────────
  float ra  = pktF32(pkt, 12);
  float dec = pktF32(pkt, 16);
  float alt = pktF32(pkt, 20);
  float az  = pktF32(pkt, 24);
  float lst = pktF32(pkt, 28);
  float tRA = pktF32(pkt, 32);
  float tDec= pktF32(pkt, 36);

  if (!safeFloat(ra))   { strncpy(m_ra.data, "?", sizeof(m_ra.data) - 1); m_ra.data[sizeof(m_ra.data) - 1] = '\0'; }
  else                  formatRaStr(ra,   m_ra.data,       sizeof(m_ra.data));
  if (!safeFloat(dec))  { strncpy(m_dec.data, "?", sizeof(m_dec.data) - 1); m_dec.data[sizeof(m_dec.data) - 1] = '\0'; }
  else                  formatDegStr(dec, m_dec.data,      sizeof(m_dec.data));
  if (!safeFloat(alt))  { strncpy(m_alt.data, "?", sizeof(m_alt.data) - 1); m_alt.data[sizeof(m_alt.data) - 1] = '\0'; }
  else                  formatDegStr(alt, m_alt.data,      sizeof(m_alt.data));
  if (!safeFloat(az))   { strncpy(m_az.data, "?", sizeof(m_az.data) - 1); m_az.data[sizeof(m_az.data) - 1] = '\0'; }
  else                  formatAzStr(az,   m_az.data,       sizeof(m_az.data));
  if (!safeFloat(lst))  { strncpy(m_sidereal.data, "?", sizeof(m_sidereal.data) - 1); m_sidereal.data[sizeof(m_sidereal.data) - 1] = '\0'; }
  else                  formatRaStr(lst,  m_sidereal.data, sizeof(m_sidereal.data));
  if (!safeFloat(tRA))  { strncpy(m_raT.data, "?", sizeof(m_raT.data) - 1); m_raT.data[sizeof(m_raT.data) - 1] = '\0'; }
  else                  formatRaStr(tRA,  m_raT.data,      sizeof(m_raT.data));
  if (!safeFloat(tDec)) { strncpy(m_decT.data, "?", sizeof(m_decT.data) - 1); m_decT.data[sizeof(m_decT.data) - 1] = '\0'; }
  else                  formatDegStr(tDec,m_decT.data,     sizeof(m_decT.data));
  m_ra.valid = m_dec.valid = m_alt.valid = m_az.valid = true;
  m_sidereal.valid = m_raT.valid = m_decT.valid = true;
  m_timerRaDec.markUpdated();
  m_timerAzAlt.markUpdated();
  m_timerTime.markUpdated();
  m_timerRaDecT.markUpdated();

  // ── Bytes 40-55: tracking rates (int32 LE, same as :GXRr#/:GXRd#/:GXRe#/:GXRf#)
  m_trackRateRa        = (long)(int32_t)pktU32(pkt, 40);
  m_trackRateDec       = (long)(int32_t)pktU32(pkt, 44);
  m_storedTrackRateRa  = (long)(int32_t)pktU32(pkt, 48);
  m_storedTrackRateDec = (long)(int32_t)pktU32(pkt, 52);
  m_hasInfoTrackingRate = true;
  m_timerTrackRate.markUpdated();

  // ── Bytes 56-61: focuser (optional; when hasFocuser)
  if (m_hasFocuser)
  {
    m_focuserPosN   = pktU32(pkt, 56);
    m_focuserSpeedN = pktU16(pkt, 60);
    snprintf(m_focuser.data, sizeof(m_focuser.data),
             "?%05lu %03u", (unsigned long)m_focuserPosN, (unsigned)m_focuserSpeedN);
    m_focuser.valid = true;
    m_timerFocuser.markUpdated();
  }
  else
  {
    m_focuser.valid = false;
  }

  m_connectionFailure = 0;
}

// ===========================================================================
//  All-config bulk update  (:GXCS#)
// ===========================================================================

void TeenAstroMountStatus::updateAllConfig(bool force)
{
  if (!m_timerConfig.needsUpdate(30000) && !force) return;

  // Fetch the 120-char base64 response.
  char raw[122] = "";
  if (m_client->get(":GXCS#", raw, sizeof(raw)) != LX200_VALUEGET || strlen(raw) != 120)
  {
    m_configValid = false;
    m_connectionFailure++;
    return;
  }

  // Decode to 90 bytes.
  uint8_t pkt[90];
  if (!b64Decode(raw, 120, pkt))
  {
    m_configValid = false;
    m_connectionFailure++;
    return;
  }

  // Verify XOR checksum: XOR of bytes 0–88 must equal byte 89.
  uint8_t xorChk = 0;
  for (int i = 0; i < 89; i++) xorChk ^= pkt[i];
  if (xorChk != pkt[89])
  {
    m_configValid = false;
    m_connectionFailure++;
    return;
  }

  // ── Unpack axes (0–31) ────────────────────────────────────────────────
  for (int ax = 0; ax < 2; ax++)
  {
    int base = ax * 16;
    m_cfgGear[ax]         = pktU32(pkt, base + 0);
    m_cfgStepRot[ax]      = pktU16(pkt, base + 4);
    m_cfgBacklash[ax]     = pktU16(pkt, base + 6);
    m_cfgBacklashRate[ax] = pktU16(pkt, base + 8);
    m_cfgLowCurr[ax]      = pktU16(pkt, base + 10);
    m_cfgHighCurr[ax]     = pktU16(pkt, base + 12);
    m_cfgMicro[ax]        = pkt[base + 14];
    m_cfgFlags[ax]        = pkt[base + 15];
  }

  // ── Unpack rates (32–55) ──────────────────────────────────────────────
  m_cfgGuideRate    = pktF32(pkt, 32);
  m_cfgSlowRate     = pktF32(pkt, 36);
  m_cfgMediumRate   = pktF32(pkt, 40);
  m_cfgFastRate     = pktF32(pkt, 44);
  m_cfgAcceleration = pktF32(pkt, 48);
  m_cfgMaxRate      = pktU16(pkt, 52);
  m_cfgDefaultRate  = pkt[54];
  m_cfgSettleTime   = pkt[55];

  // ── Unpack limits (56–73) ─────────────────────────────────────────────
  m_cfgMeridianE   = (int16_t)pktU16(pkt, 56);
  m_cfgMeridianW   = (int16_t)pktU16(pkt, 58);
  m_cfgAxis1Min    = (int16_t)pktU16(pkt, 60);
  m_cfgAxis1Max    = (int16_t)pktU16(pkt, 62);
  m_cfgAxis2Min    = (int16_t)pktU16(pkt, 64);
  m_cfgAxis2Max    = (int16_t)pktU16(pkt, 66);
  m_cfgUnderPole10 = pktU16(pkt, 68);
  m_cfgMinAlt      = (int8_t)pkt[70];
  m_cfgMaxAlt      = (int8_t)pkt[71];
  m_cfgMinDistPole = pkt[72];
  m_cfgRefrFlags   = pkt[73];

  // ── Unpack encoders (74–83) ───────────────────────────────────────────
  m_cfgPPD1        = pktU32(pkt, 74);
  m_cfgPPD2        = pktU32(pkt, 78);
  m_cfgEncSyncMode = pkt[82];
  m_cfgEncFlags    = pkt[83];

  // ── Unpack options (84) ───────────────────────────────────────────────
  m_cfgMountIdx    = pkt[84];

  m_configValid = true;
  m_timerConfig.markUpdated();
  m_connectionFailure = 0;
}

// ===========================================================================
//  Focuser
// ===========================================================================

bool TeenAstroMountStatus::findFocuser()
{
  int k = 0;
  char fc[45];
  while (!m_hasFocuser && k < 3)
  {
    if (m_client->getFocuserStatus(fc, sizeof(fc)) == LX200_VALUEGET)
    {
      if (fc[0] == '?')
        m_hasFocuser = true;
    }
    delay(100);
    k++;
  }
  return m_hasFocuser;
}

void TeenAstroMountStatus::updateFocuser()
{
  if (!m_hasFocuser) return;

  if (m_timerFocuser.needsUpdate(UPDATERATE))
  {
    char fc[45];
    bool ok = (m_client->getFocuserStatus(fc, sizeof(fc)) == LX200_VALUEGET);
    if (ok && fc[0] == '?')
    {
      m_focuser.valid = true;
      m_timerFocuser.markUpdated();
      strncpy(m_focuser.data, fc, sizeof(m_focuser.data));
    }
    else
    {
      m_focuser.valid = false;
    }
  }
}

// ===========================================================================
//  Tracking rates
// ===========================================================================

void TeenAstroMountStatus::updateTrackingRate()
{
  // If all-state was recently updated, rates are already in cache — no serial.
  if (!m_timerTrackRate.needsUpdate(UPDATERATE))
    return;
  m_hasInfoTrackingRate = (m_client->getTrackRateRA(m_trackRateRa) == LX200_VALUEGET);
  if (m_hasInfoTrackingRate)
    m_timerTrackRate.markUpdated();
  else
    m_connectionFailure++;

  m_hasInfoTrackingRate &= (m_client->getTrackRateDec(m_trackRateDec) == LX200_VALUEGET);
  if (m_hasInfoTrackingRate)
    m_timerTrackRate.markUpdated();
  else
    m_connectionFailure++;
}

bool TeenAstroMountStatus::updateStoredTrackingRate()
{
  // If all-state was recently updated, stored rates are already in cache — no serial.
  if (!m_timerAllState.needsUpdate(UPDATERATE))
    return true;
  bool ok = (m_client->getStoredTrackRateRA(m_storedTrackRateRa) == LX200_VALUEGET);
  ok &= (m_client->getStoredTrackRateDec(m_storedTrackRateDec) == LX200_VALUEGET);
  return ok;
}

// ===========================================================================
//  Connection
// ===========================================================================

bool TeenAstroMountStatus::connected()      { return m_connectionFailure == 0; }
bool TeenAstroMountStatus::notResponding()   { return m_connectionFailure > 4; }

bool TeenAstroMountStatus::checkConnection(const char* major, const char* minor)
{
  if (!m_isValid)
  {
    updateV();
    m_isValid = hasInfoV() &&
      m_vn.data[0] == major[0] && m_vn.data[2] == minor[0];
  }
  return m_isValid;
}

const char* stepperDriverName(StepperDriver d)
{
  switch (d)
  {
  case StepperDriver_StepDir:  return "StepDir";
  case StepperDriver_TOS100:   return "TOS100";
  case StepperDriver_TMC2130:  return "TMC2130";
  case StepperDriver_TMC5160:  return "TMC5160";
  case StepperDriver_TMC2660:  return "TMC2660";
  case StepperDriver_Unknown:
  default:                     return "unknown";
  }
}

StepperDriver TeenAstroMountStatus::getDriverType()
{
  updateV();
  if (!hasInfoV() || m_vbb.data[0] == '\0') return StepperDriver_Unknown;
  int v = m_vbb.data[0] - '0';
  if (v >= 0 && v <= 4) return static_cast<StepperDriver>(v);
  return StepperDriver_Unknown;
}

StepperDriver TeenAstroMountStatus::getDriverTypeCached() const
{
  if (!m_version.valid || m_vbb.data[0] == '\0') return StepperDriver_Unknown;
  int v = m_vbb.data[0] - '0';
  if (v >= 0 && v <= 4) return static_cast<StepperDriver>(v);
  return StepperDriver_Unknown;
}

bool TeenAstroMountStatus::getDriverName(char* name)
{
  updateV();
  if (hasInfoV())
    strcpy(name, stepperDriverName(getDriverType()));
  else
    strcpy(name, "?");
  return m_version.valid;
}

bool TeenAstroMountStatus::hasFocuser() { return m_hasFocuser; }

// ===========================================================================
//  Location (delegated to client)
// ===========================================================================

bool TeenAstroMountStatus::getLstT0(double& T0)    { return m_client->getLstT0(T0) == LX200_VALUEGET; }
bool TeenAstroMountStatus::getLat(double& lat)      { return m_client->getLatitude(lat) == LX200_VALUEGET; }
bool TeenAstroMountStatus::getLong(double& longi)   { return m_client->getLongitude(longi) == LX200_VALUEGET; }

// Rate in Hz from GXAS cache only (same formula as main unit :GT#). Never calls :GT#.
bool TeenAstroMountStatus::getTrackingRate(double& r)
{
  r = 0.0;
  if (!m_hasInfoTrackingRate) return false;
  if (m_mount.tracking != MountState::TRK_ON) return true;
  // RequestedTrackingRateHA = (10000 - trackRateRA) / 10000; :GT# returns that * 60 * 1.00273790935
  double reqHA = (10000.0 - (double)m_trackRateRa) / 10000.0;
  r = reqHA * 60.0 * 1.00273790935;
  return true;
}

// ===========================================================================
//  Cached local date/time (UTC + timezone, no serial round-trip)
// ===========================================================================

static uint8_t daysInMonth(uint8_t m, uint16_t y)
{
  static const uint8_t dim[] = { 0, 31,28,31,30,31,30,31,31,30,31,30,31 };
  if (m < 1 || m > 12) return 30;
  if (m == 2 && (y % 4 == 0) && (y % 100 != 0 || y % 400 == 0)) return 29;
  return dim[m];
}

bool TeenAstroMountStatus::getLocalTimeCached(uint8_t& hour, uint8_t& min, uint8_t& sec) const
{
  if (!m_utc.valid || !m_tzValid) return false;
  long utcSec = (long)m_utcH * 3600L + (long)m_utcM * 60L + (long)m_utcS;
  long localSec = utcSec - (long)m_tzOff10 * 360L;
  if (localSec < 0)       localSec += 86400L;
  if (localSec >= 86400L) localSec -= 86400L;
  hour = (uint8_t)(localSec / 3600L);
  min  = (uint8_t)((localSec % 3600L) / 60L);
  sec  = (uint8_t)(localSec % 60L);
  return true;
}

bool TeenAstroMountStatus::getLocalTimeCachedTotalSec(long& totalSeconds) const
{
  uint8_t h, m, s;
  if (!getLocalTimeCached(h, m, s)) return false;
  totalSeconds = (long)h * 3600L + (long)m * 60L + (long)s;
  return true;
}

bool TeenAstroMountStatus::getLocalDateCached(uint8_t& month, uint8_t& day, uint8_t& year) const
{
  if (!m_utcDate.valid || !m_tzValid) return false;
  long utcSec = (long)m_utcH * 3600L + (long)m_utcM * 60L + (long)m_utcS;
  long localSec = utcSec - (long)m_tzOff10 * 360L;
  int dayDelta = 0;
  if (localSec < 0)       dayDelta = -1;
  if (localSec >= 86400L) dayDelta = +1;

  int d = (int)m_utcDay + dayDelta;
  int mo = (int)m_utcMonth;
  int y  = (int)m_utcYear;
  uint16_t fullYear = (uint16_t)y + 2000;
  if (d < 1)
  {
    mo--;
    if (mo < 1) { mo = 12; y--; }
    d = daysInMonth((uint8_t)mo, (uint16_t)(y + 2000));
  }
  else if (d > (int)daysInMonth((uint8_t)mo, fullYear))
  {
    d = 1;
    mo++;
    if (mo > 12) { mo = 1; y++; }
  }
  month = (uint8_t)mo;
  day   = (uint8_t)d;
  year  = (uint8_t)y;
  return true;
}

// ===========================================================================
//  Error messages
// ===========================================================================

bool TeenAstroMountStatus::getLastErrorMessage(char message[])
{
  strcpy(message, "");
  switch (getError())
  {
  case ERR_NONE:        strcpy(message, "None"); break;
  case ERR_MOTOR_FAULT: strcpy(message, "Motor or Driver Fault"); break;
  case ERR_ALT:         strcpy(message, "Altitude Min/Max"); break;
  case ERR_LIMIT_SENSE: strcpy(message, "Limit Sense"); break;
  case ERR_LIMIT_A1:    strcpy(message, "Axis1 Lim. Exceeded"); break;
  case ERR_LIMIT_A2:    strcpy(message, "Axis2 Lim. Exceeded"); break;
  case ERR_UNDER_POLE:  strcpy(message, "Under Pole Limit Exceeded"); break;
  case ERR_MERIDIAN:    strcpy(message, "Meridian Limit (W) Exceeded"); break;
  case ERR_SYNC:        strcpy(message, "Sync. ignored >30&deg;"); break;
  default: break;
  }
  return message[0];
}
