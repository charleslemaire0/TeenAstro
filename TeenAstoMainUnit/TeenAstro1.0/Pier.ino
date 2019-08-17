// set Side of pier
//boolean setSide(PierSide side)
//{
//  if (side == PIER_NOTVALID)
//    return false;
//  double  HA, Dec;
//  GetEqu(localSite.latitude(), &HA, &Dec);
//  double  axis1, axis2;
//  pierSide = side;
//  EquToInstr(localSite.latitude(), HA, Dec, &axis1, &axis2);
//  cli();
//  posAxis1 = axis1;
//  posAxis2 = axis2;
//  sei();
//  return true;
//}

  bool checkPole(const double& HA, const PierSide& inputSide, CheckMode mode)
{
  bool ok = true;
  double underPoleLimit = (mode == CHECKMODE_GOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0/60;
  switch (inputSide)
  {
  case PIER_WEST:
    if (HA < -underPoleLimit * 15.) ok = false;
    break;
  case PIER_EAST:
    if (HA > underPoleLimit * 15.) ok = false;
    break;
  default:
    ok = false;
    break;
  }
  return ok;
}



bool checkMeridian(const double& HA, const PierSide& inputSide, CheckMode mode)
{
  bool ok = true;
  double MinutesPastMeridianW = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOW : minutesPastMeridianGOTOW + 5;
  double MinutesPastMeridianE = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOE : minutesPastMeridianGOTOE + 5;
  
  switch (inputSide)
  {
  case PIER_WEST:
    if (HA * 60. > MinutesPastMeridianW * 15.) ok = false;
    break;
  case PIER_EAST:
    if (HA * 60. < -MinutesPastMeridianE *15.) ok = false;
    break;
  default:
    ok = false;
    break;
  }
  return ok;
}

//only for Equatorial Fork
boolean checkDeclinatioLimit()
{
  static double hh;
  static double dd;
  if (mountType != MOUNT_TYPE_FORK)
    return true;
  if (pierSide == PIER_WEST)
    return false;
  getEqu(&hh, &dd, true);
  return dd > MinDec && dd < MaxDec;
}

bool Azok(long pos)
{
  return pos < (long)((360 + DegreePastAZ) * StepsPerDegreeAxis1) && pos >= long(-DegreePastAZ * StepsPerDegreeAxis1);
}
bool Altok(long pos)
{
  return  abs(pos) < (long)StepsPerDegreeAxis2 * 180L ;
}

bool checkAzimuth()
{
  static long azm;
  cli();
  azm = posAxis1;
  sei();
  return Azok(azm);
}
// Predict Side of Pier
// return 0 if no side can reach the given position
PierSide predictSideOfPier(const double& HA, const PierSide& inputSide)
{
  if (meridianFlip == FLIP_NEVER)
    return PIER_EAST;
  if (checkPole(HA, inputSide, CHECKMODE_GOTO) && checkMeridian(HA, inputSide, CHECKMODE_GOTO))
  {
    //Serial.println(inputSide);
    return inputSide;
  }
  PierSide otherside;
  if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;

  if (checkPole(HA, otherside, CHECKMODE_GOTO) && checkMeridian(HA, otherside, CHECKMODE_GOTO))
  {
    //Serial.println(otherside);
    return otherside;
  }
  //Serial.println(0);
  return PIER_NOTVALID;
}
