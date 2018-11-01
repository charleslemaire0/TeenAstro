// ConfigStorage.h

#ifndef _CONFIGSTORAGE_h
#define _CONFIGSTORAGE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// ID of the settings block
#define CONFIG_VERSION "f00"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 32

struct StoreStruct
{
	char version[4];
	bool reverse;
	unsigned long startPosition;
	unsigned long maxPosition;
  unsigned int minSpeed;
  unsigned int maxSpeed;
  uint8_t cmdAcc;
  uint8_t manAcc;
  uint8_t manDec;
  unsigned int inpulse;
}
storage = {
	CONFIG_VERSION,
	false,
	0,
	32000,
	10,
	60,
	10,
	10,
	10,
  16
};


void loadConfig();
void saveConfig();

#endif

