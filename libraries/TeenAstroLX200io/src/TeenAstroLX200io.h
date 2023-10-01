#pragma once
#include<Arduino.h>
#include<TeenAstroCatalog.h>


#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#define Ser Serial
#endif
#ifdef ARDUINO_LOLIN_C3_MINI
#define Ser Serial1
#endif


#define TIMEOUT_CMD 30         
#define TIMEOUT_WEB 15

enum LX200RETURN
{
  LX200_NOTOK,
  LX200_SETVALUEFAILED, LX200_GETVALUEFAILED, LX200_SYNCFAILED,
  LX200_SETTARGETFAILED, LX200_TARGETBELOWHORIZON, LX200_TARGETABOVEOVERHEAD,
  LX200_GOHOME_FAILED, LX200_GOPARK_FAILED,
  LX200_FLIPSAMESIDE,
  LX200_ERR_MOTOR_FAULT, LX200_ERR_ALT, LX200_ERR_AXIS1, LX200_ERR_AXIS2, LX200_ERR_LIMIT_SENSE, LX200_ERR_UNDER_POLE,LX200_ERR_MERIDIAN,
  LX200_ERRGOTO_NOOBJECTSELECTED, LX200_ERRGOTO_PARKED, LX200_ERRGOTO_BUSY, LX200_ERRGOTO_LIMITS, LX200_ERRGOTO_UNKOWN,
  LX200_OK,
  LX200_VALUESET, LX200_VALUEGET, LX200_SYNCED, LX200_GOTO_TARGET, LX200_GOHOME, LX200_GOPARK,
  LX200_PUSHTO_TARGET
};

enum ErrorsGoTo
{
  ERRGOTO_NONE,
  ERRGOTO_BELOWHORIZON,
  ERRGOTO_NOOBJECTSELECTED,
  ERRGOTO_SAMESIDE,
  ERRGOTO_PARKED,
  ERRGOTO_SLEWING,
  ERRGOTO_LIMITS,
  ERRGOTO_GUIDING,
  ERRGOTO_ABOVEOVERHEAD,
  ERRGOTO_MOTOR,
  ERRGOTO____,
  ERRGOTO_MOTOR_FAULT,
  ERRGOTO_ALT,
  ERRGOTO_LIMIT_SENSE,
  ERRGOTO_AXIS1,
  ERRGOTO_AXIS2,
  ERRGOTO_UNDER_POLE,
  ERRGOTO_MERIDIAN
};
enum TARGETTYPE
{
  T_AZALT, T_RADEC, T_USERRADEC
};

enum NAV { NAV_SYNC, NAV_GOTO, NAV_PUSHTO };

bool isOk(LX200RETURN val);
bool readLX200Bytes(char* command, char* recvBuffer, int bufferSize, unsigned long timeOutMs, bool keepHashtag = false);
LX200RETURN GetLX200(char* command, char* output, int buffersize);
LX200RETURN GetLX200Short(char* command, short* value);
LX200RETURN GetLX200Float(char* command, float* value);
LX200RETURN GetLocalTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetLocalTimeLX200(long &value);
LX200RETURN GetUTCTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetUTCTimeLX200(long &value);
LX200RETURN SetLX200(char* command);
LX200RETURN SetBoolLX200(char* command);
LX200RETURN SetLocalTimeLX200(long &value);
LX200RETURN SetUTCTimeLX200(long & value);
LX200RETURN GetMountIdxLX200(int& value);
LX200RETURN GetMountNameLX200(int idx, char* name, int len);
LX200RETURN GetSiteLX200(int &value);
LX200RETURN GetSiteNameLX200(int idx, char* name, int len);
LX200RETURN GetLstT0LX200(double &T0);
LX200RETURN GetLatitudeLX200(double& degree);
LX200RETURN GetLongitudeLX200(double& degree);
LX200RETURN GetTrackingRateLX200(double& rate);
LX200RETURN SetSiteLX200(int &value);
LX200RETURN SetMountLX200(int& value);
LX200RETURN SetMountNameLX200(int idx, char* name);
LX200RETURN Move2TargetLX200(TARGETTYPE target);
LX200RETURN Push2TargetLX200(TARGETTYPE target);
LX200RETURN SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3);
LX200RETURN SetTargetDecLX200(bool& ispos, uint8_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SetTargetAzLX200(uint16_t& v1, uint8_t& v2, uint8_t& v3);
LX200RETURN SetTargetAltLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGoHomeLX200(NAV mode);
LX200RETURN SyncGoParkLX200(NAV mode);
LX200RETURN SyncGotoLX200(NAV mode, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGotoLX200(NAV mode, float &Ra, float &Dec);
LX200RETURN SyncGotoLX200AltAz(NAV mode, float &Az, float &Alt);
LX200RETURN SyncGotoUserLX200(NAV mode);
LX200RETURN SyncGotoLX200(NAV mode, float &Ra, float &Dec, double epoch);
LX200RETURN SyncSelectedStarLX200(unsigned short alignSelectedStar);
LX200RETURN GetLocalDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
LX200RETURN GetUTCDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
LX200RETURN SyncGotoCatLX200(NAV mode);
LX200RETURN SyncGotoPlanetLX200(NAV mode, unsigned short obj);
//motor
LX200RETURN readReverseLX200(const uint8_t &axis, bool &reverse);
LX200RETURN writeReverseLX200(const uint8_t &axis, const bool &reverse);
LX200RETURN readBacklashLX200(const uint8_t &axis, float &backlash);
LX200RETURN writeBacklashLX200(const uint8_t &axis, const float &backlash);
LX200RETURN readBacklashRateLX200(const uint8_t& axis, float& backlashRate);
LX200RETURN writeBacklashRateLX200(const uint8_t& axis, const float& backlashRate);
LX200RETURN readTotGearLX200(const uint8_t &axis, float &totGear);
LX200RETURN writeTotGearLX200(const uint8_t &axis, const float &totGear);
LX200RETURN readStepPerRotLX200(const uint8_t &axis, float &stepPerRot);
LX200RETURN writeStepPerRotLX200(const uint8_t &axis, const float &stepPerRot);
LX200RETURN readMicroLX200(const uint8_t &axis, uint8_t &microStep);
LX200RETURN writeMicroLX200(const uint8_t &axis, const uint8_t &microStep);
LX200RETURN readSilentStepLX200(const uint8_t &axis, uint8_t &cool);
LX200RETURN writeSilentStepLX200(const uint8_t &axis, const uint8_t &cool);
LX200RETURN readLowCurrLX200(const uint8_t &axis, unsigned int &lowCurr);
LX200RETURN writeLowCurrLX200(const uint8_t &axis, const unsigned int &lowCurr);
LX200RETURN readHighCurrLX200(const uint8_t &axis, unsigned int &highCurr);
LX200RETURN writeHighCurrLX200(const uint8_t &axis, const unsigned int &highCurr);
//encoder
LX200RETURN readEncoderReverseLX200(const uint8_t& axis, bool& reverse);
LX200RETURN writeEncoderReverseLX200(const uint8_t& axis, const bool& reverse);
LX200RETURN readPulsePerDegreeLX200(const uint8_t& axis, float& ppd);
LX200RETURN writePulsePerDegreeLX200(const uint8_t& axis, const float& ppd);
LX200RETURN StartEncoderCalibration();
LX200RETURN CancelEncoderCalibration();
LX200RETURN CompleteEncoderCalibration();
LX200RETURN readEncoderAutoSync(uint8_t& syncmode);
LX200RETURN writeEncoderAutoSync(const uint8_t syncmode);

//focuser
LX200RETURN readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                              unsigned int& minSpeed, unsigned int& maxSpeed,
                              unsigned int& cmdAcc, unsigned int& manAcc, unsigned int& manDec);
LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr, unsigned int& curr, unsigned int& steprot);
boolean hmsToDouble(double *f, char *hms);
boolean dmsToDouble(double *f, char *dms, boolean sign_present, boolean highPrecision);