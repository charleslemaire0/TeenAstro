#pragma once
#include "EEPROM.h"
// -----------------------------------------------------------------------------------
// EEPROM related functions

// write int numbers into EEPROM at position i (2 bytes)

struct extendedEEPROM : EEPROMClass
{
  void writeUShort(int i, ushort j)
  {
    uint8_t *k = (uint8_t *)&j;
    EEPROM.update(i + 0, *k);
    k++;
    EEPROM.update(i + 1, *k);
  }

  // read int numbers from EEPROM at position i (2 bytes)
  ushort readUShort(int i)
  {
    uint16_t    j;
    uint8_t     *k = (uint8_t *)&j;
    *k = EEPROM.read(i + 0);
    k++;
    *k = EEPROM.read(i + 1);
    return j;
  }

  void writeShort(int i, short j)
  {
    uint8_t* k = (uint8_t*)&j;
    EEPROM.update(i + 0, *k);
    k++;
    EEPROM.update(i + 1, *k);
  }

  // read int numbers from EEPROM at position i (2 bytes)
  short readShort(int i)
  {
    int16_t   j;
    uint8_t* k = (uint8_t*)&j;
    *k = EEPROM.read(i + 0);
    k++;
    *k = EEPROM.read(i + 1);
    return j;
  }

  // write 4 byte variable into EEPROM at position i (4 bytes)
  void writeQuad(int i, byte *v)
  {
    EEPROM.update(i + 0, *v);
    v++;
    EEPROM.update(i + 1, *v);
    v++;
    EEPROM.update(i + 2, *v);
    v++;
    EEPROM.update(i + 3, *v);
  }

  // read 4 byte variable from EEPROM at position i (4 bytes)
  void readQuad(int i, byte *v)
  {
    *v = EEPROM.read(i + 0);
    v++;
    *v = EEPROM.read(i + 1);
    v++;
    *v = EEPROM.read(i + 2);
    v++;
    *v = EEPROM.read(i + 3);
  }

  // write String into EEPROM at position i
  bool writeString(int i, char l[], int E_buffersize)
  {
    if ((int)strlen(l) + 1 > E_buffersize)
      return false;
    for (int l1 = 0; l1 < (int)strlen(l) + 1; l1++)
    {
      EEPROM.update(i + l1, l[l1]);
    }
    return true;
  }

  // read String from EEPROM at position i
  // the String must be stored with as null-terminated string!
  bool readString(int i, char l[], int E_buffersize)
  { 
    bool validend = false;
    for (int l1 = 0; l1 < E_buffersize; l1++)
    {
      l[l1] = EEPROM.read(i + l1);
      if (!l[l1])
      {
        validend = true;
        break;
      }
    }
    return validend;
  }

  // write 4 byte float into EEPROM at position i (4 bytes)
  void writeFloat(int i, float f)
  {
    writeQuad(i, (byte *)&f);
  }

  // read 4 byte float from EEPROM at position i (4 bytes)
  float readFloat(int i)
  {
    float   f;
    readQuad(i, (byte *)&f);
    return f;
  }

  // write 4 byte long into EEPROM at position i (4 bytes)
  void writeLong(int i, long l)
  {
    writeQuad(i, (byte *)&l);
  }

  void writeULong(int i, unsigned long l)
  {
    writeQuad(i, (byte *)&l);
  }


  // read 4 byte long from EEPROM at position i (4 bytes)
  unsigned long readULong(int i)
  {
    unsigned long    l;
    readQuad(i, (byte *)&l);
    return l;
  }

  // read 4 byte long from EEPROM at position i (4 bytes)
  long readLong(int i)
  {
    long    l;
    readQuad(i, (byte *)&l);
    return l;
  };


};

static extendedEEPROM XEEPROM;