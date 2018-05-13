#pragma once
#include <Arduino.h>
#include "Catalog.h"

class Telescope
{
public:
  enum Errors { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE, ERR_DEC, ERR_AZM, ERR_UNDER_POLE, ERR_MERIDIAN, ERR_SYNC };

  enum AlignMode { ALIM_ONE, ALIM_TWO, ALIM_THREE };
  enum AlignState {
    ALI_OFF,
    ALI_SELECT_STAR_1, ALI_SLEW_STAR_1, ALI_RECENTER_1,
    ALI_SELECT_STAR_2, ALI_SLEW_STAR_2, ALI_RECENTER_2,
    ALI_SELECT_STAR_3, ALI_SLEW_STAR_3, ALI_RECENTER_3
  };
  enum Mount { GEM, FEM };
  enum TrackState { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
  enum ParkState { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
  enum PierState { PIER_E, PIER_W, PIER_UNKNOW };

  Mount           mountType = GEM;

  AlignState      align = ALI_OFF;
  AlignMode       aliMode = ALIM_ONE;
  unsigned short  alignSelectedStar = 1;
  int             alignMaxNumStars = -1;

  Errors lastError = Telescope::ERR_NONE;


  char TempRa[20];
  char TempDec[20];
  unsigned long lastStateRaDec;
  char TempAz[20];
  char TempAlt[20];
  unsigned long lastStateAzAlt;
  char TempUTC[20];
  char TempSideral[20];
  unsigned long lastStateTime;
  char TelStatus[20];
  char sideofpier[20];
  unsigned long lastStateTel;
public:
  bool connected = true;
  bool hasInfoRa = false;
  bool hasInfoDec = false;
  bool hasInfoAz = false;
  bool hasInfoAlt = false;
  bool hasInfoUTC = false;
  bool hasInfoSideral = false;
  bool hasPierInfo = false;
  bool hasTelStatus = false;
  unsigned long lastState;
  void updateRaDec();
  void updateAzAlt();

  void updateTime();

  void updateTel();

  ParkState getParkState();

  TrackState getTrackingState();
 
  bool atHome();

  bool isGuiding();

  PierState getPierState();
 
  Errors getError();

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
  bool SyncGotoLX200(bool, float& Ra, float& Dec);
  bool SyncSelectedStarLX200();
  bool GetDateLX200(unsigned int &day, unsigned int &month, unsigned int &year);
  bool SyncGotoCatLX200(bool sync, Catalog cat, int idx);
  bool SyncGotoPlanetLX200(bool sync, unsigned short obj);
  void addStar();
  int readLX200Bytes(char* command, char* recvBuffer, unsigned long timeOutMs);
private:
  void serialRecvFlush();
  void char2RA(char* txt, unsigned int& hour, unsigned int& minute, unsigned int& second);
  void char2DEC(char* txt, int& deg, unsigned int& min, unsigned int& sec);
};

