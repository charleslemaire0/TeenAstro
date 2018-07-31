#pragma once
#include "Helper_math.h"
#include "EEPROM.h"
#include "Helper_EEProm.h"
class siteDefinition
{
  // site index: 100-199
  // 100..103 latitude  1  ((index 1-1)*25+100)
  // 104..107 longitude 1
  // 108      timeZone  1
  // 109..124 site name 1
  // 125..128 latitude  2  ((index 2-1)*25+100)
  // 129..132 longitude 2
  // 133      timeZone  2
  // 134..149 site name 2
  // 150..103 latitude  3  ((index 3-1)*25+100)
  // 154..157 longitude 3
  // 158      timeZone  3
  // 159..174 site name 3
  // 175..178 latitude  4  ((index 4-1)*25+100)
  // 179..182 longitude 4
  // 183      timeZone  4
  // 184..199 site name 4
#define EE_currentSite  100
#define EE_sites        101

  struct ssite
  {
    double latitude;
    double longitude;
    char siteName[16];
  };
private:
  uint8_t  m_siteIndex;
  ssite  m_site;
  double m_cosLat;
  double m_sinLat;

public:
  const uint8_t siteIndex()
  {
    return m_siteIndex;
  }
  const double* latitude()
  {
    return &m_site.latitude;
  }
  const double* longitude()
  {
    return &m_site.longitude;
  }
  const char* siteName()
  {
    return &m_site.siteName[0];
  }
  bool setLat(const double l)
  {
    if (-90 > l || l > 90)
    {
      return false;
    }
    m_site.latitude = l;
    EEPROM_writeFloat(EE_sites + m_siteIndex * 25, (float)m_site.latitude);
    m_cosLat = cos(m_site.latitude / Rad);
    m_sinLat = sin(m_site.latitude / Rad);
    return true;
  }
  bool setLong(const double l)
  {
    if (-360 > l || l > 360)
    {
      return false;
    }
    m_site.longitude = l;
    EEPROM_writeFloat(EE_sites + m_siteIndex * 25 + 4, (float)m_site.longitude);
    return true;
  }
  bool setSiteName(const char* s)
  {
    strncpy(m_site.siteName, s, 16);
    EEPROM_writeString(EE_sites + m_siteIndex * 25 + 9, &m_site.siteName[0]);
    return true;
  }
  const double sinLat()
  {
    return m_sinLat;
  }
  const double cosLat()
  {
    return m_cosLat;
  }
  void ReadSiteDefinition(uint8_t siteIndex)
  {
    m_siteIndex = siteIndex;
    int adress = EE_sites + m_siteIndex * 25;
    m_site.latitude = EEPROM_readFloat(adress);
    if (-90 > m_site.latitude || m_site.latitude > 90)
    {
      m_site.latitude = 0;
      EEPROM_writeFloat(EE_sites + m_siteIndex * 25, (float)m_site.latitude);
    }

    adress += 4;
    m_site.longitude = EEPROM_readFloat(adress);
    if (-180 > m_site.longitude || m_site.longitude > 180)
    {
      m_site.longitude = 0;
      EEPROM_writeFloat(EE_sites + m_siteIndex * 25 + 4, (float)m_site.longitude);
    }
    adress += 4;
    adress += 1;
    EEPROM_readString(adress, m_site.siteName);
    m_cosLat = cos(m_site.latitude / Rad);
    m_sinLat = sin(m_site.latitude / Rad);
  }
  void ReadCurrentSiteDefinition()
  {
    m_siteIndex = EEPROM.read(EE_currentSite);
    if (m_siteIndex > 3)
    {
      initdefault();
    }
    else
      ReadSiteDefinition(m_siteIndex);
  }
  void initdefault()
  {
    // init the site information, lat/long/tz/name
    m_siteIndex = 0;
    EEPROM.write(EE_currentSite, m_siteIndex);
    setLat(0);
    setLong(0);
    setSiteName("INIT");
  }
};

