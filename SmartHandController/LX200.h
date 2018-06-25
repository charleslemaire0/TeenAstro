#pragma once
#include "Catalog.h"
#include "Config.h"

enum LX200RETURN {
  LX200NOTOK, LX200SETVALUEFAILED, LX200GETVALUEFAILED, LX200SYNCFAILED, LX200SETTARGETFAILED, LX200BELOWHORIZON,
  LX200GOHOME_FAILED, LX200GOPARK_FAILED,
  LX200_ERR_MOTOR_FAULT, LX200_ERR_ALT, LX200_ERR_LIMIT_SENSE, LX200_ERR_DEC, LX200_ERR_AZM,  LX200_ERR_UNDER_POLE, LX200_ERR_MERIDIAN, LX200_ERR_SYNC, LX200NOOBJECTSELECTED, LX200PARKED, LX200BUSY, LX200LIMITS, LX200UNKOWN,
  LX200OK, LX200VALUESET, LX200VALUEGET, LX200SYNCED, LX200GOINGTO, LX200GOHOME, LX200GOPARK
};

bool isOk(LX200RETURN val);
LX200RETURN GetLX200(char* command, char* output);
LX200RETURN GetTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
LX200RETURN GetTimeLX200(long &value);
LX200RETURN SetLX200(char* command);
LX200RETURN SetTimeLX200(long &value);
LX200RETURN GetSiteLX200(int &value);
LX200RETURN GetLatitudeLX200(int& degree, int& minute);
LX200RETURN GetLongitudeLX200(int& degree, int& minute);
void SetSiteLX200(int &value);
LX200RETURN Move2TargetLX200();
LX200RETURN SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3);
LX200RETURN SetTargetDecLX200(short& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGoHomeLX200(bool sync);
LX200RETURN SyncGoParkLX200(bool sync);
LX200RETURN SyncGotoLX200(bool sync, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, short& vd1, uint8_t& vd2, uint8_t& vd3);
LX200RETURN SyncGotoLX200(bool, float &Ra, float &Dec);
LX200RETURN SyncSelectedStarLX200(unsigned short alignSelectedStar);
LX200RETURN GetDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
LX200RETURN SyncGotoCatLX200(bool sync, Catalog cat, int idx);
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
LX200RETURN writeMicroLX200(const uint8_t &axis, const uint8_t &microStep) ;
LX200RETURN readLowCurrLX200(const uint8_t &axis, uint8_t &lowCurr);
LX200RETURN writeLowCurrLX200(const uint8_t &axis, const uint8_t &lowCurr);
LX200RETURN readHighCurrLX200(const uint8_t &axis, uint8_t &highCurr);
LX200RETURN writeHighCurrLX200(const uint8_t &axis, const uint8_t &highCurr);
