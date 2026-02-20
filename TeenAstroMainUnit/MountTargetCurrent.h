#pragma once
#include "MountTypes.h"

struct MountTargetCurrent {
  PoleSide newTargetPoleSide;
  double newTargetAlt;
  double newTargetAzm;
  double newTargetDec;
  double newTargetRA;
  double currentAzm;
  double currentAlt;
};
