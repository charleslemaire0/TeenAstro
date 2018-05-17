#pragma once
#include "Catalog.h"
#include "config.h"

bool GetLX200(char* command, char* output, bool silent);
bool GetTimeLX200(unsigned int &hour, unsigned int &minute, unsigned int &second);
bool GetTimeLX200(long &value);
bool SetLX200(char* command, bool silent = false);
bool SetTimeLX200(long &value);
bool GetSiteLX200(int &value);
void SetSiteLX200(int &value);
void Move2TargetLX200(bool silent = false);
bool SetTargetRaLX200(uint8_t& vr1, uint8_t& vr2, uint8_t& vr3);
bool SetTargetDecLX200(short& vd1, uint8_t& vd2, uint8_t& vd3);
bool SyncGotoLX200(bool sync, uint8_t& vr1, uint8_t& vr2, uint8_t& vr3, short& vd1, uint8_t& vd2, uint8_t& vd3);
bool SyncGotoLX200(bool, float &Ra, float &Dec);
bool SyncSelectedStarLX200(unsigned short alignSelectedStar);
bool GetDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
bool SyncGotoCatLX200(bool sync, Catalog cat, int idx);
bool SyncGotoPlanetLX200(bool sync, unsigned short obj);
bool readReverseLX200(const uint8_t &axis, bool &reverse);
bool writeReverseLX200(const uint8_t &axis, const bool &reverse);
bool readBacklashLX200(const uint8_t &axis, float &backlash);
bool writeBacklashLX200(const uint8_t &axis, const float &backlash);
bool readTotGearLX200(const uint8_t &axis, float &totGear);
bool writeTotGearLX200(const uint8_t &axis, const float &totGear);
bool readStepPerRotLX200(const uint8_t &axis, float &stepPerRot);
bool writeStepPerRotLX200(const uint8_t &axis, const float &stepPerRot);
bool readMicroLX200(const uint8_t &axis, uint8_t &microStep);
bool writeMicroLX200(const uint8_t &axis, const uint8_t &microStep) ;
bool readLowCurrLX200(const uint8_t &axis, uint8_t &lowCurr);
bool writeLowCurrLX200(const uint8_t &axis, const uint8_t &lowCurr);
bool readHighCurrLX200(const uint8_t &axis, uint8_t &highCurr);
bool writeHighCurrLX200(const uint8_t &axis, const uint8_t &highCurr);