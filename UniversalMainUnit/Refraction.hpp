#pragma once
#include "EEPROM_address.h"
#include "XEEPROM.hpp"
class RefractionFlags
{
public:
  bool forPole;
  bool forGoto;
  bool forTracking;
  bool setPole(bool value)
  {
    if (forPole != value)
    {
      forPole = value;
      writeToEEPROM();
      return true;
    }
    return false;
  }
  bool setGoto(bool value)
  {
    if (forGoto != value)
    {
      forGoto = value;
      writeToEEPROM();
      return true;
    }
    return false;
  }
  bool setTracking(bool value)
  {
    if (forTracking != value)
    {
      forTracking = value;
      writeToEEPROM();
      return true;
    }
    return false;
  }
  void readFromEEPROM()
  {
    uint8_t val = XEEPROM.read(getMountAddress(EE_refraction));
    forPole = bitRead(val, 0);
    forGoto = bitRead(val, 1);
    forTracking = bitRead(val, 2);
  }
  void writeToEEPROM()
  {
    uint8_t val = 0;
    bitWrite(val, 0, forPole);
    bitWrite(val, 1, forGoto);
    bitWrite(val, 2, forTracking);
    XEEPROM.write(getMountAddress(EE_refraction), val);
  }
  void writeDefaultToEEPROM()
  {
    forPole = false;
    forGoto = false;
    forTracking = false;
    writeToEEPROM();
  }
};
