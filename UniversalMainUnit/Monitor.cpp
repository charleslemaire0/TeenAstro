#include "Global.h"

#ifdef __ESP32__
uint32_t EEPROMHash;
#include "RokkitHash.h"
#endif




#ifdef __ESP32__
/*
 * checkEEPROM
 * for ESP32 that uses a RAM image and flash instead of an EEPROM
 * checks if EEPROM RAM image has changed by performing a hash
 * and commits if necessary
 */
void checkEEPROM(void)
{
  const char *dP;
  uint32_t hash;
  unsigned len;

  dP = (const char*) EEPROM.getDataPtr();
  len = EEPROM.length();

  hash = rokkit(dP , len);
  if (hash != EEPROMHash)   // global variable
  {
    EEPROMHash = hash;
    EEPROM.commit();
  }
}
#endif





/*
 * Monitor Task
 * runs every 100mS
 * 
 * Check limits and do housekeeping
 */
void monitorTask(void *arg)
{
  unsigned counter = 0;
  while (1)
  { 
    bool ledStatus;

    if (!checkAltitude())
    {
      setEvents(EV_ERROR);
      lastError = ERRT_ALT;
    }
    else
    {
      if (lastError != ERRT_NONE)
      {
        lastError = ERRT_NONE;
        resetEvents(EV_ERROR);
      }
    }
    
    // display hearbeat once per second
    if (counter % 10 == 0)
    {
      ledStatus = !ledStatus;
//      digitalWrite(LED_BUILTIN, ledStatus); // LED not implemented on all platforms
    }
    counter++;

#ifdef __ESP32__ 
    // Check EEPROM once every 10 seconds
    if (counter % 100 == 0)
    {
      checkEEPROM();
    }
#endif

    vTaskDelay(MON_TASK_PERIOD  / portTICK_PERIOD_MS);
  }





}

// not yet implemented
void SafetyCheck(const bool forceTracking)
{
#if 0  
  // basic check to see if we're not at home
  PierSide currentSide = GetPierSide();
  long axis1, axis2;
  setAtMount(axis1, axis2);

  if (atHome)
    atHome = !siderealTracking;
  if (!geoA1.withinLimits(axis1))
  {
    lastError = ERRT_AXIS1;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      siderealTracking = false;
    return;
  }
  else if (lastError == ERRT_AXIS1)
  {
    lastError = ERRT_NONE;
  }

  if (!geoA2.withinLimits(axis2))
  {
    lastError = ERRT_AXIS2;
    if (movingTo)
      abortSlew = true;
    else if (!forceTracking)
      siderealTracking = false;
    return;
  }
  else if (lastError == ERRT_AXIS2)
  {
    lastError = ERRT_NONE;
  }
  if (mount.type == MOUNT_TYPE_GEM)
  {
    if (!checkMeridian(axis1, axis2, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == PIER_WEST) || (!staA2.dir && currentSide == PIER_EAST))
      {
        lastError = ERRT_MERIDIAN;
        if (movingTo)
        {
          abortSlew = true;
        }
        if (currentSide >= PIER_WEST && !forceTracking)
          siderealTracking = false;
        return;
      }
      else if (lastError == ERRT_MERIDIAN)
      {
        lastError = ERRT_NONE;
      }
    }
    else if (lastError == ERRT_MERIDIAN)
    {
      lastError = ERRT_NONE;
    }
    if (!checkPole(axis1, CHECKMODE_TRACKING))
    {
      if ((staA1.dir && currentSide == PIER_EAST) || (!staA2.dir && currentSide == PIER_WEST))
      {
        lastError = ERRT_UNDER_POLE;
        if (movingTo)
          abortSlew = true;
        if (currentSide == PIER_EAST && !forceTracking)
          siderealTracking = false;
        return;
      }
      else if (lastError == ERRT_UNDER_POLE)
      {
        lastError = ERRT_NONE;
      }
    }
    else if (lastError == ERRT_UNDER_POLE)
    {
      lastError = ERRT_NONE;
    }  
  }
#endif      
}


