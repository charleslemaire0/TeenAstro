
// -----------------------------------------------------------------------------------
// GEOMETRIC ALIGN FOR EQUATORIAL MOUNTS

//


// takes the topocentric refracted coordinates and applies corrections to arrive at instrument equatorial coordinates
void EquToInstr(const double *Lat, double HA, double Dec, double *HA1,
  double *Dec1)
{

    // just ignore the the correction if right on the pole
    *HA1 = HA;
    *Dec1 = Dec;
  
  CorrectHADec(HA1, Dec1);
}

// takes the instrument equatorial coordinates and applies corrections to arrive at topocentric refracted coordinates
void InstrToEqu(const double *Lat, double HA, double Dec, double *HA1,
  double *Dec1)
{


    // just ignore the the correction if right on the pole
    *HA1 = HA;
    *Dec1 = Dec;
  
  CorrectHADec(HA1, Dec1);
}


void StepToEqu(const double *Lat, long Axis1, long Axis2, double *HA1, double *Dec1)
{
  double HA = ((double)Axis1) / StepsPerDegreeAxis1;
  double Dec = ((double)Axis2) / StepsPerDegreeAxis2;
  InstrToEqu(Lat, HA, Dec, HA1, Dec1);
}

void StepToInstr(long Axis1, long Axis2, double *AngleAxis1, double *AngleAxis2)
{
  *AngleAxis1 = ((double)Axis1) / StepsPerDegreeAxis1;
  *AngleAxis2 = ((double)Axis2) / StepsPerDegreeAxis2;
  InsrtAngle2Angle(AngleAxis1, AngleAxis2, &pierSide);
}

void InstrtoStep(double AngleAxis1, double AngleAxis2, long *Axis1, long *Axis2)
{
  Angle2InsrtAngle(&AngleAxis1, &AngleAxis2, &pierSide);
  *Axis1 = (long)(AngleAxis1 * StepsPerDegreeAxis1);
  *Axis2 = (long)(AngleAxis2 * StepsPerDegreeAxis2);
}


void EquToStep(const double *Lat, double HA, double Dec, long *Axis1, long *Axis2)
{
  double HA1;
  double Dec1;
  EquToInstr(Lat, HA, Dec, &HA1, &Dec1);
  InstrtoStep(HA1, Dec1, Axis1, Axis2);
}

void GetInstr(double *HA, double *Dec)
{
  cli();
  long axis1 = posAxis1;
  long axis2 = posAxis2;
  sei();
  StepToInstr(axis1, axis2, HA, Dec);
}

void GetEqu(const double *Lat, double *HA, double *Dec)
{
  cli();
  long axis1 = posAxis1;
  long axis2 = posAxis2;
  sei();
  StepToEqu(Lat, axis1, axis2, HA, Dec);
}
