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
  enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
  enum TrackState { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
  enum ParkState { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
  enum PierState { PIER_E, PIER_W, PIER_UNKNOW };

public: 
  AlignState      align = ALI_OFF;
  AlignMode       aliMode = ALIM_ONE;
  unsigned short  alignSelectedStar = 1;
  int             alignMaxNumStars = -1;

  Errors lastError = Telescope::ERR_NONE;

  char TempRa[15];
  char TempDec[15];
  unsigned long lastStateRaDec;
  char TempAz[15];
  char TempAlt[15];
  unsigned long lastStateAzAlt;
  char TempUTC[15];
  char TempSideral[15];
  unsigned long lastStateTime;
  char TelStatus[16];
  unsigned long lastStateFocuser;
  char TempFocuserStatus[45];

  unsigned long lastStateTel;
public:
  int connectionFailure = 0;
  bool hasInfoRa = false;
  bool hasInfoDec = false;
  bool hasInfoAz = false;
  bool hasInfoAlt = false;
  bool hasInfoUTC = false;
  bool hasInfoSideral = false;
  bool hasTelStatus = false;
  bool hasInfoFocuser = false;
  unsigned long lastState;
  void updateRaDec();
  void updateAzAlt();
  void updateTime();
  void updateFocuser();
  void updateTel();
  Mount getMount();
  ParkState getParkState();
  TrackState getTrackingState();
  bool atHome();
  bool isPulseGuiding();
  bool isGuidingN();
  bool isGuidingS();
  bool isGuidingE();
  bool isGuidingW();
  bool connected();
  bool notResponding();
  PierState getPierState();
  Errors getError();
  void addStar();

private:

};

