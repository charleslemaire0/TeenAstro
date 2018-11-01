// 
// 
// 
#include <EEPROM.h>
#include "ConfigStorage.h"


void loadConfig() {
	// To make sure there are settings, and they are YOURS!
	// If nothing is found it will use the default settings.
	if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
		EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
		EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2])
		for (unsigned int t = 0; t<sizeof(storage); t++)
			*((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
  checkvalue();
}

void checkvalue()
{
  unsigned int vali;
  unsigned long vall;
  uint8_t vals;
  bool dosave = false;
  vali = max(min(storage.inpulse, 512u), 1u);
  dosave = dosave || vali != storage.inpulse;
  storage.inpulse = vali;
  vall = max(min(storage.startPosition, 65535uL * storage.inpulse), 1uL);
  dosave = dosave || vall != storage.startPosition;
  storage.startPosition = vall;
  vall = max(min(storage.maxPosition, 65535uL * storage.inpulse), 1uL);
  dosave = dosave || vall != storage.maxPosition;
  storage.maxPosition = vall;
  vali = max(min(storage.maxSpeed, 999u), 1u);
  dosave = dosave || vali != storage.maxSpeed;
  storage.maxSpeed = vali;
  vali = max(min(storage.minSpeed, storage.maxSpeed), 1u);
  dosave = dosave || vali != storage.minSpeed;
  storage.minSpeed = vali;
  vals = max(min(storage.cmdAcc, 99u), 1u);
  dosave = dosave || vals != storage.cmdAcc;
  storage.cmdAcc = vals;
  vals = dosave || max(min(storage.manDec, 99u), 1u);
  dosave = dosave || vals != storage.manDec;
  storage.manDec = vals;
  vals = max(min(storage.manAcc, 99u), 1u);
  dosave = dosave || vals != storage.manAcc;
  storage.manAcc = vals;
  if (dosave)
    saveConfig();
}

void saveConfig() {
	for (unsigned int t = 0; t<sizeof(storage); t++)
		EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
}