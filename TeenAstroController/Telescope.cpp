#include "Telescope.h"
#include "LX200.h"

void Telescope::updateRaDec()
{
  if (millis() - lastStateRaDec > 100 && connected)
  {
    hasInfoRa = GetLX200(":GR#", TempRa, true);
    hasInfoDec = GetLX200(":GD#", TempDec, true);
    lastStateRaDec = millis();
    if (!hasInfoRa && !hasInfoDec)
    {
      connected = true;
    }
  }
};
void Telescope::updateAzAlt()
{
  if (millis() - lastStateAzAlt > 100 && connected)
  {
    hasInfoAz = GetLX200(":GZ#", TempAz, true);
    hasInfoAlt = GetLX200(":GA#", TempAlt, true);
    lastStateAzAlt = millis();
    if (!hasInfoAz && !hasInfoAlt)
    {
      connected = true;
    }
  }
}
void Telescope::updateTime()
{
  if (millis() - lastStateTime > 100 && connected)
  {
    hasInfoUTC = GetLX200(":GL#", TempUTC, true);
    hasInfoSideral = GetLX200(":GS#", TempSideral, true);
    lastStateTime = millis();
    if (!hasInfoUTC && !hasInfoSideral)
    {
      connected = true;
    }
  }
};
void Telescope::updateTel()
{
  if (millis() - lastStateTel > 100 && connected)
  {
    hasPierInfo = GetLX200(":Gm#", sideofpier, true);
    hasTelStatus = GetLX200(":GU#", TelStatus, true);
    lastStateTel = millis();
  }
  if (!hasPierInfo && !hasTelStatus)
  {
    connected = true;
  }
};
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
Telescope::TrackState Telescope::getTrackingState()
{
  if (strchr(&TelStatus[0], 'N') == NULL)
  {
    return TRK_SLEWING;
  }
  else if (strchr(&TelStatus[0], 'n') == NULL)
  {
    return TRK_ON;
  }
  if (strchr(&TelStatus[0], 'n') != NULL && strchr(&TelStatus[0], 'N') != NULL)
  {
    return TRK_OFF;
  }
  return TRK_UNKNOW;
}
bool Telescope::atHome()
{
  return strchr(&TelStatus[0], 'H') != NULL;
}
bool Telescope::isGuiding()
{
  return  strchr(&TelStatus[0], 'G') != NULL;
}
Telescope::PierState Telescope::getPierState()
{
  if (strchr(&sideofpier[0], 'E') != NULL)
  {
    return PIER_E;
  }
  else if (strchr(&sideofpier[0], 'W') != NULL)
  {
    return PIER_W;
  }
  return PIER_UNKNOW;
}
Telescope::Errors Telescope::getError()
{
  if (strlen(TelStatus) > 2)
  {
    int l = strlen(TelStatus) - 2;
    switch (TelStatus[l])
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
}
void Telescope::addStar()
{
  if (align == ALI_RECENTER_1 || align == ALI_RECENTER_2 || align == ALI_RECENTER_3)
  {
    if (SetLX200(":A+#", true))
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