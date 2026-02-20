#include "MountAxisLimitManager.h"
#include "MainUnit.h"

namespace {
const int AXIS_LIMIT_EEPROM_SCALE = 10;
const int AXIS_LIMIT_FORCE_RESET_MIN = 9999;
const int AXIS_LIMIT_FORCE_RESET_MAX = -9999;
}

MountAxisLimitManager::MountAxisLimitManager(Mount& mount) : mount_(mount) {}

void MountAxisLimitManager::resetEELimit()
{
  mount_.axes.geoA1.minAxis = (long)(mount_.axes.geoA1.LimMinAxis * mount_.axes.geoA1.stepsPerDegree);
  mount_.axes.geoA1.maxAxis = (long)(mount_.axes.geoA1.LimMaxAxis * mount_.axes.geoA1.stepsPerDegree);
  mount_.axes.geoA2.minAxis = (long)(mount_.axes.geoA2.LimMinAxis * mount_.axes.geoA2.stepsPerDegree);
  mount_.axes.geoA2.maxAxis = (long)(mount_.axes.geoA2.LimMaxAxis * mount_.axes.geoA2.stepsPerDegree);
  XEEPROM.writeShort(getMountAddress(EE_minAxis1), AXIS_LIMIT_EEPROM_SCALE * mount_.axes.geoA1.LimMinAxis);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis1), AXIS_LIMIT_EEPROM_SCALE * mount_.axes.geoA1.LimMaxAxis);
  XEEPROM.writeShort(getMountAddress(EE_minAxis2), AXIS_LIMIT_EEPROM_SCALE * mount_.axes.geoA2.LimMinAxis);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis2), AXIS_LIMIT_EEPROM_SCALE * mount_.axes.geoA2.LimMaxAxis);
}

void MountAxisLimitManager::forceResetEELimit()
{
  XEEPROM.writeShort(getMountAddress(EE_minAxis1), AXIS_LIMIT_FORCE_RESET_MIN);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis1), AXIS_LIMIT_FORCE_RESET_MAX);
  XEEPROM.writeShort(getMountAddress(EE_minAxis2), AXIS_LIMIT_FORCE_RESET_MIN);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis2), AXIS_LIMIT_FORCE_RESET_MAX);
}

bool MountAxisLimitManager::initAxisLimit(GeoAxis& geo, int eeAddress, bool isMax)
{
  bool ok = true;
  int val = XEEPROM.readShort(getMountAddress(eeAddress));
  int minval = AXIS_LIMIT_EEPROM_SCALE * geo.LimMinAxis;
  int maxval = AXIS_LIMIT_EEPROM_SCALE * geo.LimMaxAxis;
  if (val < minval || val > maxval)
  {
    val = isMax ? maxval : minval;
    XEEPROM.writeShort(getMountAddress(eeAddress), val);
    ok = false;
  }
  if (isMax)
    geo.maxAxis = (long)(val * geo.stepsPerDegree / AXIS_LIMIT_EEPROM_SCALE);
  else
    geo.minAxis = (long)(val * geo.stepsPerDegree / AXIS_LIMIT_EEPROM_SCALE);
  return ok;
}

bool MountAxisLimitManager::initLimitMinAxis1() { return initAxisLimit(mount_.axes.geoA1, EE_minAxis1, false); }
bool MountAxisLimitManager::initLimitMaxAxis1() { return initAxisLimit(mount_.axes.geoA1, EE_maxAxis1, true); }
bool MountAxisLimitManager::initLimitMinAxis2() { return initAxisLimit(mount_.axes.geoA2, EE_minAxis2, false); }
bool MountAxisLimitManager::initLimitMaxAxis2() { return initAxisLimit(mount_.axes.geoA2, EE_maxAxis2, true); }
