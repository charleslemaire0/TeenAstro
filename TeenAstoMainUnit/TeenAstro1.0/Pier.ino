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





// Predict Side of Pier
// return 0 if no side can reach the given position
PierSide predictSideOfPier(const double& HA, const double& Dec,const PierSide& inputSide)
{

  instrumental_stepper pos;
  EquToStep(HA,Dec, inputSide, &pos.m_axis1, &pos.m_axis2);
  if (pos.withinLimit())
  {
    return inputSide;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    PierSide otherside;
    if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;
    EquToStep(HA, Dec, otherside, &pos.m_axis1, &pos.m_axis2);
    if (pos.withinLimit())
    {
      return otherside;
    }
  }
 return  PIER_NOTVALID;
}
