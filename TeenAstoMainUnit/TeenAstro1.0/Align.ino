



//class instrumental_angle
//{
//  double m_axis1;
//  double m_axis2;
//public:
//  instrumental_angle(const double &axis1, const double &axis2)
//  {
//    m_axis1 = axis1;
//    m_axis2 = axis2;
//  }
//  PierSide getPierSide()
//  {
//     return -quaterRotAxis2 <= m_axis2 * StepsPerDegreeAxis2 && m_axis2 <= quaterRotAxis2 * StepsPerDegreeAxis2 ? PIER_EAST : PIER_WEST;
//  }
//};


class AzAlt
{
  double Az;
  double Alt;
  PierSide Side;
};

class HADec
{
  double Ha;
  double Dec;
  PierSide Side;
};




void StepToInstr(long Axis1, long Axis2, double *AngleAxis1, double *AngleAxis2, PierSide* Side)
{
  *AngleAxis1 = ((double)Axis1) / StepsPerDegreeAxis1;
  *AngleAxis2 = ((double)Axis2) / StepsPerDegreeAxis2;
  InsrtAngle2Angle(AngleAxis1, AngleAxis2, Side);
}

void InstrtoStep(double AngleAxis1, double AngleAxis2, PierSide Side, long *Axis1, long *Axis2)
{
  Angle2InsrtAngle(Side, &AngleAxis1, &AngleAxis2);
  *Axis1 = (long)(AngleAxis1 * StepsPerDegreeAxis1);
  *Axis2 = (long)(AngleAxis2 * StepsPerDegreeAxis2);
}

void EquToStep(double HA, double Dec, PierSide Side, long *Axis1, long *Axis2)
{
  InstrtoStep(HA, Dec, Side, Axis1, Axis2);
}

void StepToEqu(long Axis1, long Axis2, double *HA, double *Dec, PierSide *Side )
{
  StepToInstr( Axis1,  Axis2, HA, Dec, Side);
  CorrectHADec(HA, Dec);
}


PierSide GetPierSide()
{
  cli(); long pos = posAxis2; sei();
  return -quaterRotAxis2 <= pos && pos <= quaterRotAxis2 ? PIER_EAST : PIER_WEST;
}

