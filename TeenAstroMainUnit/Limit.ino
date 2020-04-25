//Methode for limit checks, all checks are done in stepper cooordinates

void setAtMount(long &axis1, long &axis2)
{
  cli();
  axis1 = posAxis1;
  axis2 = posAxis2;
  sei();
}

PierSide getPierSide(const long &axis2)
{
  return -quaterRotAxis2 <= axis2 && axis2 <= quaterRotAxis2 ? PIER_EAST : PIER_WEST;
}

//for GEM
bool checkPole(const long &axis1, CheckMode mode)
{
  double underPoleLimit = (mode == CHECKMODE_GOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0 / 60;
  return (axis1 > quaterRotAxis1 - underPoleLimit * 15. * StepsPerDegreeAxis1) && (axis1 < quaterRotAxis1 + underPoleLimit * 15. * StepsPerDegreeAxis1);
  return true;
}
bool checkMeridian(const long &axis1, const long &axis2, CheckMode mode)
{
  bool ok = true;
  double MinutesPastMeridianW = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOW : minutesPastMeridianGOTOW + 5;
  double MinutesPastMeridianE = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOE : minutesPastMeridianGOTOE + 5;
  switch (getPierSide(axis2))
  {
  case PIER_WEST:
    if (axis1 > (12. + MinutesPastMeridianW / 60.)* 15.0 * StepsPerDegreeAxis1) ok = false;
    break;
  case PIER_EAST:
    if (axis1 < -MinutesPastMeridianE / 60. * 15.0 * StepsPerDegreeAxis1) ok = false;
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
  return axis2 > MinAxis2EQ * StepsPerDegreeAxis2 && axis2 < MaxAxis2EQ * StepsPerDegreeAxis2;
}

//for az alt
bool checkAxis2LimitAZALT(const long &axis2)
{
  return axis2 > MinAxis2AZALT * StepsPerDegreeAxis2 && axis2 < MaxAxis2AZALT * StepsPerDegreeAxis2;
}
bool checkAxis1LimitAZALT(const long &axis1)
{
  return axis1 < (long)((180 + DegreePastAZ) * StepsPerDegreeAxis1) && axis1 >= long((-180 - DegreePastAZ) * StepsPerDegreeAxis1);
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