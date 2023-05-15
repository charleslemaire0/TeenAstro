
byte PushToEqu(Coord_EQ EQ_T, PierSide preferedPierSide, double Lat, float* deltaA1, float* deltaA2)
{
  return PushToHor(EQ_T.To_Coord_HO(Lat, RefrOptForGoto()), preferedPierSide, deltaA1, deltaA2);
}

byte PushToHor(Coord_HO HO_T, PierSide preferedPierSide, float* deltaA1, float* deltaA2)
{
  double Axis1_target, Axis2_target = 0;
  long axis1_target, axis2_target = 0;
  PierSide selectedSide = PierSide::PIER_NOTVALID;


  if (HO_T.Alt() < minAlt * DEG_TO_RAD) return ERRGOTO_BELOWHORIZON;   // fail, below min altitude
  if (HO_T.Alt() > maxAlt * DEG_TO_RAD) return ERRGOTO_ABOVEOVERHEAD;   // fail, above max altitude

  Coord_IN instr_T = HO_T.To_Coord_IN(alignment.Tinv);
  Axis1_target = instr_T.Axis1() * RAD_TO_DEG;
  Axis2_target = instr_T.Axis2() * RAD_TO_DEG;

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

