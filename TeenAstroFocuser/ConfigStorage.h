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
  uint8_t curr;
  uint8_t micro;
	bool reverse;
	unsigned long startPosition;
	unsigned long maxPosition;
  unsigned int lowSpeed;
  unsigned int highSpeed;
  uint8_t cmdAcc;
  uint8_t manAcc;
  uint8_t manDec;
  unsigned int resolution;
}
storage = {
	CONFIG_VERSION,
	false,
	50,
  4,
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

