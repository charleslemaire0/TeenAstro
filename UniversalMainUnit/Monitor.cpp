#include "Global.h"

#ifdef __ESP32__
uint32_t EEPROMHash;
#include "RokkitHash.h"
#endif


static ErrorsTracking lastErr;  // private variable protected by semaphore


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


ErrorsTracking lastError(void)
{
  ErrorsTracking error;
  xSemaphoreTake(swMutex, portMAX_DELAY); 
  error = lastErr;
  xSemaphoreGive(swMutex);  
  return error;
}

void lastError(ErrorsTracking error)
{
  xSemaphoreTake(swMutex, portMAX_DELAY); 
  lastErr = error;
  xSemaphoreGive(swMutex);  
}



/*
 * Monitor Task
 * runs every 100mS
 * 
 * Check limits and do housekeeping
 */
void monitorTask(void *arg)
{
  static unsigned counter = 0;
  static ErrorsTracking StartLoopError = ERRT_NONE;
  while (1)
  { 
    StartLoopError = lastError();

    if (!checkAltitude())
    {
      lastError(ERRT_ALT);
    }
    else
    {
      if (lastError() == ERRT_ALT)
      {
        lastError(ERRT_NONE);
      }
    }
    
    // once per second
    if (counter % 10 == 0)
    {
      SafetyCheck();
    }

#ifdef __ESP32__ 
    // Check EEPROM once every 10 seconds
    if (counter % 100 == 0)
    {
      checkEEPROM();
    }
#endif

    if (StartLoopError != lastError())
    {
      lastError() == ERRT_NONE ? digitalWrite(LEDPin, LOW) : digitalWrite(LEDPin, HIGH);
    }
    counter++;
    vTaskDelay(MON_TASK_PERIOD  / portTICK_PERIOD_MS);
  }
}

void SafetyCheck()
{
  // basic check to see if we're not at home
  PierSide currentSide = mount.mP->GetPierSide();
  Steps steps;

  steps.steps1 = motorA1.getCurrentPos();
  steps.steps2 = motorA2.getCurrentPos();

//  if (atHome())
//    atHome = !siderealTracking;

  if (!geoA1.withinLimits(steps.steps1))
  {
    lastError(ERRT_AXIS1);
    if (isSlewing() || isTracking())
      stopMoving();
    return;
  }
  else if (lastError() == ERRT_AXIS1)
  {
    lastError(ERRT_NONE);
  }

  if (!geoA2.withinLimits(steps.steps2))
  {
    lastError(ERRT_AXIS2);
    if (isSlewing() || isTracking())
      stopMoving();
    return;
  }
  else if (lastError() == ERRT_AXIS2)
  {
    lastError(ERRT_NONE);
  }

  if (mount.mP->type == MOUNT_TYPE_GEM)
  {
    Axes axes;
    mount.mP->stepsToAxes(&steps, &axes);
    if (!mount.mP->checkMeridian(&axes, CHECKMODE_TRACKING, currentSide))
    {
      lastError(ERRT_MERIDIAN);
      if (isSlewing() || isTracking())
      {
        stopMoving();
      }
      return;
    }
    else if (lastError() == ERRT_MERIDIAN)
    {
      lastError(ERRT_NONE);
    }

    if (!mount.mP->checkPole(axes.axis1, CHECKMODE_TRACKING))
    {
        lastError(ERRT_UNDER_POLE);
        if (isSlewing() || isTracking())
        {
          stopMoving();
        }
        return;
    }
    else if (lastError() == ERRT_UNDER_POLE)
    {
      lastError(ERRT_NONE);
    }  
  }
}


