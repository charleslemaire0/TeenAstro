#include <Arduino.h>
#include <Ephemeris.h>
#include <TeenAstroLX200io.h>
#include <TeenAstroFunction.h>

#define LX200sbuff 20
#define LX200lbuff 50


// integer numeric conversion with error checking
boolean atoi2(char *a, int *i)
{
  char *conv_end;
  long l = strtol(a, &conv_end, 10);

  if ((l < -32767) || (l > 32768) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
}

void serialRecvFlush()
{
  while (Ser.available() > 0) Ser.read();
}

void char2RA(char* txt, unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  char* pEnd;
  hour = (int)strtol(&txt[0], &pEnd, 10);
  minute = (int)strtol(&txt[3], &pEnd, 10);
  second = (int)strtol(&txt[6], &pEnd, 10);
}

void char2AZ(char* txt, unsigned int& deg, unsigned int& min, unsigned int& sec)
{
  char* pEnd;
  deg = (int)strtol(&txt[0], &pEnd, 10);
  min = (int)strtol(&txt[4], &pEnd, 10);
  sec = (int)strtol(&txt[6], &pEnd, 10);
}

void char2DEC(char* txt, int& deg, unsigned int& min, unsigned int& sec)
{
  char* pEnd;
  deg = (int)strtol(&txt[0], &pEnd, 10);
  min = (int)strtol(&txt[4], &pEnd, 10);
  sec = (int)strtol(&txt[7], &pEnd, 10);
}


// this readBytesUntil() lets you know if the "character" was found
byte readBytesUntil2(char character, char buffer[], int length, boolean* characterFound, long timeout)
{
  unsigned long startTime = millis() + timeout;
  int pos = 0;
  *characterFound = false;
  while (((long)(startTime - millis()) > 0) && (pos < length))
  {
    if (Ser.available())
    {
      buffer[pos] = Ser.read();
      if (buffer[pos] == character)
      {
        *characterFound = true; break;
      }
      pos++;
    }
  }
  return pos;
}



// smart LX200 aware command and response over serial
bool readLX200Bytes(char* command, char* recvBuffer, int bufferSize, unsigned long timeOutMs, bool keepHashtag)
{
  enum CMDREPLY
  {
    CMDR_NO, CMDR_SHORT, CMDR_LONG, CMDR_INVALID
  };
  CMDREPLY cmdreply = CMDR_NO;
  Ser.setTimeout(timeOutMs);
  memset(recvBuffer, 0, bufferSize);
  // clear the read/write buffers
  Ser.flush();
  serialRecvFlush();
  // send the command

  if ((command[0] == (char)6) && (command[1] == 0)) cmdreply = CMDR_SHORT;
  if (command[0] == ':')
  {
    switch (command[1])
    {
    case 'A':
      if (strchr("023CW", command[2])) cmdreply = CMDR_SHORT;
      else cmdreply = CMDR_INVALID;
      break;
    case 'B':
      if (strchr("+-", command[2])) cmdreply = CMDR_NO;
      else cmdreply = CMDR_INVALID;
      break;
    case 'C':
      if (strchr("AMSU", command[2])) cmdreply = CMDR_LONG;
      else cmdreply = CMDR_INVALID;
      break;
    case 'D':
      if (strchr("#", command[2])) cmdreply = CMDR_LONG;
      else cmdreply = CMDR_INVALID;
      break;
    case 'F':
      if (strchr("+-gGPQsS", command[2])) cmdreply = CMDR_NO;
      else if (strchr("OoIi:012345678cCmrW", command[2])) cmdreply = CMDR_SHORT;
      else if (strchr("x?~MV", command[2])) cmdreply = CMDR_LONG;
      else cmdreply = CMDR_INVALID;
      break;
    case 'g':
      if (strchr("ts", command[2])) cmdreply = CMDR_SHORT;
      else cmdreply = CMDR_INVALID;
      break;
    case 'G':
      if (strchr("AaCcDdegGhLMNOPmnoRrSTtVXZ", command[2]))
      {
        timeOutMs *= 2;
        cmdreply = CMDR_LONG;
      }
      else cmdreply = CMDR_INVALID;
      break;
    case 'h':
      if (strchr("F", command[2])) cmdreply = CMDR_NO;
      else if (strchr("COPQR", command[2]))
      {
        cmdreply = CMDR_SHORT;
      }
      else cmdreply = CMDR_INVALID;
      break;
    case 'M':
      if (strchr("ewnsg", command[2])) cmdreply = CMDR_NO;
      else if (strchr("SAF@U", command[2])) cmdreply = CMDR_SHORT;
      else if (strchr("?", command[2])) cmdreply = CMDR_LONG;
      else cmdreply = CMDR_INVALID;
      break;
    case 'Q':
      if (strchr("#ewns", command[2])) cmdreply = CMDR_NO;
      else cmdreply = CMDR_INVALID;
      break;
    case 'R':
      if (strchr("GCMS01234", command[2])) cmdreply = CMDR_NO;
      else cmdreply = CMDR_INVALID;
      break;
    case 'S':
      if (strchr("!", command[2])) cmdreply = CMDR_NO;
      else if (strchr("aCeLSGtgmnMNOPrdhoTBXzU", command[2])) cmdreply = CMDR_SHORT;
      else cmdreply = CMDR_INVALID;
      break;
    case 'T':
      if (strchr("QR+-SLK", command[2])) cmdreply = CMDR_NO;
      else if (strchr("edrnc", command[2])) cmdreply = CMDR_SHORT;
      else cmdreply = CMDR_INVALID;
      break;
    case 'U':
      if (strchr("#", command[2])) cmdreply = CMDR_NO;
      else cmdreply = CMDR_INVALID;
      break;
    case 'W':
      if (strchr("0123", command[2])) cmdreply = CMDR_NO;
      else if (strchr("?", command[2])) cmdreply = CMDR_LONG;
      else cmdreply = CMDR_INVALID;
      break;
    case'$':
      if (strchr("$!", command[2])) cmdreply = CMDR_NO;
      else if (strchr("X", command[2])) cmdreply = CMDR_SHORT;
      else cmdreply = CMDR_INVALID;
      break;
    default:
      cmdreply = CMDR_INVALID;
      break;
    }
  }
  else cmdreply = CMDR_INVALID;

  if (cmdreply == CMDR_INVALID) return false;

  Ser.print(command);
 
  switch (cmdreply)
  {
  case CMDR_NO:
    recvBuffer[0] = 0;
    return true;
    break;
  case CMDR_SHORT:
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
    break;
  }
  case CMDR_LONG:
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
        {
          if (keepHashtag)
          {
            recvBuffer[recvBufferPos] = b;
            recvBufferPos++;
          }
          break;
        }

        start = millis();
        recvBuffer[recvBufferPos] = b;
        recvBufferPos++;
        if (recvBufferPos > bufferSize - 1) recvBufferPos = bufferSize - 1;
      }
    }
    return (recvBuffer[0] != 0);
    break;
  }
  default:
    break;
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

LX200RETURN GetLX200Float(char* command, float* value)
{
  char out[LX200sbuff];
  char* conv_end;
  if (GetLX200(command, out, sizeof(out)) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  float f = strtod(out, &conv_end);
  if ((&out[0] != conv_end) && (f >= -12 && f <= 12.0))
  {
    *value = f;
    return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}

LX200RETURN GetLocalTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second)
{
  char out[LX200sbuff];
  if (GetLX200(":GL#", out, sizeof(out)) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  char2RA(out, hour, minute, second);
  return LX200VALUEGET;
}

LX200RETURN GetUTCTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second)
{
  char out[LX200sbuff];
  if (GetLX200(":GXT0#", out, sizeof(out)) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  char2RA(out, hour, minute, second);
  return LX200VALUEGET;
}

LX200RETURN GetRALX200(unsigned int &hour, unsigned int &minute, unsigned int &second)
{
  char out[LX200sbuff];
  if (GetLX200(":GR#", out, sizeof(out)) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  char2RA(out, hour, minute, second);
  return LX200VALUEGET;
}

LX200RETURN GetLocalTimeLX200(long &value)
{
  unsigned int hour, minute, second;
  if (!GetLocalTimeLX200(hour, minute, second) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  value = hour * 60 + minute;
  value *= 60;
  value += second;
  return LX200VALUEGET;
}

LX200RETURN GetUTCTimeLX200(long &value)
{
  unsigned int hour, minute, second;
  if (!GetUTCTimeLX200(hour, minute, second) == LX200GETVALUEFAILED)
    return LX200GETVALUEFAILED;
  value = hour * 60 + minute;
  value *= 60;
  value += second;
  return LX200VALUEGET;
}

LX200RETURN SetLocalTimeLX200(long &value)
{
  char cmd[LX200sbuff];
  unsigned int hour, minute, second;
  second = value % 60;
  value /= 60;
  minute = value % 60;
  value /= 60;
  hour = value;
  sprintf(cmd, ":SL%02d:%02d:%02d#", hour, minute, second);
  return SetLX200(cmd);
}

LX200RETURN SetUTCTimeLX200(long &value)
{
  char cmd[LX200sbuff];
  unsigned int hour, minute, second;
  second = value % 60;
  value /= 60;
  minute = value % 60;
  value /= 60;
  hour = value;
  sprintf(cmd, ":SXT0%02d:%02d:%02d#", hour, minute, second);
  return SetLX200(cmd);
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

LX200RETURN GetLatitudeLX200(double& degree)
{
  char out[LX200sbuff];
  double minute;
  if (GetLX200(":Gt#", out, sizeof(out)) == LX200VALUEGET)
  {
    if (dmsToDouble(&degree, out, true, false))
      return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}
LX200RETURN GetLstT0LX200(double &T0)
{
  char out[LX200sbuff];
  if (GetLX200(":GS#", out, sizeof(out)) == LX200VALUEGET)
  {
    if (hmsToDouble(&T0, out))
      return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}
LX200RETURN GetLongitudeLX200(double& degree)
{
  char out[LX200sbuff];
  if (GetLX200(":Gg#", out, sizeof(out)) == LX200VALUEGET)
  {
    double longi = 0;
    if ((out[0] == '-') || (out[0] == '+'))
    {
      if (dmsToDouble(&longi, (char *)&out[1], false, false))
      {
        degree = out[0] == '-' ? -longi : longi;
        return LX200VALUEGET;
      }
    }
  }
  return LX200GETVALUEFAILED;
}

LX200RETURN GetDeclinationLX200(double& degree)
{
  char out[LX200sbuff];
  if (GetLX200(":GD#", out, sizeof(out)) == LX200VALUEGET)
  {
    if (dmsToDouble(&degree, out, true, false))
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

LX200RETURN Move2TargetLX200(TARGETTYPE target)
{
  char out[LX200sbuff];
  int val;

  switch (target)
  {
  case T_RADEC:
    val = readLX200Bytes(":MS#", out, sizeof(out), TIMEOUT_CMD);
    break;
  case T_AZALT:
    val = readLX200Bytes(":MA#", out, sizeof(out), TIMEOUT_CMD);
    break;
  case T_USERRADEC:
    val = readLX200Bytes(":MU#", out, sizeof(out), TIMEOUT_CMD);
  }
  LX200RETURN response;

  switch (out[0] - '0')
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
  case 8:
    response = LX200ABOVEOVERHEAD;
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
  return SetLX200(cmd);
}

LX200RETURN SetTargetAzLX200(uint16_t& v1, uint8_t& v2, uint8_t& v3)
{
  char cmd[LX200sbuff], out[LX200sbuff];
  int iter = 0;
  sprintf(cmd, ":Sz%03u*%02u:%02u#", v1, v2, v3);
  return SetLX200(cmd);
}

LX200RETURN SetTargetDecLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  char cmd[LX200sbuff], out[LX200sbuff];
  int iter = 0;
  sprintf(cmd, ":Sd+%02u:%02u:%02u#", vd1, vd2, vd3);
  if (!ispos)
    cmd[3] = '-';
  return SetLX200(cmd);
}

LX200RETURN SetTargetAltLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  char cmd[LX200sbuff], out[LX200sbuff];
  int iter = 0;
  sprintf(cmd, ":Sa+%02u*%02u'%02u#", vd1, vd2, vd3);
  if (!ispos)
    cmd[3] = '-';
  return SetLX200(cmd);
}

LX200RETURN SyncGotoLX200(bool sync, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3)
{
  if (SetTargetRaLX200(vr1, vr2, vr3) == LX200VALUESET && SetTargetDecLX200(ispos, vd1, vd2, vd3) == LX200VALUESET)
  {
    if (sync)
    {
      char out[LX200sbuff];
      if (GetLX200(":CM#", out, LX200sbuff))
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
      return Move2TargetLX200(T_RADEC);
    }
  }
  else
  {
    return LX200SETTARGETFAILED;
  }
}

LX200RETURN SyncGotoUserLX200(bool sync)
{
  if (sync)
  {
    char out[LX200sbuff];
    if (GetLX200(":CU#", out, LX200sbuff))
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
    return Move2TargetLX200(T_USERRADEC);
  }
}

LX200RETURN SyncGotoLX200AltAz(bool sync, uint16_t& vz1, uint8_t& vz2, uint8_t& vz3, bool& ispos, uint16_t& va1, uint8_t& va2, uint8_t& va3)
{
  if (SetTargetAzLX200(vz1, vz2, vz3) == LX200VALUESET && SetTargetAltLX200(ispos, va1, va2, va3) == LX200VALUESET)
  {
    if (sync)
    {
      char out[LX200sbuff];
      if (GetLX200(":CA#", out, LX200sbuff))
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
      return Move2TargetLX200(T_AZALT);
    }
  }
  else
  {
    return LX200SETTARGETFAILED;
  }
}

LX200RETURN SyncGotoLX200(bool sync, float &Ra, float &Dec)
{
  uint8_t vr1, vr2, vr3, vd2, vd3;
  uint16_t vd1;
  bool ispos = false;
  long Ras = (long)(Ra * 3600);
  long Decs = (long)(Dec * 3600);
  gethms(Ras, vr1, vr2, vr3);
  getdms(Decs, ispos, vd1, vd2, vd3);
  return SyncGotoLX200(sync, vr1, vr2, vr3, ispos, vd1, vd2, vd3);
}


LX200RETURN SyncGotoLX200AltAz(bool sync, float &Az, float &Alt)
{
  uint8_t  vz2, vz3, va2, va3;
  uint16_t vz1, va1;
  bool ispos = false;
  long Azs = (long)(Az * 3600);
  long Altcs = (long)(Alt * 3600);
  getdms(Azs, ispos, vz1, vz2, vz3);
  getdms(Altcs, ispos, va1, va2, va3);
  return SyncGotoLX200AltAz(sync, vz1, vz2, vz3, ispos, va1, va2, va3);
}

LX200RETURN SyncGotoLX200(bool sync, float &Ra, float &Dec, double epoch)
{
  unsigned int day, month, year;
  if (GetUTCDateLX200(day, month, year) != LX200VALUEGET)
    return LX200GETVALUEFAILED;
  EquatorialCoordinates coo;
  coo.ra = Ra;
  coo.dec = Dec;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(coo, epoch, day, month, year, 0, 0, 0);
  return SyncGotoLX200(sync, cooNow.ra, cooNow.dec);
}

LX200RETURN GetLocalDateLX200(unsigned int &day, unsigned int &month, unsigned int &year)
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

LX200RETURN GetUTCDateLX200(unsigned int &day, unsigned int &month, unsigned int &year)
{
  char out[LX200sbuff];
  if (GetLX200(":GXT1#", out, sizeof(out)) == LX200VALUEGET)
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

LX200RETURN GetTrackingRateLX200(double& rate)
{
  char out[LX200sbuff];
  if (GetLX200(":GT#", out, sizeof(out)) == LX200VALUEGET)
  {
    rate = atof(out);
    return LX200VALUEGET;
  }
  return LX200GETVALUEFAILED;
}

LX200RETURN SyncGotoCatLX200(bool sync)
{
  int epoch;
  unsigned int day, month, year;
  if (GetUTCDateLX200(day, month, year) == LX200GETVALUEFAILED) return LX200GETVALUEFAILED;
  if (!cat_mgr.isStarCatalog() && !cat_mgr.isDsoCatalog()) return LX200UNKOWN;
  EquatorialCoordinates coo;
  coo.ra = cat_mgr.rah();
  coo.dec = cat_mgr.dec();
  epoch = cat_mgr.epoch(); if (epoch == 0) return LX200GETVALUEFAILED;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(coo, epoch, day, month, year, 0, 0, 0);
  return SyncGotoLX200(sync, cooNow.ra, cooNow.dec);
}

LX200RETURN SyncGotoPlanetLX200(bool sync, unsigned short objSys)
{
  unsigned int day, month, year, hour, minute, second;
  int minuteLat, minuteLong;
  double  degreeLat, degreeLong;
  if (GetUTCDateLX200(day, month, year) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  if (GetUTCTimeLX200(hour, minute, second) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  if (GetLongitudeLX200(degreeLong) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }
  if (GetLatitudeLX200(degreeLat) == LX200GETVALUEFAILED)
  {
    return LX200GETVALUEFAILED;
  }

  Ephemeris Eph;
  Eph.flipLongitude(true);

  Eph.setLocationOnEarth((float)degreeLat, 0, 0, (float)degreeLong, 0, 0);
  SolarSystemObjectIndex objI = static_cast<SolarSystemObjectIndex>(objSys);
  SolarSystemObject obj = Eph.solarSystemObjectAtDateAndTime(objI, day, month, year, hour, minute, second);
  return SyncGotoLX200(sync, obj.equaCoordinates.ra, obj.equaCoordinates.dec);
}

LX200RETURN SyncSelectedStarLX200(unsigned short alignSelectedStar)
{
  if (alignSelectedStar >= 0) return SyncGotoCatLX200(false); else return LX200UNKOWN;
}

LX200RETURN readReverseLX200(const uint8_t &axis, bool &reverse)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMRR#", out, sizeof(out)) : GetLX200(":GXMRD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    reverse = out[0] == '1' ? true : false;
  }
  return ok;
}

LX200RETURN writeReverseLX200(const uint8_t &axis, const bool &reverse)
{
  char text[LX200sbuff];
  sprintf(text, ":SXMRX,%u#", (unsigned int)reverse);
  text[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(text);
}

LX200RETURN readBacklashLX200(const uint8_t &axis, float &backlash)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMBR#", out, sizeof(out)) : GetLX200(":GXMBD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    backlash = (float)strtol(&out[0], NULL, 10);
  }
  return ok;
}
LX200RETURN writeBacklashLX200(const uint8_t &axis, const float &backlash)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":SXMBX,%u#", (unsigned int)backlash);
  cmd[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readTotGearLX200(const uint8_t &axis, float &totGear)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMGR#", out, sizeof(out)) : GetLX200(":GXMGD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    totGear = (float)strtol(&out[0], NULL, 10);
  }
  return ok;
}
LX200RETURN writeTotGearLX200(const uint8_t &axis, const float &totGear)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":SXMGX,%u#", (unsigned int)totGear);
  cmd[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readStepPerRotLX200(const uint8_t &axis, float &stepPerRot)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMSR#", out, sizeof(out)) : GetLX200(":GXMSD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    stepPerRot = (float)strtol(&out[0], NULL, 10);
  }
  return ok;
}
LX200RETURN writeStepPerRotLX200(const uint8_t &axis, const float &stepPerRot)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":SXMSX,%u#", (unsigned int)stepPerRot);
  cmd[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readMicroLX200(const uint8_t &axis, uint8_t &microStep)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMMR#", out, sizeof(out)) : GetLX200(":GXMMD#", out, sizeof(out));
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
  sprintf(cmd, ":SXMMX,%u#", microStep);
  cmd[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readSilentStepLX200(const uint8_t &axis, uint8_t &silent)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMmR#", out, sizeof(out)) : GetLX200(":GXMmD#", out, sizeof(out));
  if (ok == LX200VALUEGET)
  {
    long value = strtol(&out[0], NULL, 10);

    if ((value >= 0 && value < 2))
    {
      silent = value;
      return ok;
    }
    return LX200GETVALUEFAILED;
  }
  return ok;
}
LX200RETURN writeSilentStepLX200(const uint8_t &axis, const uint8_t &silent)
{
  char cmd[LX200sbuff];
  sprintf(cmd, ":SXMmX,%u#", silent);
  cmd[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readLowCurrLX200(const uint8_t &axis, uint8_t &lowCurr)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMcR#", out, sizeof(out)) : GetLX200(":GXMcD#", out, sizeof(out));
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
  sprintf(cmd, ":SXMcX,%u#", lowCurr);
  cmd[5] = axis == 1 ? 'R' : 'D';
  return SetLX200(cmd);
}
LX200RETURN readHighCurrLX200(const uint8_t &axis, uint8_t &highCurr)
{
  char out[LX200sbuff];
  LX200RETURN ok = axis == 1 ? GetLX200(":GXMCR#", out, sizeof(out)) : GetLX200(":GXMCD#", out, sizeof(out));
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
  sprintf(cmd, ":SXMCX,%u#", highCurr);
  cmd[5] = axis == 1 ? 'R' : 'D';
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

LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr, unsigned int& curr, unsigned int& steprot)
{
  char out[LX200lbuff];
  if (GetLX200(":FM#", out, sizeof(out)) == LX200GETVALUEFAILED)
    if (GetLX200(":FM#", out, sizeof(out)) == LX200GETVALUEFAILED)
      if (GetLX200(":FM#", out, sizeof(out)) == LX200GETVALUEFAILED)
        return LX200GETVALUEFAILED;
  if (strlen(out) != 16)
    return LX200GETVALUEFAILED;
  char* pEnd;
  reverse = out[1] == '1';
  micro = out[3] - '0';
  incr = strtol(&out[5], &pEnd, 10);
  curr = strtol(&out[9], &pEnd, 10);
  steprot = strtol(&out[13], &pEnd, 10);
  return LX200VALUEGET;
}


// convert string in format HH:MM:SS to floating point
// (also handles)           HH:MM.M
boolean hmsToDouble(double *f, char *hms)
{
  char h[3], m[5], s[3];
  int  h1, m1, m2 = 0, s1 = 0;
  boolean highPrecision = true;

  while (*hms == ' ') hms++; // strip prefix white-space

  if (highPrecision)
  {
    if (strlen(hms) != 8) return false;
  }
  else if (strlen(hms) != 7) return false;

  h[0] = *hms++; h[1] = *hms++; h[2] = 0; atoi2(h, &h1);
  if (highPrecision)
  {
    if (*hms++ != ':') return false; m[0] = *hms++; m[1] = *hms++; m[2] = 0; atoi2(m, &m1);
    if (*hms++ != ':') return false; s[0] = *hms++; s[1] = *hms++; s[2] = 0; atoi2(s, &s1);
  }
  else
  {
    if (*hms++ != ':') return false; m[0] = *hms++; m[1] = *hms++; m[2] = 0; atoi2(m, &m1);
    if (*hms++ != '.') return false; m2 = (*hms++) - '0';
  }
  if ((h1 < 0) || (h1 > 23) || (m1 < 0) || (m1 > 59) || (m2 < 0) || (m2 > 9) || (s1 < 0) || (s1 > 59)) return false;

  *f = h1 + m1 / 60.0 + m2 / 600.0 + s1 / 3600.0;
  return true;
}

// convert string in format sDD:MM:SS to floating point
// (also handles)           DDD:MM:SS
//                          sDD:MM
//                          DDD:MM
//                          sDD*MM
//                          DDD*MM
boolean dmsToDouble(double *f, char *dms, boolean sign_present, boolean highPrecision)
{
  char d[4], m[5], s[3];
  int d1, m1, s1 = 0;
  int lowLimit = 0, highLimit = 360;
  int checkLen, checkLen1;
  double sign = 1.0;
  boolean secondsOff = false;

  while (*dms == ' ') dms++; // strip prefix white-space

  checkLen1 = strlen(dms);

  // determine if the seconds field was used and accept it if so
  if (highPrecision)
  {
    checkLen = 9;
    if (checkLen1 != checkLen) return false;
  }
  else
  {
    checkLen = 6;
    if (checkLen1 != checkLen)
    {
      if (checkLen1 == 9)
      {
        secondsOff = false; checkLen = 9;
      }
      else return false;
    }
    else secondsOff = true;
  }

  // determine if the sign was used and accept it if so
  if (sign_present)
  {
    if (*dms == '-') sign = -1.0; else if (*dms == '+') sign = 1.0; else return false;
    dms++; d[0] = *dms++; d[1] = *dms++; d[2] = 0; if (!atoi2(d, &d1)) return false;
  }
  else
  {
    d[0] = *dms++; d[1] = *dms++; d[2] = *dms++; d[3] = 0; if (!atoi2(d, &d1)) return false;
  }

  // make sure the seperator is an allowed character
  if ((*dms != ':') && (*dms != '*') && (*dms != char(223))) return false; else dms++;

  m[0] = *dms++; m[1] = *dms++; m[2] = 0; if (!atoi2(m, &m1)) return false;

  if ((highPrecision) && (!secondsOff))
  {
    // make sure the seperator is an allowed character
    if (*dms++ != ':') return false;
    s[0] = *dms++; s[1] = *dms++; s[2] = 0; atoi2(s, &s1);
  }

  if (sign_present)
  {
    lowLimit = -90; highLimit = 90;
  }
  if ((d1 < lowLimit) || (d1 > highLimit) || (m1 < 0) || (m1 > 59) || (s1 < 0) || (s1 > 59)) return false;

  *f = sign * (d1 + m1 / 60.0 + s1 / 3600.0);
  return true;
}