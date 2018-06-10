// set Side of pier
boolean setSide(byte side)
{
  double  HA, Dec;
  GeoAlign.GetEqu(localSite.latitude(), &HA, &Dec);
  double  axis1, axis2;
  if (side == PierSideEast)
  {
    pierSide = PierSideEast;
    DecDir = DecDirEInit;
  }
  else if (side == PierSideWest)
  {
    pierSide = PierSideWest;
    DecDir = DecDirWInit;
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
  case PierSideWest:
    if (HA < -underPoleLimit * 15.) ok = false;
    break;
  case PierSideEast:
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
  case PierSideWest:
    if (HA * 60 > MinutesPastMeridianW * 15.) ok = false;
    break;
  case PierSideEast:
    if (HA * 60 < -MinutesPastMeridianE *15.) ok = false;
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
    return inputSide;
  if (checkPole(HA, inputSide, CheckModeGOTO) && checkMeridian(HA, inputSide, CheckModeGOTO))
  {
    //Serial.println(inputSide);
    return inputSide;
  }
  byte otherside;
  if (inputSide == PierSideEast) otherside = PierSideWest; else otherside = PierSideEast;

  if (checkPole(HA, otherside, CheckModeGOTO) && checkMeridian(HA, otherside, CheckModeGOTO))
  {
    //Serial.println(otherside);
    return otherside;
  }
  //Serial.println(0);
  return 0;
}