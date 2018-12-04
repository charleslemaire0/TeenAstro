#include <Arduino.h>
#include <Ephemeris.h>
#include "Catalog.h"
#include "LX200.h"

#define LX200sbuff 20
#define LX200lbuff 50
// integer numeric conversion with error checking
boolean atoi2(char *a, int *i) {
  char *conv_end;
  long l = strtol(a, &conv_end, 10);

  if ((l < -32767) || (l > 32768) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
}

void serialRecvFlush() {
  while (Ser.available() > 0) Ser.read();
}

void char2RA(char* txt, unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  char* pEnd;
  hour = (int)strtol(&txt[0], &pEnd, 10);
  minute = (int)strtol(&txt[3], &pEnd, 10);
  second = (int)strtol(&txt[6], &pEnd, 10);
}

void char2DEC(char* txt, int& deg, unsigned int& min, unsigned int& sec)
{
  char* pEnd;
  deg = (int)strtol(&txt[0], &pEnd, 10);
  min = (int)strtol(&txt[4], &pEnd, 10);
  sec = (int)strtol(&txt[7], &pEnd, 10);
}

// this readBytesUntil() lets you know if the "character" was found
byte readBytesUntil2(char character, char buffer[], int length, boolean* characterFound, long timeout) {
  unsigned long startTime = millis() + timeout;
  int pos = 0;
  *characterFound = false;
  while (((long)(startTime - millis()) > 0) && (pos < length)) {
    if (Ser.available()) {
      buffer[pos] = Ser.read();
      if (buffer[pos] == character) { *characterFound = true; break; }
      pos++;
    }
  }
  return pos;
}

// smart LX200 aware command and response over serial
bool readLX200Bytes(char* command, char* recvBuffer, int bufferSize, unsigned long timeOutMs) {
  Ser.setTimeout(timeOutMs);
  memset(recvBuffer, 0, bufferSize);
  // clear the read/write buffers
  Ser.flush();
  serialRecvFlush();

  // send the command
  Ser.print(command);
  Ser.flush();
  boolean noResponse = false;
  boolean shortResponse = false;
  if ((command[0] == (char)6) && (command[1] == 0)) shortResponse = true;
  if (command[0] == ':') {
    if (command[1] == '%') {
      Ser.setTimeout(timeOutMs * 4);
    }
    if (command[1] == 'A') {
      if (strchr("W123456789+", command[2])) { shortResponse = true; Ser.setTimeout(timeOutMs * 4); }
    }
    if (command[1] == 'M') {
      if (strchr("ewnsg", command[2])) noResponse = true;
      if (strchr("SAF?", command[2])) shortResponse = true;
    }
    if (command[1] == 'Q') {
      if (strchr("#ewns", command[2])) noResponse = true;
    }
    if (command[1] == 'S') {
      if (strchr("!", command[2])) noResponse = true;
      else if (strchr("CLSGtgMNOPrdhoTBX", command[2])) shortResponse = true;
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
    if (command[1] == 'F')
    {
      Ser.setTimeout(timeOutMs*4);
      if (strchr("+-GPSgs", command[2])) { noResponse = true; }
      if (strchr("OoIi:012345678cCmW", command[2])) { shortResponse = true;}
    }
    if (command[1] == 'h') {
      if (strchr("F", command[2])) noResponse = true;
      if (strchr("COPQR", command[2])) { shortResponse = true; Ser.setTimeout(timeOutMs * 2); }
    }
    if (command[1] == 'T') {
      if (strchr("QR+-SLK", command[2])) noResponse = true;
      if (strchr("edrn", command[2])) shortResponse = true;
    }
    if (command[1] == 'U') noResponse = true;
    if ((command[1] == 'W') && (command[2] != '?')) { noResponse = true; }
    if ((command[1] == '$') && (command[2] == 'Q') && (command[3] == 'Z')) {
      if (strchr("+-Z/!", command[4])) noResponse = true;
    }
    if (command[1] == 'G') {
      if (strchr("AZRD", command[2])) { timeOutMs *= 2; }
    }
  }

  if (noResponse) {
    recvBuffer[0] = 0;
    return true;
  }
  else if (shortResponse)
  {
    unsigned long start = millis();
    while (millis() - start < timeOutMs)
    {
      if (Ser.available())
      {
        recvBuffer[Ser.readBytes(recvBuffer, 1)] = 0;
        break;
      }
    }
    return (recvBuffer[0] != 0);
  }
  else 
  {
    // get full response, '#' terminated
    unsigned long start = millis();
    int recvBufferPos = 0;
    char b = 0;
    while (millis() - start < timeOutMs)
    {
      recvBuffer[recvBufferPos] = 0;
      if (Ser.available())
      {
        b = Ser.read();
        if (b == '#')
          break;
        start = millis();
        recvBuffer[recvBufferPos] = b;
        recvBufferPos++;
        if (recvBufferPos > bufferSize - 1) recvBufferPos = bufferSize - 1;
      }
    }
    return (recvBuffer[0] != 0);
  }
}

bool isOk(LX200RETURN val)
{
  return val >= LX200OK;
}

LX200RETURN SetLX200(char* command)
{
  char out[LX200sbuff];
  if (readLX200Bytes(command, out, sizeof(out), TIMEOUT_CMD))
    return  LX200VALUESET;
  else
    return LX200SETVALUEFAILED;
}

LX200RETURN SetBoolLX200(char* command)
{
  char out[LX200sbuff];
  if (readLX200Bytes(command, out, sizeof(out), TIMEOUT_CMD))
    if (out[0] == '1')
      return  LX200VALUESET;
  return LX200SETVALUEFAILED;
}

LX200RETURN SyncGoHomeLX200(bool sync)
{
  char cmd[5];
  sprintf(cmd, ":hX#");
  cmd[2] = sync ? 'F' : 'C';

  char out[LX200sbuff];
  bool ok = readLX200Bytes(cmd, out, sizeof(out), TIMEOUT_CMD);
  if (!ok)
  {
    return LX200SETVALUEFAILED;
  }
  if (sync)
  {
    return  LX200SYNCED;
  }
  else
  {
    if (out[0] == '0')
    {
      return LX200GOHOME_FAILED;
    }
    else
      return LX200GOHOME;
  }
}

LX200RETURN SyncGoParkLX200(bool sync)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":hX#");
  cmd[2] = sync ? 'O' : 'P';

  char out[LX200sbuff];
  bool ok = readLX200Bytes(cmd, out, sizeof(out), TIMEOUT_CMD);
  if (!ok)
  {
    return LX200SETVALUEFAILED;
  }
  if (sync)
  {
    return LX200SYNCED;
  }
  else
  {
    if (out[0] == '0')
    {
      return LX200GOPARK_FAILED;
    }
    else
      return LX200GOPARK;
  }
}

LX200RETURN GetLX200(char* command, char* output, int buffersize)
{
  if (readLX200Bytes(command, output, buffersize, TIMEOUT_CMD))
    return LX200VALUEGET;
  else
    return LX200GETVALUEFAILED;
}

LX200RETURN GetTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second)
{
  char out[LX200sbuff];
  if (GetLX200(":GL#", out, sizeof(out)) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  char2RA(out, hour, minute, second);
  return LX200VALUEGET;
}

LX200RETURN GetTimeLX200(long &value)
{
  unsigned int hour, minute, second;
  if (!GetTimeLX200(hour, minute, second) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  value = hour * 60 + minute;
  value *= 60;
  value += second;
  return LX200VALUEGET;
}

LX200RETURN SetTimeLX200(long &value)
{
  char cmd[LX200sbuff];
  unsigned int hour, minute, second;
  second = value % 60;
  value /= 60;
  minute = value % 60;
  value /= 60;
  hour = value;
  sprintf(cmd, ":SL%02d:%02d:%02d#", hour, minute, second);
  if (SetLX200(cmd) ==  LX200VALUESET)
    return SetLX200(":SG+00#");
  return LX200SETVALUEFAILED;
}

LX200RETURN GetSiteLX200(int& value)
{
  char out[LX200sbuff];
  if (GetLX200(":W?#", out, sizeof(out)) == LX200VALUEGET)
  {
    value = (int)strtol(&out[0], NULL, 10) + 1;
    return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}

LX200RETURN GetLatitudeLX200(int& degree, int& minute)
{
  char out[LX200sbuff];
  if (GetLX200(":Gt#", out, sizeof(out)) == LX200VALUEGET)
  {
    char* pEnd;
     degree = (int)strtol(&out[0], &pEnd, 10);
     minute = (int)strtol(&out[4], &pEnd, 10);
    return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}

LX200RETURN GetLongitudeLX200(int& degree, int& minute)
{
  char out[LX200sbuff];
  if (GetLX200(":Gg#", out, sizeof(out)) == LX200VALUEGET)
  {
    char* pEnd;
    degree = (int)strtol(&out[0], &pEnd, 10);
    minute = (int)strtol(&out[5], &pEnd, 10);
    return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}

void SetSiteLX200(int& value)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":W%d#", value);
  Ser.print(cmd);
  Ser.flush();
}

LX200RETURN Move2TargetLX200()
{
  char out[LX200sbuff];
  int val = readLX200Bytes(":MS#", out, sizeof(out), TIMEOUT_CMD);
  LX200RETURN response;

  switch (out[0]-'0')
  {
    //         1=Object below horizon    Outside limits, below the Horizon limit
    //         2=No object selected      Failure to resolve coordinates
    //         4=Position unreachable    Not unparked
    //         5=Busy                    Goto already active
    //         6=Outside limits          Outside limits, above the Zenith limit
  case 0:
    response = LX200GOINGTO;
    break;
  case 1:
    response = LX200BELOWHORIZON;
    break;
  case 2:
    response = LX200NOOBJECTSELECTED;
    break;
  case 4:
    response = LX200PARKED;
    break;
  case 5:
    response = LX200BUSY;
    break;
  case 6:
    response = LX200LIMITS;
    break;
  case 7:
    response = LX200BUSY;
    break;
  case 11:
    response = LX200_ERR_MOTOR_FAULT;
    break;
  case 12:
    response = LX200_ERR_ALT;
    break;
  case 13:
    response = LX200_ERR_LIMIT_SENSE;
    break;
  case 14:
    response = LX200_ERR_DEC;
    break;
  case 15:
    response = LX200_ERR_AZM;
    break;
  case 16:
    response = LX200_ERR_UNDER_POLE;
    break;
  case 17:
    response = LX200_ERR_MERIDIAN;
    break;
  case 18:
    response = LX200_ERR_SYNC;
    break;
  default:
    response = LX200UNKOWN;
    break;
  }
  return response;
}

LX200RETURN SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3)
{
  char cmd[LX200sbuff], out[LX200sbuff];
  int iter = 0;
  sprintf(cmd, ":Sr%02u:%02u:%02u#", vr1, vr2, vr3);
  while (iter < 3)
  {
    if (SetLX200(cmd) ==  LX200VALUESET)
    {
      if (GetLX200(":Gr#", out, sizeof(out)) == LX200VALUEGET)
      {
        unsigned int hour, minute, second;
        char2RA(out, hour, minute, second);
        if (hour == vr1 && minute == vr2 && second == vr3)
        {
          return LX200VALUESET;
        }
      }
    }
    iter++;
  }
  return LX200SETVALUEFAILED;
}

LX200RETURN SetTargetDecLX200(short& vd1, uint8_t& vd2, uint8_t& vd3)
{
  char cmd[LX200sbuff], out[LX200sbuff];
  int iter = 0;
  sprintf(cmd, ":Sd%+03d:%02u:%02u#", vd1, vd2, vd3);
  while (iter < 3)
  {
    if (SetLX200(cmd) ==  LX200VALUESET)
    {
      if (GetLX200(":Gd#", out, sizeof(out)) == LX200VALUEGET)
      {
        int deg;
        unsigned int min, sec;
        char2DEC(out, deg, min, sec);
        if (deg == vd1 && min == vd2 && sec == vd3)
        {
          return LX200VALUESET;
        }
      }
    }
    iter++;
  }
  return LX200SETVALUEFAILED;
}

LX200RETURN SyncGotoLX200(bool sync, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, short& vd1, uint8_t& vd2, uint8_t& vd3)
{
  if (SetTargetRaLX200(vr1, vr2, vr3) ==  LX200VALUESET && SetTargetDecLX200(vd1, vd2, vd3) == LX200VALUESET)
  {
    if (sync)
    {
      char out[LX200sbuff];
      if (GetLX200(":CM#",out, LX200sbuff))
      {
        if (strcmp(out, "N/A") == 0)
          return LX200SYNCED;
        else
          return LX200_ERR_SYNC;
      }
      else
        return LX200_ERR_SYNC;
    }
    else
    {
      return Move2TargetLX200();
    }
  }
  else
  {
    return LX200SETTARGETFAILED;
  }
}

LX200RETURN SyncGotoLX200(bool sync, float &Ra, float &Dec)
{
  int ivr1, ivr2, ivd1, ivd2;
  float fvr3, fvd3;
  Ephemeris::floatingHoursToHoursMinutesSeconds(Ra, &ivr1, &ivr2, &fvr3);
  Ephemeris::floatingDegreesToDegreesMinutesSeconds(Dec, &ivd1, &ivd2, &fvd3);
  uint8_t vr1, vr2, vr3, vd2, vd3;
  short vd1;
  vr1 = ivr1;
  vr2 = ivr2;
  vr3 = (int)fvr3;
  vd1 = ivd1;
  vd2 = ivd2;
  vd3 = (int)fvd3;
  return SyncGotoLX200(sync, vr1, vr2, vr3, vd1, vd2, vd3);
}

LX200RETURN GetDateLX200(unsigned int &day, unsigned int &month, unsigned int &year)
{
  char out[LX200sbuff];
  if (GetLX200(":GC#", out, sizeof(out)) == LX200VALUEGET)
  {
    char* pEnd;
    month = strtol(&out[0], &pEnd, 10);
    day = strtol(&out[3], &pEnd, 10);
    year = strtol(&out[6], &pEnd, 10) + 2000L;
    return LX200VALUEGET;
  }
  else
  {
    return LX200GETVALUEFAILED;
  }
}

LX200RETURN SyncGotoCatLX200(bool sync, Catalog cat, int idx)
{
  int epoch;
  unsigned int day, month, year, hour, minute, second;
  if (GetDateLX200(day, month, year) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
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
    getcathf(Messier_ra[idx], ra);
    getcatdf(Messier_dec[idx], dec);
    epoch = 2000;
    break;
  case HERSCHEL:
    getcathf(Herschel_ra[idx], ra);
    getcatdf(Herschel_dec[idx], dec);
    epoch = 1950;
    break;
  default:
    return LX200UNKOWN;
    break;
  }
  EquatorialCoordinates coo;
  coo.ra = ra;
  coo.dec = dec;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(coo, epoch, day, month, year, 0, 0, 0);
  return SyncGotoLX200(sync, cooNow.ra, cooNow.dec);
}
LX200RETURN SyncGotoPlanetLX200(bool sync, unsigned short objSys)
{
  unsigned int day, month, year, hour, minute, second;
  int degreeLat, minuteLat, degreeLong, minuteLong;
  if (GetDateLX200(day, month, year) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  if (GetTimeLX200(hour, minute, second) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  if (GetLongitudeLX200(degreeLong, minuteLong) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  if (GetLatitudeLX200(degreeLat, minuteLat) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  
  Ephemeris Eph;
  Eph.flipLongitude(true);
  Eph.setLocationOnEarth(degreeLat, minuteLat, 0, degreeLong, minuteLong, 0);
  SolarSystemObjectIndex objI = static_cast<SolarSystemObjectIndex>(objSys);
  SolarSystemObject obj = Eph.solarSystemObjectAtDateAndTime(objI, day, month, year, hour, minute, second);
  return SyncGotoLX200(sync, obj.equaCoordinates.ra, obj.equaCoordinates.dec);
}

LX200RETURN SyncSelectedStarLX200(unsigned short alignSelectedStar)
{
  if (alignSelectedStar > 0 && alignSelectedStar < 292)
  {
    return SyncGotoCatLX200(false, STAR, alignSelectedStar - 1);
  }
  else
    return LX200UNKOWN;
}

LX200RETURN readReverseLX200(const uint8_t &axis, bool &reverse)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%RR#", out , sizeof(out)) : GetLX200(":%RD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    reverse = out[0] == '1' ? true : false;
  }
  return ok;
}

LX200RETURN writeReverseLX200(const uint8_t &axis, const bool &reverse)
{
  char text[LX200sbuff];
  sprintf(text, ":$RX%u#", (unsigned int)reverse);
  text[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(text);
}
LX200RETURN readBacklashLX200(const uint8_t &axis, float &backlash)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%BR#", out, sizeof(out)) : GetLX200(":%BD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    backlash = (float)strtol(&out[0], NULL, 10);
  }
  return ok;
}
LX200RETURN writeBacklashLX200(const uint8_t &axis, const float &backlash)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":$BX%u#", (unsigned int)backlash);
  cmd[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readTotGearLX200(const uint8_t &axis, float &totGear)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%GR#", out, sizeof(out)) : GetLX200(":%GD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    totGear = (float)strtol(&out[0], NULL, 10);
  }
  return ok;
}
LX200RETURN writeTotGearLX200(const uint8_t &axis, const float &totGear)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":$GX%u#", (unsigned int)totGear);
  cmd[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readStepPerRotLX200(const uint8_t &axis, float &stepPerRot)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%SR#", out, sizeof(out)) : GetLX200(":%SD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    stepPerRot = (float)strtol(&out[0], NULL, 10);
  }
  return ok;
}
LX200RETURN writeStepPerRotLX200(const uint8_t &axis, const float &stepPerRot)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":$SX%u#", (unsigned int)stepPerRot);
  cmd[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readMicroLX200(const uint8_t &axis, uint8_t &microStep)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%MR#", out, sizeof(out)) : GetLX200(":%MD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);

    if ((value >= 0 && value < 9))
    {
      microStep = value;
      return ok;
    }
    return LX200GETVALUEFAILED;
  }
  return ok;
}
LX200RETURN writeMicroLX200(const uint8_t &axis, const uint8_t &microStep)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":$MX%u#", microStep);
  cmd[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readLowCurrLX200(const uint8_t &axis, uint8_t &lowCurr)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%cR#", out, sizeof(out)) : GetLX200(":%cD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);
    if (value >= 0 && value < 256)
    {
      lowCurr = value;
      return ok;
    }
    return LX200GETVALUEFAILED;
  }
  return ok;
}
LX200RETURN writeLowCurrLX200(const uint8_t &axis, const uint8_t &lowCurr)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":$cX%u#", lowCurr);
  cmd[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readHighCurrLX200(const uint8_t &axis, uint8_t &highCurr)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":%CR#", out, sizeof(out)) : GetLX200(":%CD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);
    if (value >= 0 && value < 256)
    {
      highCurr = value;
      return ok;
    }
    return LX200GETVALUEFAILED;
  }
  return ok;
}
LX200RETURN writeHighCurrLX200(const uint8_t &axis, const uint8_t &highCurr)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":$CX%u#", highCurr);
  cmd[3] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}

LX200RETURN readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                              unsigned int& minSpeed, unsigned int& maxSpeed,
                              unsigned int& cmdAcc, unsigned int& manAcc, unsigned int& manDec)
{
  char out[LX200lbuff];
  if (GetLX200(":F~#", out, sizeof(out)) == LX200GETVALUEFAILED)
    if (GetLX200(":F~#", out, sizeof(out)) == LX200GETVALUEFAILED)
      if (GetLX200(":F~#", out, sizeof(out)) == LX200GETVALUEFAILED)
        return LX200GETVALUEFAILED;
  if (strlen(out) != 32)
    return LX200GETVALUEFAILED;
  char* pEnd;
  startPosition = strtol(&out[1], &pEnd, 10);
  maxPosition = strtol(&out[7], &pEnd, 10);
  minSpeed = strtol(&out[13], &pEnd, 10);
  maxSpeed = strtol(&out[17], &pEnd, 10);
  cmdAcc = strtol(&out[21], &pEnd, 10);
  manAcc = strtol(&out[25], &pEnd, 10);
  manDec = strtol(&out[29], &pEnd, 10);
  return LX200VALUEGET;
}

LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr,  unsigned int& curr)
{
  char out[LX200lbuff];
  if (GetLX200(":FM#", out, sizeof(out)) == LX200GETVALUEFAILED)
    if (GetLX200(":FM#", out, sizeof(out)) == LX200GETVALUEFAILED)
      if (GetLX200(":FM#", out, sizeof(out)) == LX200GETVALUEFAILED)
        return LX200GETVALUEFAILED;
  if (strlen(out) != 12)
    return LX200GETVALUEFAILED;
  char* pEnd;
  reverse = out[1] == '1';
  micro = out[3] - '0';
  incr = strtol(&out[5], &pEnd, 10);
  curr = strtol(&out[9], &pEnd, 10);
  return LX200VALUEGET;
}

 
