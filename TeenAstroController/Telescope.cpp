#include "Telescope.h"
#include <Ephemeris.h>


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

bool Telescope::GetLX200(char* command, char* output, bool silent)
{
  memset(output, 0, sizeof(output));
  bool ok = readLX200Bytes(command, output, 100);
  if (!silent && !ok)
  {
    //DisplayMessage("Read Value!", "Failed", 500);
    1;
  }
  return ok;
}

bool Telescope::GetTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second)
{
  char out[20];
  if (!GetLX200(":GL#", out, false))
    return false;
  char2RA(out, hour, minute, second);
  long value = hour * 60 + minute;
  value *= 60;
  value += second;
  char2RA(out, hour, minute, second);
  return true;
}

bool Telescope::GetTimeLX200(long &value)
{
  unsigned int hour, minute, second;
  if (!GetTimeLX200(hour, minute, second))
    return false;
  value = hour * 60 + minute;
  value *= 60;
  value += second;
  return true;
}

bool Telescope::SetTimeLX200(long &value)
{
  char out[20];
  unsigned int hour, minute, second;
  second = value % 60;
  value /= 60;
  minute = value % 60;
  value /= 60;
  hour = value;
  sprintf(out, ":SL%02d:%02d:%02d#", hour, minute, second);
  bool ok = SetLX200(out);
  ok = ok && SetLX200(":SG+00#", true);
  return ok;
}

bool Telescope::GetSiteLX200(int& value)
{
  char out[20];
  if (GetLX200(":W?#", out, false))
  {
    value = (int)strtol(&out[0], NULL, 10) + 1;
    return true;
  }
  return false;
}
void Telescope::SetSiteLX200(int& value)
{
  char out[20];
  sprintf(out, ":W%d#", value);
  Serial.print(out);
  Serial.flush();
}

bool Telescope::SetLX200(char* command, bool silent)
{
  char out[20];
  memset(out, 0, sizeof(out));
  bool ok = readLX200Bytes(command, out, 100);
  if (!silent)
  {
    if (ok)
      //DisplayMessage("Value Set!", NULL, 500);
      1;
    else
      1;
      //DisplayMessage("Value Set!", "Failed", -1);
  }
  return ok;
}

void Telescope::Move2TargetLX200(bool silent)
{
  char out[20];
  memset(out, 0, sizeof(out));
  int val = readLX200Bytes(":MS#", out, 100);
  switch (val)
  {
    //         1=Object below horizon    Outside limits, below the Horizon limit
    //         2=No object selected      Failure to resolve coordinates
    //         4=Position unreachable    Not unparked
    //         5=Busy                    Goto already active
    //         6=Outside limits          Outside limits, above the Zenith limit
  case 0:
    //TODO DisplayMessage("Slew to", "Target", 1000);
    break;
  case 1:
    //TODO DisplayMessage("Object below", "Horizon", -1);
    break;
  case 2:
    //TODO DisplayMessage("No Object", "Selected", -1);
    break;
  case 4:
    //TODO DisplayMessage("Telescope", "is Parked", -1);
    break;
  case 5:
    //TODO DisplayMessage("Telescope", "is busy", -1);
    break;
  case 6:
    //TODO DisplayMessage("Outside", "Limits", -1);
    break;
  default:
    char text[15];
    sprintf(text, "Error %d", val);
    ///TODO DisplayMessage("Unknown", text, -1);
    break;
  }
}

bool Telescope::SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3)
{
  char cmd[20], out[20];
  int iter = 0;
  sprintf(cmd, ":Sr%02u:%02u:%02u#", vr1, vr2, vr3);
  while (iter < 3)
  {
    if (SetLX200(cmd, true))
    {
      if (GetLX200(":Gr#", out, true))
      {
        unsigned int hour, minute, second;
        char2RA(out, hour, minute, second);
        if (hour == vr1 && minute == vr2 && second == vr3)
        {
          return true;
        }
      }
    }
    iter++;
  }
  return false;
}

bool Telescope::SetTargetDecLX200(short& vd1, uint8_t& vd2, uint8_t& vd3)
{
  char  cmd[20], out[20];
  int iter = 0;
  sprintf(cmd, ":Sd%+03d:%02u:%02u#", vd1, vd2, vd3);
  while (iter < 3)
  {
    if (SetLX200(cmd, true))
    {
      if (GetLX200(":Gd#", out, true))
      {
        int deg;
        unsigned int min, sec;
        char2DEC(out, deg, min, sec);
        if (deg == vd1 && min == vd2 && sec == vd3)
        {
          return true;
        }
      }
    }
    iter++;
  }
  return false;
}


bool Telescope::SyncGotoLX200(bool sync, float& Ra, float& Dec)
{
  int ivr1, ivr2, ivd1, ivd2;
  float fvr3, fvd3;
  Ephemeris::floatingHoursToHoursMinutesSeconds(Ra, &ivr1, &ivr2, &fvr3);
  Ephemeris::floatingDegreesToDegreesMinutesSeconds(Dec, &ivd1, &ivd2, &fvd3);
  uint8_t vr1, vr2, vr3, vd2, vd3;
  short vd1;
  vr1 = (uint8_t)ivr1;
  vr2 = (uint8_t)ivr2;
  vr3 = (uint8_t)fvr3;
  vd1 = (short)ivd1;
  vd1 = (uint8_t)ivd2;
  vd3 = (uint8_t)fvd3;
  return SyncGotoLX200(sync, vr1, vr2, vr3, vd1, vd2, vd3);
}

bool Telescope::SyncGotoLX200(bool sync, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, short& vd1, uint8_t& vd2, uint8_t& vd3)
{

  bool ok = SetTargetRaLX200(vr1, vr2, vr3);
  ok = ok && SetTargetDecLX200(vd1, vd2, vd3);
  if (ok)
  {
    if (sync)
    {
      Serial.print(":CS#");
      Serial.flush();
      //TODO DisplayMessage("Synced with", "Target", 500);
    }
    else
    {
      Move2TargetLX200();
    }
    return true;
  }
  else
  {
    //TODO DisplayMessage("Set Target", "Failed!", 500);
  }
  return false;
}


bool Telescope::GetDateLX200(unsigned int &day, unsigned int &month, unsigned int &year)
{
  char out[20];
  if (GetLX200(":GC#", out, true))
  {
    char* pEnd;
    month = strtol(&out[0], &pEnd, 10);
    day = strtol(&out[3], &pEnd, 10);
    year = strtol(&out[6], &pEnd, 10) + 2000L;
  }
  else
  {
    //TODO DisplayMessage(" Telescope is", "not Responding", -1);
    return false;
  }
  return true;
}

bool Telescope::SyncGotoCatLX200(bool sync, Catalog cat, int idx)
{
  int epoch;
  unsigned int day, month, year, hour, minute, second;
  if (!GetDateLX200(day, month, year))
  {
    return false;
  }
  float ra, dec;
  switch (cat)
  {
  case STAR:
    getcathf(Star_ra[idx], ra);
    getcatdf(Star_dec[idx], dec);
    epoch = 2000;
    break;
  case MESSIER:
    return false;
    break;
  case HERSCHEL:
    getcathf(Herschel_ra[idx], ra);
    getcatdf(Herschel_dec[idx], dec);
    epoch = 1950;
    break;
  default:
    return false;
    break;
  }
  EquatorialCoordinates coo;
  coo.ra = ra;
  coo.dec = dec;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(coo, epoch, day, month, year, 0, 0, 0);
  return SyncGotoLX200(sync, coo.ra, coo.dec);
}

bool Telescope::SyncGotoPlanetLX200(bool sync, unsigned short objSys)
{
  char out[20];
  unsigned int day, month, year, hour, minute, second;
 
  if (!GetDateLX200(day,month,year))
  {
   //Todo  DisplayMessage(" Telescope is", "not Responding", -1);
    return false;
  }
  if (!GetTimeLX200(hour, minute, second))
  {
    //Todo  DisplayMessage(" Telescope is", "not Responding", -1);
    return false;
  }

  Ephemeris Eph;
  SolarSystemObjectIndex objI = static_cast<SolarSystemObjectIndex>(objSys);
  SolarSystemObject obj = Eph.solarSystemObjectAtDateAndTime(objI, day, month, year, hour, minute, second);
  return SyncGotoLX200(sync, obj.equaCoordinates.ra, obj.equaCoordinates.dec);
}


bool Telescope::SyncSelectedStarLX200()
{
  if (alignSelectedStar > 0 && alignSelectedStar < 292)
  {
    return SyncGotoCatLX200(false, STAR, alignSelectedStar - 1);
  }
  else
    return false;
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

// integer numeric conversion with error checking
boolean atoi2(char *a, int *i) {
  char *conv_end;
  long l = strtol(a, &conv_end, 10);

  if ((l<-32767) || (l>32768) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
}

// this readBytesUntil() lets you know if the "character" was found
byte readBytesUntil2(char character, char buffer[], int length, boolean* characterFound, long timeout) {
  unsigned long startTime = millis() + timeout;
  int pos = 0;
  *characterFound = false;
  while (((long)(startTime - millis())>0) && (pos<length)) {
    if (Serial.available()) {
      buffer[pos] = Serial.read();
      if (buffer[pos] == character) { *characterFound = true; break; }
      pos++;
    }
  }
  return pos;
}

// smart LX200 aware command and response over serial
int Telescope::readLX200Bytes(char* command, char* recvBuffer, unsigned long timeOutMs) {
  Serial.setTimeout(timeOutMs);

  // clear the read/write buffers
  Serial.flush();
  serialRecvFlush();

  // send the command
  Serial.print(command);

  boolean noResponse = false;
  boolean shortResponse = false;
  if ((command[0] == (char)6) && (command[1] == 0)) shortResponse = true;
  if (command[0] == ':') {
    if (command[1] == '%') {
      Serial.setTimeout(timeOutMs * 4);
    }
    if (command[1] == 'A') {
      if (strchr("W123456789+", command[2])) { shortResponse = true; Serial.setTimeout(timeOutMs * 4); }
    }
    if (command[1] == 'M') {
      if (strchr("ewnsg", command[2])) noResponse = true;
      if (strchr("SA", command[2])) shortResponse = true;
    }
    if (command[1] == 'Q') {
      if (strchr("#ewns", command[2])) noResponse = true;
    }
    if (command[1] == 'S') {
      if (strchr("CLSGtgMNOPrdhoTB", command[2])) shortResponse = true;
    }
    if (command[1] == 'L') {
      if (strchr("BNCDL!", command[2])) noResponse = true;
      if (strchr("o$W", command[2])) shortResponse = true;
    }
    if (command[1] == 'B') {
      if (strchr("+-", command[2])) noResponse = true;
    }
    if (command[1] == 'C') {
      if (strchr("S", command[2])) noResponse = true;
    }
    if (command[1] == 'h') {
      if (strchr("F", command[2])) noResponse = true;
      if (strchr("COPQR", command[2])) { shortResponse = true; Serial.setTimeout(timeOutMs * 2); }
    }
    if (command[1] == 'T') {
      if (strchr("QR+-SLK", command[2])) noResponse = true;
      if (strchr("edrn", command[2])) shortResponse = true;
    }
    if (command[1] == 'U') noResponse = true;
    if ((command[1] == 'W') && (command[2] != '?')) { noResponse = true; }
    if ((command[1] == 'W') && (command[2] == '?')) { shortResponse = true; }
    if ((command[1] == '$') && (command[2] == 'Q') && (command[3] == 'Z')) {
      if (strchr("+-Z/!", command[4])) noResponse = true;
    }
    if (command[1] == 'G') {
      if (strchr("RD", command[2])) { timeOutMs *= 2; }
    }
  }

  if (noResponse) {
    recvBuffer[0] = 0;
    return true;
  }
  else
    if (shortResponse) {
      recvBuffer[Serial.readBytes(recvBuffer, 1)] = 0;
      return (recvBuffer[0] - '0');
    }
    else {
      // get full response, '#' terminated
      unsigned long start = millis();
      int recvBufferPos = 0;
      char b = 0;
      while (millis() - start < timeOutMs && (b != '#')) {
        if (Serial.available()) {
          b = Serial.read();
          recvBuffer[recvBufferPos] = b; recvBufferPos++; if (recvBufferPos>19) recvBufferPos = 19; recvBuffer[recvBufferPos] = 0;
        }
      }
      return (recvBuffer[0] != 0);
    }
}

void Telescope::serialRecvFlush() {
  while (Serial.available()>0) Serial.read();
}

void Telescope::char2RA(char* txt, unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  char* pEnd;
  hour = (int)strtol(&txt[0], &pEnd, 10);
  minute = (int)strtol(&txt[3], &pEnd, 10);
  second = (int)strtol(&txt[6], &pEnd, 10);
}

void Telescope::char2DEC(char* txt, int& deg, unsigned int& min, unsigned int& sec)
{
  char* pEnd;
  deg = (int)strtol(&txt[0], &pEnd, 10);
  min = (int)strtol(&txt[4], &pEnd, 10);
  sec = (int)strtol(&txt[7], &pEnd, 10);
}