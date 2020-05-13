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
  return -GA2.quaterRot <= axis2 && axis2 <= GA2.quaterRot ? PIER_EAST : PIER_WEST;
}

//for GEM
bool checkPole(const long &axis1, CheckMode mode)
{
  double underPoleLimit = (mode == CHECKMODE_GOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0 / 60;
  return (axis1 > GA1.quaterRot - underPoleLimit * 15. * GA1.stepsPerDegree) && (axis1 < GA1.quaterRot + underPoleLimit * 15. * GA1.stepsPerDegree);
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
    if (axis1 > (12. + MinutesPastMeridianW / 60.)* 15.0 * GA1.stepsPerDegree) ok = false;
    break;
  case PIER_EAST:
    if (axis1 < -MinutesPastMeridianE / 60. * 15.0 * GA1.stepsPerDegree) ok = false;
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
  return axis2 > MinAxis2EQ * GA2.stepsPerDegree && axis2 < MaxAxis2EQ * GA2.stepsPerDegree;
}

//for az alt
bool checkAxis2LimitAZALT(const long &axis2)
{
  return axis2 > MinAxis2AZALT * GA2.stepsPerDegree && axis2 < MaxAxis2AZALT * GA2.stepsPerDegree;
}
bool checkAxis1LimitAZALT(const long &axis1)
{
  return axis1 < (long)((180 + DegreePastAZ) * GA1.stepsPerDegree) && axis1 >= long((-180 - DegreePastAZ) * GA1.stepsPerDegree);
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