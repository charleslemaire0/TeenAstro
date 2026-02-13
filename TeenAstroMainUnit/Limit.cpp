// Methode for limit checks, all checks are done in stepper cooordinates
#include "Global.h"

void setAtMount(long &axis1, long &axis2)
{
  cli();
  axis1 = mount.staA1.pos;
  axis2 = mount.staA2.pos;
  sei();
}

PoleSide getPoleSide(const long &axis2)
{
  return -mount.geoA2.quaterRot <= axis2 && axis2 <= mount.geoA2.quaterRot ? POLE_UNDER : POLE_OVER;
}

//for GEM
bool checkPole(const long &axis1, const long& axis2, CheckMode mode)
{
  bool ok = false;
  double underPoleLimit = (mode == CHECKMODE_GOTO) ? mount.underPoleLimitGOTO : mount.underPoleLimitGOTO + 5.0 / 60;
  PoleSide _currPoleSide = getPoleSide(axis2);
  switch (_currPoleSide)
  {
  case POLE_UNDER:
    ok = (axis1 < mount.geoA1.poleDef + (underPoleLimit - 6) * 15. * mount.geoA1.stepsPerDegree);
    break;
  case POLE_OVER:
    ok = (axis1 > mount.geoA1.poleDef - (underPoleLimit - 6) * 15. * mount.geoA1.stepsPerDegree);
    break;
  default:
    ok = false;
    break;
  }
  return ok ;
}

bool checkMeridian(const long &axis1, const long &axis2, CheckMode mode)
{
  bool ok = true;
  double MinutesPastMeridianW, MinutesPastMeridianE;
  PoleSide _currPoleSide = getPoleSide(axis2);
  if (mode == CHECKMODE_GOTO)
  {
    MinutesPastMeridianW = mount.minutesPastMeridianGOTOW;
    MinutesPastMeridianE = mount.minutesPastMeridianGOTOE;
  }
  else
  {
    //CHECKMODETRACKING ONLY FOR GEM
    MinutesPastMeridianW = mount.minutesPastMeridianGOTOW + 5;
    MinutesPastMeridianE = mount.minutesPastMeridianGOTOE + 5;
    if (mount.distanceFromPoleToKeepTrackingOn < 180) // false if keepTrackingOnWhenFarFromPole (customization) is not defined
    {
      if (_currPoleSide == POLE_UNDER)
      {
        if (axis2 < (90 - mount.distanceFromPoleToKeepTrackingOn) * mount.geoA2.stepsPerDegree)
          MinutesPastMeridianW = MinutesPastMeridianE = 360;
      }
      else if (_currPoleSide == POLE_OVER)
      {
        if (axis2 > (90 + mount.distanceFromPoleToKeepTrackingOn) * mount.geoA2.stepsPerDegree)
          MinutesPastMeridianW = MinutesPastMeridianE = 360;
      }
    }
  }
  switch (_currPoleSide)
  {
  case POLE_UNDER:
    ok = axis1 > mount.geoA1.poleDef - mount.geoA1.quaterRot - (MinutesPastMeridianE / 60.) * 15.0 * mount.geoA1.stepsPerDegree;
    break;
  case POLE_OVER:
    ok = axis1 <  mount.geoA1.poleDef + mount.geoA1.quaterRot + (MinutesPastMeridianW / 60.) * 15.0 * mount.geoA1.stepsPerDegree;
    break;
  default:
    ok = false;
    break;
  }
  return ok;
}




// check if defined position is within limit
bool withinLimit(const long &axis1, const long &axis2)
{
  bool ok = false;
  ok = mount.geoA1.withinLimit(axis1) && mount.geoA2.withinLimit(axis2);
  if (!ok)
    return ok;
  if (isAltAZ())
  {
    
  }
  else
  {
    if (mount.mountType == MOUNT_TYPE_GEM)
    {
      ok = checkPole(axis1, axis2, CHECKMODE_GOTO);
      if (!ok)
        return ok;
    }
       
    if (mount.meridianFlip == FLIP_ALWAYS)
      ok = checkMeridian(axis1, axis2, CHECKMODE_GOTO);
  }
  return ok;
}

// init the telescope home position;  if defined use the user defined home position
void initLimit()
{
  bool ok = initLimitMinAxis1();
  ok &= initLimitMaxAxis1();
  ok &= mount.geoA1.maxAxis > mount.geoA1.minAxis;
  ok &= initLimitMinAxis2();
  ok &= initLimitMaxAxis2();
  ok &= mount.geoA2.maxAxis > mount.geoA2.minAxis;
  if (!ok)
  {
    reset_EE_Limit();
  }
}

void reset_EE_Limit()
{
  mount.geoA1.minAxis = mount.geoA1.LimMinAxis * mount.geoA1.stepsPerDegree;
  mount.geoA1.maxAxis = mount.geoA1.LimMaxAxis * mount.geoA1.stepsPerDegree;
  mount.geoA2.minAxis = mount.geoA2.LimMinAxis * mount.geoA2.stepsPerDegree;
  mount.geoA2.maxAxis = mount.geoA2.LimMaxAxis * mount.geoA2.stepsPerDegree;
  XEEPROM.writeShort(getMountAddress(EE_minAxis1), 10 * mount.geoA1.LimMinAxis);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis1), 10 * mount.geoA1.LimMaxAxis);
  XEEPROM.writeShort(getMountAddress(EE_minAxis2), 10 * mount.geoA2.LimMinAxis);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis2), 10 * mount.geoA2.LimMaxAxis);
}

void force_reset_EE_Limit()
{
  XEEPROM.writeShort(getMountAddress(EE_minAxis1), 9999);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis1), -9999);
  XEEPROM.writeShort(getMountAddress(EE_minAxis2), 9999);
  XEEPROM.writeShort(getMountAddress(EE_maxAxis2), -9999);
}

bool initLimitMinAxis1()
{
  bool ok = true;
  int val = XEEPROM.readShort(getMountAddress(EE_minAxis1));
  int minval = 10 * mount.geoA1.LimMinAxis;
  int maxval = 10 * mount.geoA1.LimMaxAxis;
  if (val < minval || val > maxval)
  {
    val = minval;
    XEEPROM.writeShort(getMountAddress(EE_minAxis1), val);
    ok = false;
  }
  mount.geoA1.minAxis = val * mount.geoA1.stepsPerDegree / 10.0;
  return ok;
}
bool initLimitMaxAxis1()
{
  bool ok = true;
  int val = XEEPROM.readShort(getMountAddress(EE_maxAxis1));
  int minval = 10 * mount.geoA1.LimMinAxis;
  int maxval = 10 * mount.geoA1.LimMaxAxis;;
  if (val < minval || val > maxval)
  {
    val = maxval;
    XEEPROM.writeShort(getMountAddress(EE_maxAxis1), val);
    ok = false;
  }
  mount.geoA1.maxAxis = val * mount.geoA1.stepsPerDegree / 10.0;
  return ok;
}
bool initLimitMinAxis2()
{
  bool ok = true;
  int val = XEEPROM.readShort(getMountAddress(EE_minAxis2));
  int minval = 10 * mount.geoA2.LimMinAxis;
  int maxval = 10 * mount.geoA2.LimMaxAxis;
  if (val < minval || val > maxval)
  {
    val = minval;
    XEEPROM.writeShort(getMountAddress(EE_minAxis2), val);
    ok = false;
  }
  mount.geoA2.minAxis = val * mount.geoA2.stepsPerDegree / 10.0;
  return ok;
}
bool initLimitMaxAxis2()
{
  bool ok = true;
  int val = XEEPROM.readShort(getMountAddress(EE_maxAxis2));
  int minval = 10 * mount.geoA2.LimMinAxis;
  int maxval = 10 * mount.geoA2.LimMaxAxis;
  if (val < minval || val > maxval)
  {
    val = maxval;
    XEEPROM.writeShort(getMountAddress(EE_maxAxis2), val);
    ok = false;
  }
  mount.geoA2.maxAxis = val * mount.geoA2.stepsPerDegree / 10.0;
  return ok;
}
