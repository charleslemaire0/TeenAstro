
byte PushToEqu(const double Ra, const double Dec, PierSide preferedPierSide, const double* cosLat, const double* sinLat,
  float* deltaA1, float* deltaA2)
{
  double azm, alt = 0;
  double Ha = haRange(rtk.LST() * 15.0 - Ra);
  EquToHor(Ha, Dec, doesRefraction.forGoto, &azm, &alt, cosLat, sinLat);
  return PushToHor(&azm, &alt, preferedPierSide, deltaA1, deltaA2);
}

byte PushToHor(const double* Azm, const double* Alt, PierSide preferedPierSide, float* deltaA1, float* deltaA2)
{
  double Axis1_target, Axis2_target = 0;
  long axis1_target, axis2_target = 0;
  PierSide selectedSide = PierSide::PIER_NOTVALID;

  if (*Alt < minAlt) return ERRGOTO_BELOWHORIZON;   // fail, below min altitude
  if (*Alt > maxAlt) return ERRGOTO_ABOVEOVERHEAD;   // fail, above max altitude

  alignment.toAxisDeg(Axis1_target, Axis2_target, *Azm, *Alt);
  if (!predictTarget(Axis1_target, Axis2_target, preferedPierSide,
    axis1_target, axis2_target, selectedSide))
  {
    return ERRGOTO_LIMITS; //fail, outside limit
  }
  Axis1_target = axis1_target / geoA1.stepsPerDegree;
  Axis2_target = axis2_target / geoA2.stepsPerDegree;
  *deltaA1 = encoderA1.deltaTarget(Axis1_target);
  *deltaA2 = encoderA2.deltaTarget(Axis2_target);
  return ERRGOTO_NONE;
}

