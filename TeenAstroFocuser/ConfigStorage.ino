// 
// 
// 
#include <EEPROM.h>
#include "ConfigStorage.h"

#define TAF_curr_default  50
#define TAF_curr_min 10
#define TAF_curr_max 160
#define TAF_micro_default  4
#define TAF_micro_min 2
#define TAF_micro_max 7
#define TAF_steprot_default  200
#define TAF_steprot_min 10
#define TAF_steprot_max 800
#define TAF_pos_default 0
#define TAF_pos_min 0
#define TAF_pos_max 2000000000UL
#define TAF_speed_default 20
#define TAF_speed_min 1
#define TAF_speed_max 999
#define TAF_acc_default 30
#define TAF_acc_min 1
#define TAF_acc_max 99
#define TAF_res_default 16
#define TAF_res_min 1
#define TAF_res_max 512


void EEPROMwrite(int idx, char* objaddress, unsigned int len)
{
  for (unsigned int t = 0; t < len; t++)
    EEPROM.write(idx + t, *(objaddress + t));
}
void EEPROMwrite_uint(int idx, unsigned int a)
{
  EEPROMwrite(idx, (char*)&a, sizeof(a));
}
void EEPROMwrite_ulong(int idx, unsigned long a)
{
  EEPROMwrite(idx, (char*)&a, sizeof(a));
}
void EEPROMread(int idx, char* objaddress, unsigned int len)
{
  for (unsigned int t = 0; t < len; t++)
    *(objaddress + t) = EEPROM.read(idx + t);
}
void EEPROMread_uint(int idx, unsigned int &a)
{
  EEPROMread(idx, (char*)&a, sizeof(a));
}
void EEPROMread_ulong(int idx, unsigned long &a)
{
  EEPROMread(idx, (char*)&a, sizeof(a));
}

//---------------------------------------------------------------------------------------------
Parameteruint8_t::Parameteruint8_t(const int &adress, const uint8_t &value_default, const  uint8_t &value_min, const  uint8_t &value_max)
{
  m_adress = adress;
  m_value_default = value_default;
  m_value_min = value_min;
  m_value_max = value_max;
  load();
}
uint8_t Parameteruint8_t::get()
{
  return m_value;
}
uint8_t Parameteruint8_t::getmin()
{
  return m_value_min;
}
uint8_t Parameteruint8_t::getmax()
{
  return m_value_max;
}
bool Parameteruint8_t::set(const uint8_t &val)
{
  if (val<m_value_min || val>m_value_max)
    return false;
  if (val == m_value)
    return true;
  m_value = val;
  EEPROM.write(m_adress, m_value);
  return true;
}
void Parameteruint8_t::load()
{
  uint val = EEPROM.read(m_adress);
  if (val<m_value_min || val>m_value_max)
    set(m_value_default);
  else
    m_value = val;
}

//---------------------------------------------------------------------------------------------
Parameteruint::Parameteruint(const int &adress, const unsigned int &value_default, const  unsigned int &value_min, const  unsigned int &value_max)
{
  m_adress = adress;
  m_value_default = value_default;
  m_value_min = value_min;
  m_value_max = value_max;
  load();
}
unsigned int Parameteruint::get()
{
  return m_value;
}
unsigned int Parameteruint::getmin()
{
  return m_value_min;
}
unsigned int Parameteruint::getmax()
{
  return m_value_max;
}
bool Parameteruint::set(const unsigned int &val)
{
  if (val<m_value_min || val>m_value_max)
    return false;
  if (val == m_value)
    return true;
  m_value = val;
  EEPROMwrite_uint(m_adress, m_value);
  return true;
}
void Parameteruint::load()
{
  EEPROMread_uint(m_adress, m_value);
  if (m_value<m_value_min || m_value>m_value_max)
  {
    set(m_value_default);
  }

}

//---------------------------------------------------------------------------------------------
Parameterulong::Parameterulong(const int &adress, const unsigned long &value_default, const  unsigned long &value_min, const  unsigned long &value_max)
{
  m_adress = adress;
  m_value_default = value_default;
  m_value_min = value_min;
  m_value_max = value_max;
  load();
}
unsigned long Parameterulong::get()
{
  return m_value;
}
unsigned long Parameterulong::getmin()
{
  return m_value_min;
}
unsigned long Parameterulong::getmax()
{
  return m_value_max;
}
bool Parameterulong::set(const unsigned long &val)
{
  if (val<m_value_min || val>m_value_max)
    return false;
  if (val == m_value)
    return true;
  m_value = val;
  EEPROMwrite_ulong(m_adress, m_value);
  return true;
}
void Parameterulong::load()
{
  EEPROMread_ulong(m_adress, m_value);
  if (m_value<m_value_min || m_value>m_value_max)
    set(m_value_default);
}
//---------------------------------------------------------------------------------------------

ParameterPosition::ParameterPosition(const int &adress)
{
  m_pos = new Parameterulong(adress, 0, TAF_pos_min, TAF_pos_max);
  m_adress = adress + sizeof(unsigned long);
  for (int k = 0; k < 11; k++)
  {
    m_id[k] = EEPROM.read(m_adress + k);
  }
}
void ParameterPosition::get(char* id, unsigned long &pos)
{
  memcpy(id, m_id, sizeof(m_id));
  m_id[10] = 0;
  pos = m_pos->get();
}
unsigned long ParameterPosition::getPosition()
{
  return m_pos->get();
}
bool ParameterPosition::isvalid()
{
  return m_id[0] != 0;
}
bool ParameterPosition::set(char* id, const unsigned long &pos)
{
  bool endreached = false;
  for (int k = 0; k < 10; k++)
  {
    if (id[k] == '#' || id[k] == 0)
      endreached = true;
    m_id[k] = endreached ? 0 : id[k];
    EEPROM.write(m_adress + k, (uint8_t)m_id[k]);
  }
  m_id[10] = 0;
  EEPROM.write(m_adress + 10, 0);
  m_pos->set(pos);
  return true;
}


void loadConfig()
{
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  int k = CONFIG_START;
  resolution = new Parameteruint(k, TAF_res_default, TAF_res_min, TAF_res_max);
  k += sizeof(unsigned int);
  curr = new Parameteruint8_t(k, TAF_curr_default, TAF_curr_min, TAF_curr_max);
  k += sizeof(uint8_t);
  micro = new Parameteruint8_t(k, TAF_micro_default, TAF_micro_min, TAF_micro_max);
  k += sizeof(uint8_t);
  steprot = new Parameteruint(k, TAF_steprot_default, TAF_steprot_min, TAF_steprot_max);
  k += sizeof(unsigned int);
  reverse = new Parameteruint8_t(k, 0, 0, 1);
  k += sizeof(uint8_t);
  startPosition = new Parameterulong(k, TAF_pos_default, TAF_pos_min, min(TAF_pos_max, 65535 * resolution->get()));
  k += sizeof(unsigned long);
  maxPosition = new Parameterulong(k, 65535, TAF_pos_min, min(TAF_pos_max, 65535 * resolution->get()));
  k += sizeof(unsigned long);
  lowSpeed = new Parameteruint(k, TAF_speed_default, TAF_speed_min, TAF_speed_max);
  k += sizeof(unsigned int);
  highSpeed = new Parameteruint(k, TAF_speed_default, TAF_speed_min, TAF_speed_max);
  k += sizeof(unsigned int);
  cmdAcc = new Parameteruint8_t(k, TAF_acc_default, TAF_acc_min, TAF_acc_max);
  k += sizeof(uint8_t);
  manAcc = new Parameteruint8_t(k, TAF_acc_default, TAF_acc_min, TAF_acc_max);
  k += sizeof(uint8_t);
  manDec = new Parameteruint8_t(k, TAF_acc_default, TAF_acc_min, TAF_acc_max);
  k += sizeof(uint8_t);
  for (int i = 0; i < 10; i++)
  {
    PositionList[i] = new ParameterPosition(k);
    k += sizeof(unsigned long) + sizeof(char[11]);
  }
}




