#include <TeenAstroLX200io.h>
#include <TeenAstroMountStatus.h>

#define updaterate 200
 
void TeenAstroMountStatus::nextStepAlign()
{
  if (!isAligning())
    return;
  if (isAlignSelect())
  {
    m_align = ALI_SLEW; return;
  }
  if (isAlignSlew())
  {
    m_align = ALI_RECENTER; return;
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
    m_hasInfoV = GetLX200(":GVP#", m_TempVP, sizeof(m_TempVP)) == LX200VALUEGET;
    m_hasInfoV = m_hasInfoV && !strcmp(m_TempVP, "TeenAstro");
    m_hasInfoV = m_hasInfoV && GetLX200(":GVN#", m_TempVN, sizeof(m_TempVN)) == LX200VALUEGET;
    m_hasInfoV = m_hasInfoV && GetLX200(":GVD#", m_TempVD, sizeof(m_TempVD)) == LX200VALUEGET;
    m_hasInfoV = m_hasInfoV && GetLX200(":GVB#", m_TempVB, sizeof(m_TempVB)) == LX200VALUEGET;
    m_hasInfoV = m_hasInfoV && GetLX200(":GVb#", m_TempVb, sizeof(m_TempVb)) == LX200VALUEGET;
    m_hasInfoV ? 0 : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateRaDec()
{
  if (millis() - m_lastStateRaDec > updaterate)
  {
    m_hasInfoRa = GetLX200(":GR#", m_TempRa, sizeof(m_TempRa)) == LX200VALUEGET;
    m_hasInfoDec = GetLX200(":GD#", m_TempDec, sizeof(m_TempDec)) == LX200VALUEGET;
    m_hasInfoRa && m_hasInfoDec ? m_lastStateRaDec = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateRaDecT()
{
  if (millis() - m_lastStateRaDecT > updaterate)
  {
    m_hasInfoRaT = GetLX200(":Gr#", m_TempRaT, sizeof(m_TempRaT)) == LX200VALUEGET;
    m_hasInfoDecT = GetLX200(":Gd#", m_TempDecT, sizeof(m_TempDecT)) == LX200VALUEGET;
    m_hasInfoRaT && m_hasInfoDecT ? m_lastStateRaDecT = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateAzAlt()
{
  if (millis() - m_lastStateAzAlt > updaterate)
  {
    m_hasInfoAz = GetLX200(":GZ#", m_TempAz, sizeof(m_TempAz)) == LX200VALUEGET;
    m_hasInfoAlt = GetLX200(":GA#", m_TempAlt, sizeof(m_TempAlt)) == LX200VALUEGET;
    m_hasInfoAz && m_hasInfoAlt ? m_lastStateAzAlt = millis() : m_connectionFailure++;
  }
}
void TeenAstroMountStatus::updateAxis()
{
  if (millis() - m_lastStateAxis > updaterate)
  {
    m_hasInfoAxis1 = GetLX200(":GXF8#", m_TempAxis1, sizeof(m_TempAxis1)) == LX200VALUEGET;
    m_hasInfoAxis2 = GetLX200(":GXF9#", m_TempAxis2, sizeof(m_TempAxis2)) == LX200VALUEGET;
    m_hasInfoAxis1 && m_hasInfoAxis2 ? m_lastStateAxis = millis() : m_connectionFailure++;
  }
}
void TeenAstroMountStatus::updateTime()
{
  if (millis() - m_lastStateTime > updaterate)
  {
    m_hasInfoUTC = GetLX200(":GXT0#", m_TempUTC, sizeof(m_TempUTC)) == LX200VALUEGET;
    m_hasInfoUTCdate =  GetLX200(":GXT1#", m_TempUTCdate, sizeof(m_TempUTCdate)) == LX200VALUEGET;
    m_hasInfoSidereal = GetLX200(":GS#", m_TempSidereal, sizeof(m_TempSidereal)) == LX200VALUEGET;
    m_hasInfoUTC && m_hasInfoSidereal  && m_hasInfoUTCdate ? m_lastStateTime = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateFocuser()
{
  if (!m_hasFocuser)
  {
    return;
  }
  if (millis() - m_lastStateFocuser > updaterate)
  {
    char fc[45];
    m_hasInfoFocuser = GetLX200(":F?#", fc, sizeof(m_TempFocuser)) == LX200VALUEGET;
    if (m_hasInfoFocuser && fc[0] == '?')
    {
      m_lastStateFocuser = millis();
      strncpy(m_TempFocuser, fc, 45);
    }
    else if (m_hasInfoFocuser && fc[0] == '0')
    {
      m_hasInfoFocuser = false;
      m_hasFocuser = false;
    }
    else
    {
      m_hasInfoFocuser = false;
    }
  }
};
void TeenAstroMountStatus::updateTrackingRate()
{
  if (millis() - m_lastTrackingRate > updaterate)
  {
    m_hasInfoTrackingRate = GetLX200(":GT#", m_TempTrackingRate, sizeof(m_TempTrackingRate)) == LX200VALUEGET;
    m_hasInfoTrackingRate ? m_lastTrackingRate = millis() : m_connectionFailure++;
  }
};
void TeenAstroMountStatus::updateMount()
{
  if (millis() - m_lastStateMount > updaterate)
  {
    m_hasInfoMount = GetLX200(":GXI#", m_TempMount, sizeof(m_TempMount)) == LX200VALUEGET;
    m_hasInfoMount ? m_lastStateMount = millis() : m_connectionFailure++;
  }
};
bool TeenAstroMountStatus::connected()
{
  return m_connectionFailure == 0;
}
bool TeenAstroMountStatus::notResponding()
{
  return m_connectionFailure > 4;
}
TeenAstroMountStatus::ParkState TeenAstroMountStatus::getParkState()
{
  if (strchr(&m_TempMount[0], 'P') != NULL)
  {
    return PRK_PARKED;
  }
  else if (strchr(&m_TempMount[0], 'p') != NULL)
  {
    return PRK_UNPARKED;
  }
  else if (strchr(&m_TempMount[0], 'I') != NULL)
  {
    return PRK_PARKING;
  }
  else if (strchr(&m_TempMount[0], 'F') != NULL)
  {
    return PRK_FAILED;
  }
  return PRK_UNKNOW;
}
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
  case '2':
  case '1':
  case '0':
    return  static_cast<SiderealMode>(m_TempMount[1]-'0');
  default:
    return SiderealMode::SID_STAR;
  }
}
bool TeenAstroMountStatus::isTrackingCorrected()
{
  return m_TempMount[10] == 'c';
}
bool TeenAstroMountStatus::getLstT0(double &T0)
{
  return GetLstT0LX200(T0) == LX200VALUEGET;
};
bool TeenAstroMountStatus::getLat(double &lat)
{
  return GetLatitudeLX200(lat) == LX200VALUEGET;
};
bool TeenAstroMountStatus::getTrackingRate(double &r)
{
  return GetTrackingRateLX200(r) == LX200VALUEGET;
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
bool TeenAstroMountStatus::getGuidingRate(unsigned char &g)
{
  g = m_TempMount[4] - '0';
  return g<10;
}
bool TeenAstroMountStatus::isSpiralRunning()
{
  return  m_TempMount[5] == '@';
}
bool TeenAstroMountStatus::isPulseGuiding()
{
  return  m_TempMount[6] == '*';
}
bool TeenAstroMountStatus::isGuidingE()
{
  return  m_TempMount[7] == '>';
}
bool TeenAstroMountStatus::isGuidingW()
{
  return  m_TempMount[7] == '<';
}
bool TeenAstroMountStatus::isGuidingN()
{
  return  m_TempMount[8] == '^';
}
bool TeenAstroMountStatus::isGuidingS()
{
  return  m_TempMount[8] == '_';
}
bool TeenAstroMountStatus::isAligned()
{
  return m_TempMount[11] == '1';
}
bool TeenAstroMountStatus::isGNSSValid()
{
  return  bitRead(m_TempMount[14] - '0', 0);
}
bool TeenAstroMountStatus::isLowPower()
{
  return  bitRead(m_TempMount[14] - '0', 1);
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
    return ERR_DEC;
  case '5':
    return ERR_AZM;
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
  case ERR_DEC:
    strcpy(message, "Dec Limit Exceeded");
    break;
  case ERR_AZM:
    strcpy(message, "Azm Limit Exceeded");
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

    if (SetLX200(text) == LX200VALUESET)
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