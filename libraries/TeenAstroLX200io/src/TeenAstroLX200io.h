#pragma once
#include<Arduino.h>
#include<TeenAstroCatalog.h>
#define Ser Serial
#define TIMEOUT_CMD 30         
#define TIMEOUT_WEB 15

enum LX200RETURN
{
  LX200NOTOK, LX200SETVALUEFAILED, LX200GETVALUEFAILED, LX200SYNCFAILED, LX200SETTARGETFAILED, LX200BELOWHORIZON, LX200ABOVEOVERHEAD,
  LX200GOHOME_FAILED, LX200GOPARK_FAILED,
  LX200_ERR_MOTOR_FAULT, LX200_ERR_ALT, LX200_ERR_LIMIT_SENSE, LX200_ERR_DEC, LX200_ERR_AZM, LX200_ERR_UNDER_POLE, LX200_ERR_MERIDIAN, LX200_ERR_SYNC, LX200NOOBJECTSELECTED, LX200PARKED, LX200BUSY, LX200LIMITS, LX200UNKOWN,
  LX200OK, LX200VALUESET, LX200VALUEGET, LX200SYNCED, LX200GOINGTO, LX200GOHOME, LX200GOPARK
};
enum TARGETTYPE
{
  T_AZALT, T_RADEC, T_USERRADEC
};

bool isOk(LX200RETURN val);
bool readLX200Bytes(char* command, char* recvBuffer, int bufferSize, unsigned long timeOutMs, bool keepHashtag = false);
LX200RETURN GetLX200(char* command, char* output, int buffersize);
LX200RETURN GetLX200Float(char* command, float* value);
LX200RETURN GetLocalTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetLocalTimeLX200(long &value);
LX200RETURN GetUTCTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetUTCTimeLX200(long &value);
LX200RETURN SetLX200(char* command);
LX200RETURN SetBoolLX200(char* command);
LX200RETURN SetLocalTimeLX200(long &value);
LX200RETURN SetUTCTimeLX200(long & value);
LX200RETURN GetSiteLX200(int &value);
LX200RETURN GetLstT0LX200(double &T0);
LX200RETURN GetLatitudeLX200(double& degree);
LX200RETURN GetLongitudeLX200(double& degree);
LX200RETURN GetTrackingRateLX200(double& rate);
void SetSiteLX200(int &value);
LX200RETURN Move2TargetLX200(TARGETTYPE target);
LX200RETURN SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3);
LX200RETURN SetTargetDecLX200(bool& ispos, uint8_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SetTargetAzLX200(uint16_t& v1, uint8_t& v2, uint8_t& v3);
LX200RETURN SetTargetAltLX200(bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGoHomeLX200(bool sync);
LX200RETURN SyncGoParkLX200(bool sync);
LX200RETURN SyncGotoLX200(bool sync, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, bool& ispos, uint16_t& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGotoLX200(bool sync, float &Ra, float &Dec);
LX200RETURN SyncGotoLX200AltAz(bool sync, float &Az, float &Alt);
LX200RETURN SyncGotoUserLX200(bool sync);
LX200RETURN SyncGotoLX200(bool sync, float &Ra, float &Dec, double epoch);
LX200RETURN SyncSelectedStarLX200(unsigned short alignSelectedStar);
LX200RETURN GetLocalDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
LX200RETURN GetUTCDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
LX200RETURN SyncGotoCatLX200(bool sync);
LX200RETURN SyncGotoPlanetLX200(bool sync, unsigned short obj);
LX200RETURN readReverseLX200(const uint8_t &axis, bool &reverse);
LX200RETURN writeReverseLX200(const uint8_t &axis, const bool &reverse);
LX200RETURN readBacklashLX200(const uint8_t &axis, float &backlash);
LX200RETURN writeBacklashLX200(const uint8_t &axis, const float &backlash);
LX200RETURN readTotGearLX200(const uint8_t &axis, float &totGear);
LX200RETURN writeTotGearLX200(const uint8_t &axis, const float &totGear);
LX200RETURN readStepPerRotLX200(const uint8_t &axis, float &stepPerRot);
LX200RETURN writeStepPerRotLX200(const uint8_t &axis, const float &stepPerRot);
LX200RETURN readMicroLX200(const uint8_t &axis, uint8_t &microStep);
LX200RETURN writeMicroLX200(const uint8_t &axis, const uint8_t &microStep);
LX200RETURN readSilentStepLX200(const uint8_t &axis, uint8_t &cool);
LX200RETURN writeSilentStepLX200(const uint8_t &axis, const uint8_t &cool);
LX200RETURN readLowCurrLX200(const uint8_t &axis, uint8_t &lowCurr);
LX200RETURN writeLowCurrLX200(const uint8_t &axis, const uint8_t &lowCurr);
LX200RETURN readHighCurrLX200(const uint8_t &axis, uint8_t &highCurr);
LX200RETURN writeHighCurrLX200(const uint8_t &axis, const uint8_t &highCurr);
LX200RETURN readFocuserConfig(unsigned int& startPosition, unsigned int& maxPosition,
                              unsigned int& minSpeed, unsigned int& maxSpeed,
                              unsigned int& cmdAcc, unsigned int& manAcc, unsigned int& manDec);
LX200RETURN readFocuserMotor(bool& reverse, unsigned int& micro, unsigned int& incr, unsigned int& curr, unsigned int& steprot);
boolean hmsToDouble(double *f, char *hms);
boolean dmsToDouble(double *f, char *dms, boolean sign_present, boolean highPrecision);