#include <TeenAstroLX200io.h>
#include <TeenAstroMountStatus.h>

#define updaterate 200

void TeenAstroMountStatus::nextStepAlign()
{
  if (!isAligning())
    return;
  if (isAlignSelect())
  {
    m_align = ALI_SLEW;
    return;
  }
  if (isAlignSlew())
  {
    m_align = ALI_RECENTER;
    return;
  }
  if (isAlignRecenter())
  {
    m_alignStar++;
    m_align = ALI_SELECT;
    return;
  }
  return;
};
void TeenAstroMountStatus::backStepAlign()
{
  if (!isAligning())
    return;
  if (isAlignSelect())
  {
    stopAlign();
    return;
  };
  if (isAlignSlew())
  {
    m_align = ALI_SELECT;
    return;
  }
  if (isAlignRecenter())
  {
    m_align = ALI_SELECT;
    return;
  }
  return;
};
void TeenAstroMountStatus::updateV()
{
  if (!m_hasInfoV)
  {
    m_hasInfoV = GetLX200(":GVP#", m_TempVP, sizeof(m_TempVP)) == LX200_VALUEGET;
    m_hasInfoV = m_hasInfoV && !strcmp(m_TempVP, "TeenAstro");
    m_hasInfoV = m_hasInfoV && GetLX200(":GVN#", m_TempVN, sizeof(m_TempVN)) == LX200_VALUEGET;
    m_hasInfoV = m_hasInfoV && GetLX200(":GVD#", m_TempVD, sizeof(m_TempVD)) == LX200_VALUEGET;
    m_hasInfoV = m_hasInfoV && GetLX200(":GVB#", m_TempVB, sizeof(m_TempVB)) == LX200_VALUEGET;
    m_hasInfoV = m_hasInfoV && GetLX200(":GVb#", m_TempVb, sizeof(m_TempVb)) == LX200_VALUEGET;
    m_hasInfoV ? 0 : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateRaDec()
{
  if (millis() - m_lastStateRaDec > updaterate)
  {
    m_hasInfoRa = GetLX200(":GR#", m_TempRa, sizeof(m_TempRa)) == LX200_VALUEGET;
    m_hasInfoDec = GetLX200(":GD#", m_TempDec, sizeof(m_TempDec)) == LX200_VALUEGET;
    m_hasInfoRa&& m_hasInfoDec ? m_lastStateRaDec = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateHaDec()
{
  if (millis() - m_lastStateHaDec > updaterate)
  {
    m_hasInfoHa = GetLX200(":GXT3#", m_TempHa, sizeof(m_TempHa)) == LX200_VALUEGET;
    m_hasInfoDec = GetLX200(":GD#", m_TempDec, sizeof(m_TempDec)) == LX200_VALUEGET;
    m_hasInfoHa&& m_hasInfoDec ? m_lastStateHaDec = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateRaDecT()
{
  if (millis() - m_lastStateRaDecT > updaterate)
  {
    m_hasInfoRaT = GetLX200(":Gr#", m_TempRaT, sizeof(m_TempRaT)) == LX200_VALUEGET;
    m_hasInfoDecT = GetLX200(":Gd#", m_TempDecT, sizeof(m_TempDecT)) == LX200_VALUEGET;
    m_hasInfoRaT&& m_hasInfoDecT ? m_lastStateRaDecT = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateAzAlt()
{
  if (millis() - m_lastStateAzAlt > updaterate)
  {
    m_hasInfoAz = GetLX200(":GZ#", m_TempAz, sizeof(m_TempAz)) == LX200_VALUEGET;
    m_hasInfoAlt = GetLX200(":GA#", m_TempAlt, sizeof(m_TempAlt)) == LX200_VALUEGET;
    m_hasInfoAz&& m_hasInfoAlt ? m_lastStateAzAlt = millis() : m_connectionFailure++;
  }
}
void TeenAstroMountStatus::updatePush()
{
  if (m_hasEncoder && millis() - m_lastStatePush > updaterate)
  {
    m_hasInfoPush = GetLX200(":ED#", m_TempPush, sizeof(m_TempPush)) == LX200_VALUEGET;
    m_TempPush[1] = 0;
    m_TempPush[8] = 0;
    m_hasInfoPush ? m_lastStatePush = millis() : m_connectionFailure++;
  }
}
void TeenAstroMountStatus::updateAxisStep()
{
  if (millis() - m_lastStateAxisStep > updaterate)
  {
    m_hasInfoAxis1Step = GetLX200(":GXDP1#", m_TempAxis1Step, sizeof(m_TempAxis1Step)) == LX200_VALUEGET;
    m_hasInfoAxis2Step = GetLX200(":GXDP2#", m_TempAxis2Step, sizeof(m_TempAxis2Step)) == LX200_VALUEGET;
    m_hasInfoAxis1Step&& m_hasInfoAxis2Step ? m_lastStateAxisStep = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateAxisDeg()
{
  if (millis() - m_lastStateAxisDeg > updaterate)
  {
    m_hasInfoAxis1Deg = GetLX200(":GXP1#", m_TempAxis1Deg, sizeof(m_TempAxis1Deg)) == LX200_VALUEGET;
    m_hasInfoAxis2Deg = GetLX200(":GXP2#", m_TempAxis2Deg, sizeof(m_TempAxis2Deg)) == LX200_VALUEGET;
    if (hasEncoder())
    {
      m_hasInfoAxis1EDeg = GetLX200(":GXE1#", m_TempAxis1EDeg, sizeof(m_TempAxis1EDeg)) == LX200_VALUEGET;
      m_hasInfoAxis2EDeg = GetLX200(":GXE2#", m_TempAxis2EDeg, sizeof(m_TempAxis2EDeg)) == LX200_VALUEGET;
      m_hasInfoAxis1Deg&& m_hasInfoAxis2Deg&& m_hasInfoAxis1EDeg&& m_hasInfoAxis2EDeg ?
        m_lastStateAxisDeg = millis() : m_connectionFailure++;
    }
    else
    {
      m_hasInfoAxis1Degc = GetLX200(":GXP3#", m_TempAxis1Degc, sizeof(m_TempAxis1Degc)) == LX200_VALUEGET;
      m_hasInfoAxis2Degc = GetLX200(":GXP4#", m_TempAxis2Degc, sizeof(m_TempAxis2Degc)) == LX200_VALUEGET;
      m_hasInfoAxis1Deg&& m_hasInfoAxis2Deg && m_hasInfoAxis1Degc&& m_hasInfoAxis2Degc ?
        m_lastStateAxisDeg = millis() : m_connectionFailure++;
    }
  }
};
void TeenAstroMountStatus::updateTime()
{
  if (millis() - m_lastStateTime > updaterate)
  {
    m_hasInfoUTC = GetLX200(":GXT0#", m_TempUTC, sizeof(m_TempUTC)) == LX200_VALUEGET;
    m_hasInfoUTCdate = GetLX200(":GXT1#", m_TempUTCdate, sizeof(m_TempUTCdate)) == LX200_VALUEGET;
    m_hasInfoSidereal = GetLX200(":GS#", m_TempSidereal, sizeof(m_TempSidereal)) == LX200_VALUEGET;
    m_hasInfoUTC&& m_hasInfoSidereal&& m_hasInfoUTCdate ? m_lastStateTime = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateLHA()
{
  if (millis() - m_lastStateTime > updaterate)
  {
    m_hasInfoUTC = GetLX200(":GXT0#", m_TempUTC, sizeof(m_TempUTC)) == LX200_VALUEGET;
    m_hasInfoLHA = GetLX200(":GXT3#", m_TempLHA, sizeof(m_TempUTC)) == LX200_VALUEGET;
    //m_hasInfoAxis1 = GetLX200(":GXF8#", m_TempAxis1, sizeof(m_TempAxis1)) == LX200_VALUEGET;
    m_hasInfoUTC&& m_hasInfoLHA ? m_lastStateTime = millis() : m_connectionFailure++;
  }
};

bool TeenAstroMountStatus::findFocuser()
{
  int k = 0;
  char fc[45];
  while (!m_hasFocuser && k < 3)
  {
    if (GetLX200(":F?#", fc, sizeof(m_TempFocuser)) == LX200_VALUEGET)
    {
      if (fc[0] == '?')
      {
        m_hasFocuser = true;
      }
    }
    delay(100);
    k++;
  }
  return m_hasFocuser;
}

void TeenAstroMountStatus::updateFocuser()
{
  if (!m_hasFocuser)
  {
    return;
  }
  if (millis() - m_lastStateFocuser > updaterate)
  {
    char fc[45];
    m_hasInfoFocuser = GetLX200(":F?#", fc, sizeof(m_TempFocuser)) == LX200_VALUEGET;
    if (m_hasInfoFocuser && fc[0] == '?')
    {
      m_lastStateFocuser = millis();
      strncpy(m_TempFocuser, fc, 45);
    }
    else if (m_hasInfoFocuser && fc[0] == '0')
    {
      m_hasInfoFocuser = false;
    }
    else
    {
      m_hasInfoFocuser = false;
    }
  }
};
void TeenAstroMountStatus::updateTrackingRate()
{
  if (millis() - m_lastStateTrackingRate > updaterate)
  {
    char reply[15] = "0";
    m_hasInfoTrackingRate = GetLX200(":GXRr#", reply, sizeof(reply)) == LX200_VALUEGET;
    m_TempTrackingRateRa = strtol(reply, NULL, 10);
    m_hasInfoTrackingRate ? m_lastStateTrackingRate = millis() : m_connectionFailure++;
    m_hasInfoTrackingRate &= GetLX200(":GXRd#", reply, sizeof(reply)) == LX200_VALUEGET;
    m_TempTrackingRateDec = strtol(reply, NULL, 10);
    m_hasInfoTrackingRate ? m_lastStateTrackingRate = millis() : m_connectionFailure++;
  }
};
bool TeenAstroMountStatus::updateStoredTrackingRate()
{
  char reply[15] = "0";
  bool ok = GetLX200(":GXRe#", reply, sizeof(reply)) == LX200_VALUEGET;
  if (ok)
  {
    m_TempStoredTrackingRateRa = strtol(reply, NULL, 10);
  }
  ok &= GetLX200(":GXRf#", reply, sizeof(reply)) == LX200_VALUEGET;
  if (ok)
  {
    m_TempStoredTrackingRateDec = strtol(reply, NULL, 10);
  }
  return ok;
};

void TeenAstroMountStatus::updateMount()
{
  if (millis() - m_lastStateMount > updaterate)
  {
    m_hasInfoMount = GetLX200(":GXI#", m_TempMount, sizeof(m_TempMount)) == LX200_VALUEGET;
    m_hasInfoMount ? m_lastStateMount = millis() : m_connectionFailure++;
  }
};
bool TeenAstroMountStatus::connected()
{
  return m_connectionFailure == 0;
};
bool TeenAstroMountStatus::notResponding()
{
  return m_connectionFailure > 4;
};
TeenAstroMountStatus::ParkState TeenAstroMountStatus::getParkState()
{
  switch (m_TempMount[2])
  {
  case 'P':
    return PRK_PARKED;
  case 'p':
    return PRK_UNPARKED;
  case 'I':
    return PRK_PARKING;
  case 'F':
    return PRK_FAILED;
  }
  return PRK_UNKNOW;
};
TeenAstroMountStatus::Mount TeenAstroMountStatus::getMount()
{
  switch (m_TempMount[12])
  {
  case 'E':
    return MOUNT_TYPE_GEM;
  case 'K':
    return MOUNT_TYPE_FORK;
  case 'A':
    return MOUNT_TYPE_ALTAZM;
  case 'k':
    return MOUNT_TYPE_FORK_ALT;
  case 'U':
    return MOUNT_UNDEFINED;
  }
  return MOUNT_UNDEFINED;
}

bool TeenAstroMountStatus::isAltAz()
{
  switch (getMount())
  {
  case MOUNT_TYPE_GEM:
  case MOUNT_TYPE_FORK:
    return false;
    break;
  }
  return true;
}

TeenAstroMountStatus::TrackState TeenAstroMountStatus::getTrackingState()
{
  switch (m_TempMount[0])
  {
  case '3':
  case '2':
    return TRK_SLEWING;
  case '1':
    return TRK_ON;
  case '0':
    return TRK_OFF;
  default:
    return TRK_UNKNOW;
  }
}
TeenAstroMountStatus::SiderealMode TeenAstroMountStatus::getSiderealMode()
{
  switch (m_TempMount[1])
  {
  case '3':
  case '2':
  case '1':
  case '0':
    return static_cast<SiderealMode>(m_TempMount[1] - '0');
  default:
    return SiderealMode::SID_UNKNOWN;
  }
}
bool TeenAstroMountStatus::isTrackingCorrected()
{
  return m_TempMount[10] == 'c';
}
bool TeenAstroMountStatus::getLstT0(double& T0)
{
  return GetLstT0LX200(T0) == LX200_VALUEGET;
};
bool TeenAstroMountStatus::getLat(double& lat)
{
  return GetLatitudeLX200(lat) == LX200_VALUEGET;
};
bool TeenAstroMountStatus::getLong(double& longi)
{
  return GetLongitudeLX200(longi) == LX200_VALUEGET;
};
bool TeenAstroMountStatus::getTrackingRate(double& r)
{
  return GetTrackingRateLX200(r) == LX200_VALUEGET;
};
bool TeenAstroMountStatus::checkConnection(char* major, char* minor)
{
  if (!m_isValid)
  {
    updateV();
    m_isValid = hasInfoV() &&
      m_TempVN[0] == major[0] && m_TempVN[2] == minor[0];
  }
  return m_isValid;
};
bool TeenAstroMountStatus::getDriverName(char* name)
{
  if (!m_isValid)
  {
    updateV();
    if (m_isValid)
    {
      switch (m_TempVb[0])
      {
      default:
      case '0':
        strcpy(name, "unknown");
        break;
      case '1':
        strcpy(name, "TOS100");
        break;
      case '2':
        strcpy(name, "TMC2130");
        break;
      case '3':
        strcpy(name, "TMC5160");
        break;
      case '4':
        strcpy(name, "TMC2660");
        break;
      }
    }
  }
  return m_isValid;
}
bool TeenAstroMountStatus::atHome()
{
  return m_TempMount[3] == 'H';
}
bool TeenAstroMountStatus::Parking()
{
  return getParkState() == PRK_PARKING;
}
bool TeenAstroMountStatus::Parked()
{
  return getParkState() == PRK_PARKED;
}
TeenAstroMountStatus::GuidingRate TeenAstroMountStatus::getGuidingRate()
{
  int val = m_TempMount[4] - '0';
  return (val > -1 && val < 5) ? static_cast<GuidingRate>(val) : UNKNOW;
}
TeenAstroMountStatus::RateCompensation TeenAstroMountStatus::getRateCompensation()
{
  int val = m_TempMount[10] - '0';

  return (val > -1 && val < 5) ? static_cast<RateCompensation>(val) : RateCompensation::RC_UNKNOWN;
}
bool TeenAstroMountStatus::isSpiralRunning()
{
  return m_TempMount[5] == '@';
}
bool TeenAstroMountStatus::isPulseGuiding()
{
  return m_TempMount[6] == '*';
}
bool TeenAstroMountStatus::isGuidingE()
{
  return m_TempMount[7] == '>';
}
bool TeenAstroMountStatus::isGuidingW()
{
  return m_TempMount[7] == '<';
}
bool TeenAstroMountStatus::isGuidingN()
{
  return m_TempMount[8] == '^';
}
bool TeenAstroMountStatus::isGuidingS()
{
  return m_TempMount[8] == '_';
}
bool TeenAstroMountStatus::isAligned()
{
  return m_TempMount[11] == '1';
}
bool TeenAstroMountStatus::hasGNSSBoard()
{
  return bitRead(m_TempMount[14] - 'A', 0);
}
bool TeenAstroMountStatus::isGNSSValid()
{
  return bitRead(m_TempMount[14] - 'A', 1);
}
bool TeenAstroMountStatus::isGNSSTimeSync()
{
  return bitRead(m_TempMount[14] - 'A', 2);
}
bool TeenAstroMountStatus::isGNSSLocationSync()
{
  return bitRead(m_TempMount[14] - 'A', 3);
}
bool TeenAstroMountStatus::isHdopSmall()
{
  return bitRead(m_TempMount[14] - 'A', 4);
}
bool TeenAstroMountStatus::hasFocuser()
{
  return m_hasFocuser;
}
bool TeenAstroMountStatus::hasEncoder()
{
  static bool firstime = m_hasEncoder;
  if (firstime)
  {
    updateMount();
    m_hasEncoder = bitRead(m_TempMount[16] - 'A', 0);
  }
  return m_hasEncoder;
}
bool TeenAstroMountStatus::CalibratingEncoder()
{
  return bitRead(m_TempMount[16] - 'A', 1);
}
bool TeenAstroMountStatus::isPushingto()
{
  return bitRead(m_TempMount[16] - 'A', 2);
}

TeenAstroMountStatus::PierState TeenAstroMountStatus::getPierState()
{
  switch (m_TempMount[13])
  {
  case ' ':
    return PIER_UNKNOW;
  case 'E':
    return PIER_E;
  case 'W':
    return PIER_W;
  }
  return PIER_UNKNOW;
}
TeenAstroMountStatus::Errors TeenAstroMountStatus::getError()
{
  switch (m_TempMount[15])
  {
  case '1':
    return ERR_MOTOR_FAULT;
  case '2':
    return ERR_ALT;
  case '3':
    return ERR_LIMIT_SENSE;
  case '4':
    return ERR_LIMIT_A1;
  case '5':
    return ERR_LIMIT_A2;
  case '6':
    return ERR_UNDER_POLE;
  case '7':
    return ERR_MERIDIAN;
  case '8':
    return ERR_SYNC;
  default:
    return ERR_NONE;
  }
}
bool TeenAstroMountStatus::getLastErrorMessage(char message[])
{
  strcpy(message, "");
  switch (getError())
  {
  case ERR_NONE:
    strcpy(message, "None");
    break;
  case ERR_MOTOR_FAULT:
    strcpy(message, "Motor or Driver Fault");
    break;
  case ERR_ALT:
    strcpy(message, "Altitude Min/Max");
    break;
  case ERR_LIMIT_SENSE:
    strcpy(message, "Limit Sense");
    break;
  case ERR_LIMIT_A1:
    strcpy(message, "Axis1 Lim. Exceeded");
    break;
  case ERR_LIMIT_A2:
    strcpy(message, "Axis2 Lim. Exceeded");
    break;
  case ERR_UNDER_POLE:
    strcpy(message, "Under Pole Limit Exceeded");
    break;
  case ERR_MERIDIAN:
    strcpy(message, "Meridian Limit (W) Exceeded");
    break;
  case ERR_SYNC:
    strcpy(message, "Sync. ignored >30&deg;");
    break;
  default:
    break;
  }
  return message[0];
}

TeenAstroMountStatus::AlignReply TeenAstroMountStatus::addStar()
{
  if (isAlignRecenter())
  {
    char text[5] = ":A #";

    if (getAlignMode() == TeenAstroMountStatus::ALIM_TWO)
      text[2] = '2';
    else if (getAlignMode() == TeenAstroMountStatus::ALIM_THREE)
      text[2] = '3';
    else
    {
      stopAlign();
      return AlignReply::ALIR_FAILED2;
    }

    if (SetLX200(text) == LX200_VALUESET)
    {
      if (isLastStarAlign())
      {
        stopAlign();
        return AlignReply::ALIR_DONE;
      }
      else
      {
        nextStepAlign();
        return AlignReply::ALIR_ADDED;
      }
    }
    else
    {
      stopAlign();
      return AlignReply::ALIR_FAILED1;
    }
  }
  else
  {
    stopAlign();
    return AlignReply::ALIR_FAILED2;
  }
}