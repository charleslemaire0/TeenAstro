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



//only for Equatorial Fork


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


bool canReachTarget(instrumental_stepper pos)
{
  bool ok = false;
  if (isAltAZ())
  {
    ok = pos.checkAxis2LimitAZALT();
  }
  else
  {
    ok = pos.checkPole(CHECKMODE_GOTO) && pos.checkAxis2LimitEQ();
    if (!ok)
      return ok;
    if (meridianFlip == FLIP_ALWAYS)
      ok = pos.checkMeridian(CHECKMODE_GOTO);
  }
  return ok;
}

PierSide predictSideOfPier(const double& HA, const double& Dec,const PierSide& inputSide)
{

  instrumental_stepper pos;
  EquToStep(HA,Dec, inputSide, &pos.m_axis1, &pos.m_axis2);
  if (canReachTarget(pos))
  {
    return inputSide;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    PierSide otherside;
    if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;
    EquToStep(HA, Dec, otherside, &pos.m_axis1, &pos.m_axis2);
    if (canReachTarget(pos))
    {
      return otherside;
    }
  }
 return  PIER_NOTVALID;
}
