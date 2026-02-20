#include <TeenAstroMountStatus.h>

#define UPDATERATE 500

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
  if (m_timerTrackRate.needsUpdate(UPDATERATE))
  {
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
}

bool TeenAstroMountStatus::updateStoredTrackingRate()
{
  bool ok = (m_client->getStoredTrackRateRA(m_storedTrackRateRa) == LX200_VALUEGET);
  ok &= (m_client->getStoredTrackRateDec(m_storedTrackRateDec) == LX200_VALUEGET);
  return ok;
}

// ===========================================================================
//  Connection
// ===========================================================================

bool TeenAstroMountStatus::connected()      { return m_connectionFailure == 0; }
bool TeenAstroMountStatus::notResponding()   { return m_connectionFailure > 4; }

bool TeenAstroMountStatus::checkConnection(char* major, char* minor)
{
  if (!m_isValid)
  {
    updateV();
    m_isValid = hasInfoV() &&
      m_vn.data[0] == major[0] && m_vn.data[2] == minor[0];
  }
  return m_isValid;
}

bool TeenAstroMountStatus::getDriverName(char* name)
{
  updateV();
  if (hasInfoV())
  {
    switch (m_vbb.data[0] - '0')
    {
    case 0: strcpy(name, "StepDir"); break;
    case 1: strcpy(name, "TOS100");  break;
    case 2: strcpy(name, "TMC2130"); break;
    case 3: strcpy(name, "TMC5160"); break;
    case 4: strcpy(name, "TMC2660"); break;
    default: strcpy(name, "unknown"); break;
    }
  }
  return m_version.valid;
}

bool TeenAstroMountStatus::hasFocuser() { return m_hasFocuser; }

// ===========================================================================
//  Location (delegated to client)
// ===========================================================================

bool TeenAstroMountStatus::getLstT0(double& T0)    { return m_client->getLstT0(T0) == LX200_VALUEGET; }
bool TeenAstroMountStatus::getLat(double& lat)      { return m_client->getLatitude(lat) == LX200_VALUEGET; }
bool TeenAstroMountStatus::getLong(double& longi)   { return m_client->getLongitude(longi) == LX200_VALUEGET; }
bool TeenAstroMountStatus::getTrackingRate(double& r) { return m_client->getTrackingRate(r) == LX200_VALUEGET; }

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
