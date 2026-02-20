/** Push-to: PushToEqu, PushToHor (encoder delta for alignment). */
#include "MainUnit.h"

byte PushToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat, float* deltaA1, float* deltaA2)
{
  return PushToHor(EQ_T.To_Coord_HO(Lat, mount.refrOptForGoto()), preferedPoleSide, deltaA1, deltaA2);
}

byte PushToHor(Coord_HO HO_T, PoleSide preferedPoleSide, float* deltaA1, float* deltaA2)
{
  double Axis1_target = 0.0, Axis2_target = 0.0;
  long axis1_target, axis2_target = 0;
  PoleSide selectedSide = PoleSide::POLE_NOTVALID;


  if (HO_T.Alt() < mount.limits.minAlt * DEG_TO_RAD) return ERRGOTO_BELOWHORIZON;   // fail, below min altitude
  if (HO_T.Alt() > mount.limits.maxAlt * DEG_TO_RAD) return ERRGOTO_ABOVEOVERHEAD;   // fail, above max altitude

  Coord_IN instr_T = HO_T.To_Coord_IN(mount.alignment.conv.Tinv);
  Axis1_target = instr_T.Axis1() * RAD_TO_DEG;
  Axis2_target = instr_T.Axis2() * RAD_TO_DEG;

  if (!mount.predictTarget(Axis1_target, Axis2_target, preferedPoleSide,
    axis1_target, axis2_target, selectedSide))
  {
    return ERRGOTO_LIMITS; //fail, outside limit
  }
  Axis1_target = axis1_target / mount.axes.geoA1.stepsPerDegree;
  Axis2_target = axis2_target / mount.axes.geoA2.stepsPerDegree;
  *deltaA1 = mount.motorsEncoders.encoderA1.deltaTarget(Axis1_target);
  *deltaA2 = mount.motorsEncoders.encoderA2.deltaTarget(Axis2_target);
  return ERRGOTO_NONE;
}

