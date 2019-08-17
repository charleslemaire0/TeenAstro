
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
  CorrectHADec(&HA, &Dec);
  InstrtoStep(HA, Dec, Side, Axis1, Axis2);
}

void StepToEqu(long Axis1, long Axis2, double *HA, double *Dec, PierSide *Side )
{
  StepToInstr( Axis1,  Axis2, HA, Dec, Side);
  CorrectHADec(HA, Dec);
}


void GetInstr(double *HA, double *Dec, PierSide* Side)
{
  cli();
  long axis1 = posAxis1;
  long axis2 = posAxis2;
  sei();
  StepToInstr(axis1, axis2, HA, Dec, Side);
}

PierSide GetPierSide()
{
  cli(); long pos = posAxis2; sei();
  return -quaterRotAxis2 <= pos && pos <= quaterRotAxis2 ? PIER_EAST : PIER_WEST;
}


