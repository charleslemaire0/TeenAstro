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
  return true;
}
bool checkMeridian(const long &axis1, const long &axis2, CheckMode mode)
{
  bool ok = true; 
  double _ra, _dec, MinutesPastMeridianW, MinutesPastMeridianE;

  if (mode == CHECKMODE_GOTO)
  {
   MinutesPastMeridianW = minutesPastMeridianGOTOW;
   MinutesPastMeridianE = minutesPastMeridianGOTOE;
  }
  else
  {
    getEqu(&_ra, &_dec, localSite.cosLat(), localSite.sinLat(), false);
    if (_dec < maxDecToKeepTrackingOn)
    {
      MinutesPastMeridianW = 360;
      MinutesPastMeridianE = 360;
    }
    else
    {
      MinutesPastMeridianW = minutesPastMeridianGOTOW + 5;
      MinutesPastMeridianE = minutesPastMeridianGOTOE + 5;
    }
  }
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

//for Eq Fork only
bool checkAxis2LimitEQ(const long &axis2)
{
  return axis2 > MinAxis2EQ * geoA2.stepsPerDegree && axis2 < MaxAxis2EQ * geoA2.stepsPerDegree;
}

//for az alt
bool checkAxis2LimitAZALT(const long &axis2)
{
  return axis2 > MinAxis2AZALT * geoA2.stepsPerDegree && axis2 < MaxAxis2AZALT * geoA2.stepsPerDegree;
}
bool checkAxis1LimitAZALT(const long &axis1)
{
  return axis1 < (long)((180 + DegreePastAZ) * geoA1.stepsPerDegree) && axis1 >= long((-180 - DegreePastAZ) * geoA1.stepsPerDegree);
}

// check if defined position is within limit
bool withinLimit(const long &axis1, const long &axis2)
{
  bool ok = false;
  if (isAltAZ())
  {
    ok = checkAxis1LimitAZALT(axis1) && checkAxis2LimitAZALT(axis2);
  }
  else
  {
    ok = checkPole(axis1, CHECKMODE_GOTO) && checkAxis2LimitEQ(axis2);
    if (!ok)
      return ok;
    if (meridianFlip == FLIP_ALWAYS)
      ok = checkMeridian(axis1, axis2, CHECKMODE_GOTO);
  }
  return ok;
}