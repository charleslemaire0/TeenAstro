#include "Global.h"
// Method for checking limits, all checks are done in stepper cooordinates


bool withinLimits(long axis1, long axis2)
{
  if (! ((geoA1.withinLimits(axis1) && geoA2.withinLimits(axis2))))
    return false;
  return true;
}

/*
 * checkAltitude
 * return false if altitude goes above or below limit
 */
bool checkAltitude(void)
{
  xSemaphoreTake(swMutex, portMAX_DELAY); 
  prevAzAlt.az = currentAzAlt.az;       
  prevAzAlt.alt = currentAzAlt.alt;       // to compute direction of altitude change
  mount.mP->getHorApp(&currentAzAlt);
  xSemaphoreGive(swMutex);

  // check altitude overhead limit and horizon limit
  if (currentAzAlt.alt < limits.minAlt || currentAzAlt.alt > limits.maxAlt)
  {
    return false;
  }
  return true;
}



// init the telescope home position;  if defined use the user defined home position
void initLimits()
{
  initLimitMinAxis1();
  initLimitMaxAxis1();
  initLimitMinAxis2();
  initLimitMaxAxis2();
}


void initLimitMinAxis1()
{
  int val = XEEPROM.readInt(getMountAddress(EE_minAxis1));
  if (val < 0 || val > 3600)
  {
    val = 3600;
    XEEPROM.writeInt(getMountAddress(EE_minAxis1), val);
  }
  geoA1.minAxis = -val * geoA1.stepsPerDegree / 10.0;
}

void initLimitMaxAxis1()
{
  int val = XEEPROM.readInt(getMountAddress(EE_maxAxis1));
  if (val < 0 || val > 3600)
  {
    val = 3600;
    XEEPROM.writeInt(getMountAddress(EE_maxAxis1), val);
  }
  geoA1.maxAxis = val * geoA1.stepsPerDegree / 10.0;
}

void initLimitMinAxis2()
{
  int val = XEEPROM.readInt(getMountAddress(EE_minAxis2));
  if (val < 0 || val > 3600)
  {
    val = 3600;
    XEEPROM.writeInt(getMountAddress(EE_minAxis2), val);
  }
  geoA2.minAxis = -val * geoA2.stepsPerDegree / 10.0;
}

void initLimitMaxAxis2()
{
  int val = XEEPROM.readInt(getMountAddress(EE_maxAxis2));
  if (val < 0 || val > 3600)
  {
    val = 3600;
    XEEPROM.writeInt(getMountAddress(EE_maxAxis2), val);
  }
  geoA2.maxAxis = val * geoA2.stepsPerDegree / 10.0;
}

