#pragma once
//#include "Global.h"

void UpdateGnss();
bool iSGNSSValid(void);
bool GNSSTimeIsValid();
bool GNSSLocationIsValid();
bool isHdopSmall();
bool isTimeSyncWithGNSS();
bool isLocationSyncWithGNSS();
void Command_GNSS();
