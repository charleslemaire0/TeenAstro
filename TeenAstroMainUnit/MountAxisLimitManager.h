#pragma once
/**
 * Manages per-axis min/max limits (in steps) and their EEPROM persistence.
 * Operational limits (alt, meridian, under-pole) are in MountLimits.
 */
class Mount;
class GeoAxis;

class MountAxisLimitManager {
public:
  explicit MountAxisLimitManager(Mount& mount);
  void resetEELimit();
  void forceResetEELimit();
  bool initLimitMinAxis1();
  bool initLimitMaxAxis1();
  bool initLimitMinAxis2();
  bool initLimitMaxAxis2();
private:
  bool initAxisLimit(GeoAxis& geo, int eeAddress, bool isMax);
  Mount& mount_;
};
