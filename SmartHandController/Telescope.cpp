#include "Telescope.h"
#include "LX200.h"

#define updaterate 200
void Telescope::updateRaDec()
{
  if (millis() - lastStateRaDec > updaterate)
  {
    hasInfoRa = GetLX200(":GR#", TempRa, sizeof(TempRa)) == LX200VALUEGET;
    hasInfoDec = GetLX200(":GD#", TempDec, sizeof(TempDec)) == LX200VALUEGET;
    hasInfoRa && hasInfoDec ? lastStateRaDec = millis() : connectionFailure++;
  }
};
void Telescope::updateAzAlt()
{
  if (millis() - lastStateAzAlt > updaterate)
  {
    hasInfoAz = GetLX200(":GZ#", TempAz, sizeof(TempAz)) == LX200VALUEGET;
    hasInfoAlt = GetLX200(":GA#", TempAlt, sizeof(TempAlt)) == LX200VALUEGET;
    hasInfoAz && hasInfoAlt ? lastStateAzAlt = millis() : connectionFailure++;
  }
}
void Telescope::updateTime()
{
  if (millis() - lastStateTime > updaterate)
  {
    hasInfoUTC = GetLX200(":GL#", TempUTC, sizeof(TempUTC)) == LX200VALUEGET;
    hasInfoSideral = GetLX200(":GS#", TempSideral, sizeof(TempSideral)) == LX200VALUEGET;
    hasInfoUTC && hasInfoSideral ? lastStateTime = millis() : connectionFailure++;
  }
};
void Telescope::updateFocuser()
{
  if (millis() - lastStateFocuser > updaterate*2)
  {
    char fc[45];
    hasInfoFocuser = GetLX200(":F?#", fc, sizeof(TempFocuserStatus)) == LX200VALUEGET;
    if (hasInfoFocuser && fc[0] == '?')
    {
      lastStateFocuser = millis();
      strncpy(TempFocuserStatus, fc, 45);
    }
    else
    {
      hasInfoFocuser = false;
    }
  }
};
void Telescope::updateTel()
{
  if (millis() - lastStateTel > updaterate)
  {
    hasTelStatus = GetLX200(":GU#", TelStatus, sizeof(TelStatus)) == LX200VALUEGET;
    hasTelStatus ? lastStateTel = millis() : connectionFailure++;
  }
};

bool Telescope::connected()
{
  return connectionFailure == 0;
}

bool Telescope::notResponding()
{
  return connectionFailure > 4;
}

Telescope::ParkState Telescope::getParkState()
{
  if (strchr(&TelStatus[0], 'P') != NULL)
  {
    return PRK_PARKED;
  }
  else if (strchr(&TelStatus[0], 'p') != NULL)
  {
    return PRK_UNPARKED;
  }
  else if (strchr(&TelStatus[0], 'I') != NULL)
  {
    return PRK_PARKING;
  }
  else if (strchr(&TelStatus[0], 'F') != NULL)
  {
    return PRK_FAILED;
  }
  return PRK_UNKNOW;
}
Telescope::Mount Telescope::getMount()
{
  switch (TelStatus[12])
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
Telescope::TrackState Telescope::getTrackingState()
{
  if (TelStatus[1] != 'N')
  {
    return TRK_SLEWING;
  }
  else if (TelStatus[0] != 'n')
  {
    return TRK_ON;
  }
  else if (TelStatus[0] == 'n' && TelStatus[1] == 'N')
  {
    return TRK_OFF;
  }
  return TRK_UNKNOW;
}

double Telescope::getLstT0()
{
  char temp[20] = "";
  double f = 0;
  if (GetLX200(":GS#", temp, 20) == LX200VALUEGET)
  {  
    hmsToDouble(&f, temp);
  }
  return f;
};
double Telescope::getLat()
{
  double f = -10000;
  GetLatitudeLX200(f);
  return f;
};
bool Telescope::atHome()
{
  return TelStatus[3] == 'H';
}
bool Telescope::isPulseGuiding()
{
  return  TelStatus[6] == '*';
}
bool Telescope::isGuidingE()
{
  return  TelStatus[7] == '>';
}
bool Telescope::isGuidingW()
{
  return  TelStatus[7] == '<';
}
bool Telescope::isGuidingN()
{
  return  TelStatus[8] == '^';
}
bool Telescope::isGuidingS()
{
  return  TelStatus[8] == '_';
}
bool Telescope::isGNSSValid()
{
  return  TelStatus[14] == '1';
}
Telescope::PierState Telescope::getPierState()
{
  switch (TelStatus[13])
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
Telescope::Errors Telescope::getError()
{
  switch (TelStatus[15])
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
void Telescope::addStar()
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