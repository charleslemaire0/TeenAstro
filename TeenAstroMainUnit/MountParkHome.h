#pragma once
#include "MountTypes.h"

struct MountParkHome {
  ParkState parkStatus;
  bool parkSaved;
  bool homeSaved;
  bool atHome;
  bool homeMount;
  unsigned int slewSettleDuration;
  unsigned long lastSettleTime;
  bool settling;
  BacklashPhase backlashStatus;
};
