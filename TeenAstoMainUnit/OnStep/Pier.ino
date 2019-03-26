// set Side of pier
boolean setSide(byte side)
{
  double  HA, Dec;
  GeoAlign.GetEqu(localSite.latitude(), &HA, &Dec);
  double  axis1, axis2;

  if (side == PIER_EAST)
  {
    pierSide = PIER_EAST;
  }
  else if (side == PIER_WEST)
  {
    pierSide = PIER_WEST;
  }
  else
    return false;
  GeoAlign.EquToInstr(localSite.latitude(), HA, Dec, &axis1, &axis2);
  cli();
  posAxis1 = axis1;
  posAxis2 = axis2;
  sei();
  return true;
}

  bool checkPole(const double& HA, const byte& inputSide, byte mode)
{
  bool ok = true;
  double underPoleLimit = (mode == CheckModeGOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0/60;
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

bool checkMeridian(const double& HA, const byte& inputSide, byte mode)
{
  bool ok = true;
  double MinutesPastMeridianW = (mode == CheckModeGOTO) ? minutesPastMeridianGOTOW : minutesPastMeridianGOTOW + 5;
  double MinutesPastMeridianE = (mode == CheckModeGOTO) ? minutesPastMeridianGOTOE : minutesPastMeridianGOTOE + 5;
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

// Predict Side of Pier
// return 0 if no side can reach the given position
byte predictSideOfPier(const double& HA, const byte& inputSide)
{
  if (meridianFlip == MeridianFlipNever)
    return PIER_EAST;
  if (checkPole(HA, inputSide, CheckModeGOTO) && checkMeridian(HA, inputSide, CheckModeGOTO))
  {
    //Serial.println(inputSide);
    return inputSide;
  }
  byte otherside;
  if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;

  if (checkPole(HA, otherside, CheckModeGOTO) && checkMeridian(HA, otherside, CheckModeGOTO))
  {
    //Serial.println(otherside);
    return otherside;
  }
  //Serial.println(0);
  return 0;
}

byte predictTargetSideOfPier(double RaObject, double DecObject)
{
  double  HA = haRange(rtk.LST() * 15.0 - RaObject);
  double h, d;
  GeoAlign.EquToInstr(localSite.latitude(), HA, DecObject, &h, &d);
  return predictSideOfPier(h, pierSide);
}