#pragma once
#include "EEPROM.h"
// -----------------------------------------------------------------------------------
// EEPROM related functions

// write int numbers into EEPROM at position i (2 bytes)

#ifdef __arm__
#define WRITE_BYTE write 
#define READ_BYTE read 
#endif
#ifdef __ESP32__
#define WRITE_BYTE writeByte
#define READ_BYTE readByte
#endif


struct extendedEEPROM : EEPROMClass
{
  void write(int i, int j)
  {
    EEPROM.WRITE_BYTE(i, j);
  }
  int read(int i)
  {
    return EEPROM.READ_BYTE(i);
  }

  void writeInt(int i, int j)
  {
    uint8_t *k = (uint8_t *)&j;
    EEPROM.WRITE_BYTE(i + 0, *k);
    k++;
    EEPROM.WRITE_BYTE(i + 1, *k);
  }

  // read int numbers from EEPROM at position i (2 bytes)
  int readInt(int i)
  {
    uint16_t    j;
    uint8_t     *k = (uint8_t *)&j;
    *k = EEPROM.READ_BYTE(i + 0);
    k++;
    *k = EEPROM.READ_BYTE(i + 1);
    return j;
  }

  // write 4 byte variable into EEPROM at position i (4 bytes)
  void writeQuad(int i, byte *v)
  {
    EEPROM.WRITE_BYTE(i + 0, *v);
    v++;
    EEPROM.WRITE_BYTE(i + 1, *v);
    v++;
    EEPROM.WRITE_BYTE(i + 2, *v);
    v++;
    EEPROM.WRITE_BYTE(i + 3, *v);
  }

  // read 4 byte variable from EEPROM at position i (4 bytes)
  void readQuad(int i, byte *v)
  {
    *v = EEPROM.READ_BYTE(i + 0);
    v++;
    *v = EEPROM.READ_BYTE(i + 1);
    v++;
    *v = EEPROM.READ_BYTE(i + 2);
    v++;
    *v = EEPROM.READ_BYTE(i + 3);
  }

  // write String into EEPROM at position i
  bool writeString(int i, char l[], int E_buffersize)
  {
    if ((int)strlen(l) + 1 > E_buffersize)
      return false;
    for (int l1 = 0; l1 < (int)strlen(l) + 1; l1++)
    {
      EEPROM.WRITE_BYTE(i + l1, l[l1]);
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
      l[l1] = EEPROM.READ_BYTE(i + l1);
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