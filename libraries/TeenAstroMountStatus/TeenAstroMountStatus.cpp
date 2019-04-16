#include <TeenAstroLX200io.h>
#include <TeenAstroMountStatus.h>

#define updaterate 200
void TeenAstroMountStatus::updateRaDec()
{
  if (millis() - lastStateRaDec > updaterate)
  {
    hasInfoRa = GetLX200(":GR#", TempRa, sizeof(TempRa)) == LX200VALUEGET;
    hasInfoDec = GetLX200(":GD#", TempDec, sizeof(TempDec)) == LX200VALUEGET;
    hasInfoRa && hasInfoDec ? lastStateRaDec = millis() : connectionFailure++;
  }
};
void TeenAstroMountStatus::updateAzAlt()
{
  if (millis() - lastStateAzAlt > updaterate)
  {
    hasInfoAz = GetLX200(":GZ#", TempAz, sizeof(TempAz)) == LX200VALUEGET;
    hasInfoAlt = GetLX200(":GA#", TempAlt, sizeof(TempAlt)) == LX200VALUEGET;
    hasInfoAz && hasInfoAlt ? lastStateAzAlt = millis() : connectionFailure++;
  }
}
void TeenAstroMountStatus::updateTime()
{
  if (millis() - lastStateTime > updaterate)
  {
    hasInfoUTC = GetLX200(":GL#", TempUTC, sizeof(TempUTC)) == LX200VALUEGET;
    hasInfoSideral = GetLX200(":GS#", TempSideral, sizeof(TempSideral)) == LX200VALUEGET;
    hasInfoUTC && hasInfoSideral ? lastStateTime = millis() : connectionFailure++;
  }
};
void TeenAstroMountStatus::updateFocuser()
{
  if (millis() - lastStateFocuser > updaterate*2)
  {
    char fc[45];
    hasInfoFocuser = GetLX200(":F?#", fc, sizeof(TempFocuser)) == LX200VALUEGET;
    if (hasInfoFocuser && fc[0] == '?')
    {
      lastStateFocuser = millis();
      strncpy(TempFocuser, fc, 45);
    }
    else
    {
      hasInfoFocuser = false;
    }
  }
};
void TeenAstroMountStatus::updateMount()
{
  if (millis() - lastStateMount > updaterate)
  {
    hasInfoMount = GetLX200(":GU#", TempMount, sizeof(TempMount)) == LX200VALUEGET;
    hasInfoMount ? lastStateMount = millis() : connectionFailure++;
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
  if (strchr(&TempMount[0], 'P') != NULL)
  {
    return PRK_PARKED;
  }
  else if (strchr(&TempMount[0], 'p') != NULL)
  {
    return PRK_UNPARKED;
  }
  else if (strchr(&TempMount[0], 'I') != NULL)
  {
    return PRK_PARKING;
  }
  else if (strchr(&TempMount[0], 'F') != NULL)
  {
    return PRK_FAILED;
  }
  return PRK_UNKNOW;
}
TeenAstroMountStatus::Mount TeenAstroMountStatus::getMount()
{
  switch (TempMount[12])
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
  switch (TempMount[0])
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
  switch (TempMount[1])
  {
  case '2':
  case '1':
  case '0':
    return  static_cast<SideralMode>(TempMount[1]-'0');
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
  return TempMount[3] == 'H';
}
bool TeenAstroMountStatus::isPulseGuiding()
{
  return  TempMount[6] == '*';
}
bool TeenAstroMountStatus::isGuidingE()
{
  return  TempMount[7] == '>';
}
bool TeenAstroMountStatus::isGuidingW()
{
  return  TempMount[7] == '<';
}
bool TeenAstroMountStatus::isGuidingN()
{
  return  TempMount[8] == '^';
}
bool TeenAstroMountStatus::isGuidingS()
{
  return  TempMount[8] == '_';
}
bool TeenAstroMountStatus::isGNSSValid()
{
  return  TempMount[14] == '1';
}
TeenAstroMountStatus::PierState TeenAstroMountStatus::getPierState()
{
  switch (TempMount[13])
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
  switch (TempMount[15])
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
  if (align == ALI_RECENTER_1 || align == ALI_RECENTER_2 || align == ALI_RECENTER_3)
  {
    if (SetLX200(":A+#")==LX200VALUESET)
    {
      bool done = false;
      if (aliMode == ALIM_ONE
        || (aliMode == ALIM_TWO && align == ALI_RECENTER_2)
        || (aliMode == ALIM_THREE && align == ALI_RECENTER_3))
      {
        //TODO DisplayMessage("Alignment", "Success!", -1);
        align = ALI_OFF;
      }
      else
      {
        align = static_cast<AlignState>(align+1);
        //TODO DisplayMessage("Add Star", "Success!", -1);
      }

    }
    else
    {
      //TODO DisplayMessage("Add Star", "Failed!", -1);
      align = ALI_OFF;
    }
  }
  else
  {
    //TODO DisplayMessage("Failed!", "Wrong State", -1);
    align = ALI_OFF;
  }
}