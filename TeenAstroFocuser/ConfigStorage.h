// ConfigStorage.h

#ifndef _CONFIGSTORAGE_h
#define _CONFIGSTORAGE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

// ID of the settings block
#define CONFIG_VERSION "f00"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 0

class Parameteruint8_t
{
private:
  int m_adress;
  uint8_t m_value;
  uint8_t m_value_default;
  uint8_t m_value_min;
  uint8_t m_value_max;
public:
  uint8_t get();
  void load();
  bool set(const uint8_t& val);
  uint8_t getmax();
  uint8_t getmin();
  Parameteruint8_t(const int &adress, const uint8_t &value_default, const  uint8_t &value_min, const  uint8_t &value_max);
};

class Parameterulong
{
private:
  int m_adress;
  unsigned long m_value;
  unsigned long m_value_default;
  unsigned long m_value_min;
  unsigned long m_value_max;
public:
  unsigned long get();
  void load();
  bool set(const unsigned long& val);
  unsigned long getmax();
  unsigned long getmin();
  Parameterulong(const int &adress, const  unsigned long &valuedefault, const unsigned long &value_min, const unsigned long &value_max);
};

class ParameterPosition
{
private:
  int m_adress;
  Parameterulong *m_pos;
  char m_id[11] = { 0 };
public:
  bool isvalid();
  unsigned long getPosition();
  void get(char* id, unsigned long& val);
  bool set(char* id, const unsigned long& val);
  ParameterPosition(const int &adress);
};

class Parameteruint
{
private:
  int m_adress;
  unsigned int m_value;
  unsigned int m_value_default;
  unsigned int m_value_min;
  unsigned int m_value_max;
public:
  unsigned int get();
  void load();
  bool set(const unsigned int& val);
  unsigned int getmax();
  unsigned int getmin();
  Parameteruint(const int &adress, const  unsigned int &valuedefault, const unsigned int &value_min, const unsigned int &value_max);
};
Parameteruint8_t *curr;
Parameteruint *steprot;
Parameteruint8_t *micro;
Parameteruint8_t *reverse;
Parameterulong *startPosition;
Parameterulong *maxPosition;
Parameteruint *lowSpeed;
Parameteruint *highSpeed;
Parameteruint8_t *cmdAcc;
Parameteruint8_t *manAcc;
Parameteruint8_t *manDec;
Parameteruint *resolution;
ParameterPosition *PositionList[10];
void loadConfig();
#endif

