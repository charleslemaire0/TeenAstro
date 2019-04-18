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
  };
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
    if (m_alignStar == 1)
    {
      stopAlign();
      return;
    }
    m_align = ALI_RECENTER;
    m_alignStar--;
    return;
  };
  if (isAlignSlew())
  {
    m_align = ALI_SELECT;
    return;
  }
  if (isAlignRecenter())
  {
    m_align = ALI_SLEW;
    return;
  }
  return;
};
void TeenAstroMountStatus::updateRaDec()
{
  if (millis() - m_lastStateRaDec > updaterate)
  {
    hasInfoRa = GetLX200(":GR#", m_TempRa, sizeof(m_TempRa)) == LX200VALUEGET;
    hasInfoDec = GetLX200(":GD#", m_TempDec, sizeof(m_TempDec)) == LX200VALUEGET;
    hasInfoRa && hasInfoDec ? m_lastStateRaDec = millis() : connectionFailure++;
  }
};
void TeenAstroMountStatus::updateAzAlt()
{
  if (millis() - m_lastStateAzAlt > updaterate)
  {
    hasInfoAz = GetLX200(":GZ#", m_TempAz, sizeof(m_TempAz)) == LX200VALUEGET;
    hasInfoAlt = GetLX200(":GA#", m_TempAlt, sizeof(m_TempAlt)) == LX200VALUEGET;
    hasInfoAz && hasInfoAlt ? m_lastStateAzAlt = millis() : connectionFailure++;
  }
}
void TeenAstroMountStatus::updateTime()
{
  if (millis() - m_lastStateTime > updaterate)
  {
    hasInfoUTC = GetLX200(":GL#", m_TempUTC, sizeof(m_TempUTC)) == LX200VALUEGET;
    hasInfoSideral = GetLX200(":GS#", m_TempSideral, sizeof(m_TempSideral)) == LX200VALUEGET;
    hasInfoUTC && hasInfoSideral ? m_lastStateTime = millis() : connectionFailure++;
  }
};
void TeenAstroMountStatus::updateFocuser()
{
  if (millis() - m_lastStateFocuser > updaterate*2)
  {
    char fc[45];
    hasInfoFocuser = GetLX200(":F?#", fc, sizeof(m_TempFocuser)) == LX200VALUEGET;
    if (hasInfoFocuser && fc[0] == '?')
    {
      m_lastStateFocuser = millis();
      strncpy(m_TempFocuser, fc, 45);
    }
    else
    {
      hasInfoFocuser = false;
    }
  }
};
void TeenAstroMountStatus::updateMount()
{
  if (millis() - m_lastStateMount > updaterate)
  {
    hasInfoMount = GetLX200(":GU#", m_TempMount, sizeof(m_TempMount)) == LX200VALUEGET;
    hasInfoMount ? m_lastStateMount = millis() : connectionFailure++;
  }
};

bool TeenAstroMountStatus::connected()
{
  return connectionFailure == 0;
}

bool TeenAstroMountStatus::notResponding()
{
  return connectionFailure > 4;
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
    break;
  }
  return MOUNT_UNDEFINED;
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
TeenAstroMountStatus::SideralMode TeenAstroMountStatus::getSideralMode()
{
  switch (m_TempMount[1])
  {
  case '2':
  case '1':
  case '0':
    return  static_cast<SideralMode>(m_TempMount[1]-'0');
  default:
    return SideralMode::SID_STAR;
  }
}
double TeenAstroMountStatus::getLstT0()
{
  char temp[20] = "";
  double f = 0;
  if (GetLX200(":GS#", temp, 20) == LX200VALUEGET)
  {  
    hmsToDouble(&f, temp);
  }
  return f;
};
double TeenAstroMountStatus::getLat()
{
  double f = -10000;
  GetLatitudeLX200(f);
  return f;
};
bool TeenAstroMountStatus::atHome()
{
  return m_TempMount[3] == 'H';
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
bool TeenAstroMountStatus::isGNSSValid()
{
  return  m_TempMount[14] == '1';
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
  case '4':
    return ERR_DEC;
  case '6':
    return ERR_UNDER_POLE;
  case '7':
    return ERR_MERIDIAN;
  default:
    return ERR_NONE;
  }
}
void TeenAstroMountStatus::addStar()
{
  if (isAlignRecenter())
  {
    if (SetLX200(":A+#")==LX200VALUESET)
    {
      bool done = false;
      if (isLastStarAlign())
      {
        //TODO DisplayMessage("Alignment", "Success!", -1);
        stopAlign();
      }
      else
      {
        nextStepAlign();
        //TODO DisplayMessage("Add Star", "Success!", -1);
      }
    }
    else
    {
      //TODO DisplayMessage("Add Star", "Failed!", -1);
      stopAlign();
    }
  }
  else
  {
    //TODO DisplayMessage("Failed!", "Wrong State", -1);
    stopAlign();
  }
}