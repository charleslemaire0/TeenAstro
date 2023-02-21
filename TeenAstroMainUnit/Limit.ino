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
bool checkPole(const long &axis1, CheckMode mode)
{
  double underPoleLimit = (mode == CHECKMODE_GOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0 / 60;
  return (axis1 > geoA1.quaterRot - underPoleLimit * 15. * geoA1.stepsPerDegree) && (axis1 < geoA1.quaterRot + underPoleLimit * 15. * geoA1.stepsPerDegree);
}
bool checkMeridian(const long &axis1, const long &axis2, CheckMode mode)
{
  bool ok = true;
  double MinutesPastMeridianW = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOW : minutesPastMeridianGOTOW + 5;
  double MinutesPastMeridianE = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOE : minutesPastMeridianGOTOE + 5;
  switch (getPierSide(axis2))
  {
  case PIER_WEST:
    if (axis1 > (12. + MinutesPastMeridianW / 60.)* 15.0 * geoA1.stepsPerDegree) ok = false;
    break;
  case PIER_EAST:
    if (axis1 < -MinutesPastMeridianE / 60. * 15.0 * geoA1.stepsPerDegree) ok = false;
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
      ok = checkPole(axis1, CHECKMODE_GOTO);
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

