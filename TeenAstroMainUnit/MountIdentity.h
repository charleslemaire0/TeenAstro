#pragma once
#include "MountTypes.h"
#include "Config.TeenAstro.h"
#include "EEPROM_address.h"

struct MountIdentity {
  bool DecayModeTrack;
  MeridianFlip meridianFlip;
  MountType mountType;
  char mountName[maxNumMount][MountNameLen];
  bool isMountTypeFix;
};
