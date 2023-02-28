#pragma once

#include "EEPROM_adress.h"
#include "XEEPROM.hpp"

class siteDefinition
{

#define siteNameLen     15 // withnull character!
#define SiteSize        27
#define maxNumSite       3
  struct ssite
  {
    double latitude;
    double longitude;
    int16_t elevation;
    char siteName[siteNameLen];
    float toff;
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
  const int16_t* elevation()
  {
    return &m_site.elevation;
  }
  const float* toff()
  {
    return &m_site.toff;
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

    XEEPROM.writeFloat(EE_sites + m_siteIndex * SiteSize, (float)m_site.latitude);
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
    XEEPROM.writeFloat(EE_sites + m_siteIndex * SiteSize + EE_site_long, (float)m_site.longitude);
    return true;
  }
  bool setElev(const int16_t l)
  {
    if (-200 > l || l > 8000)
    {
      return false;
    }
    m_site.elevation = l;
    XEEPROM.writeInt(EE_sites + m_siteIndex * SiteSize + EE_site_height, l);
    return true;
  }
  bool setToff(float toff)
  {
    if (toff <= -12 || toff >= 12)
      return false;
    m_site.toff = toff;
    int val = (toff + 12.0) * 10.0;
    XEEPROM.write(EE_sites + m_siteIndex * SiteSize + EE_site_time, val);
    return true;
  }
  bool setSiteName(const char* s)
  {
    if (strlen(s) + 1 > siteNameLen)
      return false;
    strcpy(m_site.siteName, s);
    return XEEPROM.writeString(EE_sites + m_siteIndex * SiteSize + EE_site_name, m_site.siteName, siteNameLen);
  }
  const double* sinLat()
  {
    return &m_sinLat;
  }
  const double* cosLat()
  {
    return &m_cosLat;
  }
  void ReadSiteDefinition(uint8_t siteIndex)
  {
    if (siteIndex > maxNumSite)
      return;
    m_siteIndex = siteIndex;
    int adress = EE_sites + m_siteIndex * SiteSize;
    m_site.latitude = XEEPROM.readFloat(adress);
    if (-90 > m_site.latitude || m_site.latitude > 90)
    {
      setLat(0);
    }
    adress += 4;
    m_site.longitude = XEEPROM.readFloat(adress);
    if (-360 >= m_site.longitude || m_site.longitude >= 360)
    {
      setLong(0);
    }
    adress += 4;
    m_site.elevation = XEEPROM.readInt(adress);
    if (-200 > m_site.elevation || m_site.elevation > 8000)
    {
      setElev(0);
    }
    adress += 2;
    m_site.toff = (float)EEPROM.read(adress) / 10.0 - 12.0;
    if (m_site.toff < -12 && m_site.toff >12)
    {
      setToff(0);
    }
    adress++;
    bool ok = XEEPROM.readString(adress, m_site.siteName, siteNameLen);
    if (!ok || strlen(m_site.siteName) == 0)
    {
      char txt[10];
      sprintf(txt, "Site %d", m_siteIndex);
      setSiteName(txt);
    }
    m_cosLat = cos(m_site.latitude / Rad);
    m_sinLat = sin(m_site.latitude / Rad);
  }
  void ReadCurrentSiteDefinition()
  {
    m_siteIndex = XEEPROM.read(EE_currentSite);
    if (m_siteIndex > maxNumSite)
    {
      initdefault();
    }
    else
      ReadSiteDefinition(m_siteIndex);
  }
  void initdefault()
  {
    // init the site information, lat/long/tz/name
    char txt[10];
    for (int k = 0; k < maxNumSite - 1; k++)
    {
      m_siteIndex = k;
      XEEPROM.write(EE_currentSite, m_siteIndex);
      setLat(0);
      setLong(0);
      setElev(0);
      sprintf(txt, "Site %d", k);
      setSiteName(txt);
    }
  }
};
