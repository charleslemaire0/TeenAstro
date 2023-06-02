//Methode for limit checks, all checks are done in stepper cooordinates

void setAtMount(long &axis1, long &axis2)
{
  cli();
  axis1 = staA1.pos;
  axis2 = staA2.pos;
  sei();
}

PierSide getPierSide(const long &axis2)
{
  return -geoA2.quaterRot <= axis2 && axis2 <= geoA2.quaterRot ? PIER_EAST : PIER_WEST;
}

//for GEM
bool checkPole(const long &axis1, const long& axis2, CheckMode mode)
{
  bool ok = false;
  double underPoleLimit = (mode == CHECKMODE_GOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0 / 60;
  PierSide _currPierSide = getPierSide(axis2);
  switch (_currPierSide)
  {
  case PIER_EAST:
    ok = (axis1 < geoA1.poleDef + (underPoleLimit - 6) * 15. * geoA1.stepsPerDegree);
    break;
  case PIER_WEST:
    ok = (axis1 > geoA1.poleDef - (underPoleLimit - 6) * 15. * geoA1.stepsPerDegree);
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
  PierSide _currPierSide = getPierSide(axis2);
  if (mode == CHECKMODE_GOTO)
  {
    MinutesPastMeridianW = minutesPastMeridianGOTOW;
    MinutesPastMeridianE = minutesPastMeridianGOTOE;
  }
  else
  {
    //CHECKMODETRACKING ONLY FOR GEM
    MinutesPastMeridianW = minutesPastMeridianGOTOW + 5;
    MinutesPastMeridianE = minutesPastMeridianGOTOE + 5;
    if (distanceFromPoleToKeepTrackingOn < 180) // false if keepTrackingOnWhenFarFromPole (customization) is not defined
    {
      if (_currPierSide == PIER_EAST)
      {
        if (axis2 < (90 - distanceFromPoleToKeepTrackingOn) * geoA2.stepsPerDegree)
          MinutesPastMeridianW = MinutesPastMeridianE = 360;
      }
      else if (_currPierSide == PIER_WEST)
      {
        if (axis2 > (90 + distanceFromPoleToKeepTrackingOn) * geoA2.stepsPerDegree)
          MinutesPastMeridianW = MinutesPastMeridianE = 360;
      }
    }
  }
  switch (_currPierSide)
  {
  case PIER_EAST:
    ok = axis1 > geoA1.poleDef - geoA1.quaterRot - (MinutesPastMeridianE / 60.) * 15.0 * geoA1.stepsPerDegree;
    break;
  case PIER_WEST:
    ok = axis1 <  geoA1.poleDef + geoA1.quaterRot + (MinutesPastMeridianW / 60.) * 15.0 * geoA1.stepsPerDegree;
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
  ok = geoA1.withinLimit(axis1) && geoA2.withinLimit(axis2);
  if (!ok)
    return ok;
  if (isAltAZ())
  {
    
  }
  else
  {
    if (mountType == MOUNT_TYPE_GEM)
    {
      ok = checkPole(axis1, axis2, CHECKMODE_GOTO);
      if (!ok)
        return ok;
    }
       
    if (meridianFlip == FLIP_ALWAYS)
      ok = checkMeridian(axis1, axis2, CHECKMODE_GOTO);
  }
  return ok;
}

// init the telescope home position;  if defined use the user defined home position
void initLimit()
{
  initLimitMinAxis1();
  initLimitMaxAxis1();
  initLimitMinAxis2();
  initLimitMaxAxis2();
}

void reset_EE_Limit()
{
  XEEPROM.writeInt(getMountAddress(EE_minAxis1), 3600);
  XEEPROM.writeInt(getMountAddress(EE_maxAxis1), 3600);
  XEEPROM.writeInt(getMountAddress(EE_minAxis2), 3600);
  XEEPROM.writeInt(getMountAddress(EE_maxAxis2), 3600);
}

void initLimitMinAxis1()
{
  int val = XEEPROM.readInt(getMountAddress(EE_minAxis1));
  int maxval = 10*abs(geoA1.LimMinAxis);
  if (val < 0 || val > maxval)
  {
    val = maxval;
    XEEPROM.writeInt(getMountAddress(EE_minAxis1), val);
  }
  geoA1.minAxis = -val * geoA1.stepsPerDegree / 10.0;
}
void initLimitMaxAxis1()
{
  int val = XEEPROM.readInt(getMountAddress(EE_maxAxis1));
  int maxval = 10*abs(geoA1.LimMaxAxis);
  if (val < 0 || val > maxval)
  {
    val = maxval;
    XEEPROM.writeInt(getMountAddress(EE_maxAxis1), val);
  }
  geoA1.maxAxis = val * geoA1.stepsPerDegree / 10.0;
}
void initLimitMinAxis2()
{
  int val = XEEPROM.readInt(getMountAddress(EE_minAxis2));
  int maxval = 10*abs(geoA2.LimMinAxis);
  if (val < 0 || val > maxval)
  {
    val = maxval;
    XEEPROM.writeInt(getMountAddress(EE_minAxis2), val);
  }
  geoA2.minAxis = -val * geoA2.stepsPerDegree / 10.0;
}
void initLimitMaxAxis2()
{
  int val = XEEPROM.readInt(getMountAddress(EE_maxAxis2));
  int maxval = 10*abs(geoA2.LimMaxAxis);
  if (val < 0 || val > maxval)
  {
    val = maxval;
    XEEPROM.writeInt(getMountAddress(EE_maxAxis2), val);
  }
  geoA2.maxAxis = val * geoA2.stepsPerDegree / 10.0;
}

