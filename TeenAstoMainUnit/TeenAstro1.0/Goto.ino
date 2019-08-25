//--------------------------------------------------------------------------------------------------
// GoTo, commands to move the telescope to an location or to report the current location
//#define OLD_INTERFACE
#if defined OLD_INTERFACE
// syncs the telescope/mount to the sky
boolean syncEqu(double HA, double Dec, PierSide Side)
{
  long axis1, axis2;
  // correct for polar misalignment only by clearing the index offsets

  if (isAltAZ())
  {
    double Axis1, Axis2;
    EquToHor(HA, Dec, &Axis2, &Axis1);
    while (Axis1 > 180.0) Axis1 -= 360.0;
    while (Axis1 < -180.0) Axis1 += 360.0;
    axis1 = Axis1 * StepsPerDegreeAxis1;
    axis2 = Axis2 * StepsPerDegreeAxis2;
  }
  else
  {
    EquToStep(HA, Dec, Side, &axis1, &axis2);
  }
  cli();
  posAxis1 = axis1;
  posAxis2 = axis2;
  targetAxis1.part.m = axis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = axis2;
  targetAxis2.part.f = 0;
  sei();
  atHome = false;
  return true;
}
// syncs the telescope/mount to the sky
boolean syncAltAz(double Az, double Alt, PierSide Side)
{
  // hour angleTrackingMoveTo

  long axis1, axis2;
  // correct for polar misalignment only by clearing the index offsets

  if (isAltAZ())
  {
    axis1 = Az*StepsPerDegreeAxis1;
    axis2 = Alt*StepsPerDegreeAxis2;
  }
  else
  {
    double Ha, Dec;
    HorToEqu(Alt, Az, &Ha, &Dec);
    EquToStep(Ha, Dec, Side, &axis1, &axis2);
  }
  cli();
  posAxis1 = axis1;
  posAxis2 = axis2;
  targetAxis1.part.m = axis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = axis2;
  targetAxis2.part.f = 0;
  sei();
  atHome = false;
  return true;
}
// gets the telescopes current RA and Dec, set returnHA to true for Horizon Angle instead of RA
boolean getEqu(double *HA, double *Dec, boolean returnHA)
{
  //get HA
  cli();
  double Axis1 = posAxis1;
  double Axis2 = posAxis2;
  sei();

  if (isAltAZ())
  {
    // get the hour angle (or Azm)
    double z = Axis1 / (double)StepsPerDegreeAxis1;
    // get the declination (or Alt)
        //double lat = *localSite.latitude();
    //PoleStepAxis2 = fabs(lat) *StepsPerDegreeAxis2;
    double a = Axis2 / (double)StepsPerDegreeAxis2;

    HorToEqu(a, z, HA, Dec); // convert from Alt/Azm to HA/Dec
  }
  else
  {
    PierSide Side;
    StepToEqu(Axis1, Axis2, HA,Dec, &Side );
  }
  // return either the RA or the HA depending on returnHA
  if (!returnHA)
  {
    *HA = degRange(rtk.LST() * 15.0 - *HA);
  }
  return true;
}
// gets the telescopes current Alt and Azm
boolean getHor(double *Alt, double *Azm)
{
  double  h, d;
  getEqu(&h, &d, true);
  EquToHor(h, d, Alt, Azm);
  return true;
}


// moves the mount to a new Right Ascension and Declination (RA,Dec) in degrees
byte goToEqu(double HA, double Dec, PierSide preferedPierSide)
{
  double  a, z;
  long axis1_target, axis2_target;
  EquToHor(HA, Dec, &a, &z);
  z = AzRange(z);
  // Check to see if this goto is valid
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return 4; // fail, PRK_PARKED
  if (lastError != ERR_NONE) return lastError + 10;   // fail, telescop has Errors State
  if (a < minAlt) return 1;   // fail, below horizon
  if (a > maxAlt) return 6;   // fail, outside limits
  //if (Dec > MaxDec) return 6; // fail, outside limits
  //if (Dec < MinDec) return 6; // fail, outside limits
  if (movingTo)
  {
    abortSlew = true;
    return 5;
  }   // fail, prior goto cancelled

  if (guideDirAxis1 || guideDirAxis2) return 7;   // fail, unspecified error
  if (isAltAZ())
  {
    z = predictAzimuth(z);
    axis1_target = z * StepsPerDegreeAxis1;
    axis2_target = a * StepsPerDegreeAxis2;
  }
  else
  {
    // correct for polar offset, refraction, coordinate systems, operation past pole, etc. as required
    PierSide side = predictSideOfPier(HA, Dec, preferedPierSide);
    if (side == 0)  return 6; //fail, outside limit
    EquToStep(HA, Dec, side, &axis1_target, &axis2_target);
  }
  return goTo(axis1_target, axis2_target);
}


// moves the mount to a new Altitude and Azmiuth (Alt,Azm) in degrees
byte goToHor(double *Alt, double *Azm)
{
  double HA, Dec;
  HorToEqu(*Alt, *Azm, &HA, &Dec);
  return goToEqu(HA, Dec, GetPierSide());
}




// Predict Side of Pier
// return 0 if no side can reach the given position
PierSide predictSideOfPier(const double& HA, const double& Dec,const PierSide& inputSide)
{

  long axis1,axis2;
  EquToStep(HA,Dec, inputSide, &axis1, &axis2);
  if (withinLimit(axis1, axis2))
  {
    return inputSide;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    PierSide otherside;
    if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;
    EquToStep(HA, Dec, otherside, &axis1, &axis2);
    if (withinLimit(axis1, axis2))
    {
      return otherside;
    }
  }
 return  PIER_NOTVALID;
}

double predictAzimuth(double z)
{
  if (DegreePastAZ > 0)
  {
    long axis1_t, axis2_t;
    setAtMount(axis1_t, axis2_t);
    double zt = axis1_t / StepsPerDegreeAxis1;
    //compute alternative position
    double z1 = z + 360;
    double z2 = z - 360;
    if (checkAxis1LimitAZALT(z1*StepsPerDegreeAxis1) && dist(zt, z) > dist(zt, z1))
      z = z1;
    else if (checkAxis1LimitAZALT(z2*StepsPerDegreeAxis1) && dist(zt, z) > dist(zt, z2))
      z = z2;
  }
  return z;
}
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
#else
boolean syncEqu(double HA, double Dec, PierSide Side)
{ 
  long axis1, axis2 = 0;
  double Axis1, Axis2 = 0;
  alignment.toInstrumentalDeg(Axis1, Axis2, HA, Dec);
  Angle2InsrtAngle(Side, &Axis1, &Axis2);
  InstrtoStep(Axis1, Axis2, Side, &axis1, &axis2);
  cli();
  posAxis1 = axis1;
  posAxis2 = axis2;
  targetAxis1.part.m = axis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = axis2;
  targetAxis2.part.f = 0;
  sei();
  atHome = false;
  return true;
}
// syncs the telescope/mount to the sky
boolean syncAltAz(double Az, double Alt, PierSide Side)
{
  double Ha, Dec = 0;
  HorToEqu(Alt, Az, &Ha, &Dec);
  return syncEqu(Ha, Dec, Side);
}
// gets the telescopes current RA and Dec, set returnHA to true for Horizon Angle instead of RA
boolean getEqu(double *HA, double *Dec, boolean returnHA)
{
  cli();
  double Axis1 = posAxis1/ (double)StepsPerDegreeAxis1;
  double Axis2 = posAxis2/ (double)StepsPerDegreeAxis2;
  sei();
  alignment.toReferenceDeg(*HA, *Dec, Axis1, Axis2);
  if (!returnHA)
  {
    *HA = degRange(rtk.LST() * 15.0 - *HA);
  }
  return true;
}
// gets the telescopes current Alt and Azm
boolean getHor(double *Alt, double *Azm)
{
  double  h, d;
  getEqu(&h, &d, true);
  EquToHor(h, d, Alt, Azm);
  return true;
}
// moves the mount to a new Right Ascension and Declination (RA,Dec) in degrees
byte goToEqu(double HA, double Dec, PierSide preferedPierSide)
{
  double a, z, Axis1_target,Axis2_target = 0;
  long axis1_target, axis2_target = 0;
  if (movingTo)
  {
    abortSlew = true;
    return 5;
  }   // fail, prior goto cancelled
  if (guideDirAxis1 || guideDirAxis2) return 7;   // fail, unspecified error
  EquToHor(HA, Dec, &a, &z);
  //z = AzRange(z);
  // Check to see if this goto is valid
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return 4; // fail, PRK_PARKED
  if (lastError != ERR_NONE) return lastError + 10;   // fail, telescop has Errors State
  if (a < minAlt) return 1;   // fail, below horizon
  if (a > maxAlt) return 6;   // fail, outside limits
  //if (Dec > MaxDec) return 6; // fail, outside limits
  //if (Dec < MinDec) return 6; // fail, outside limits

  alignment.toInstrumentalDeg(Axis1_target, Axis2_target, HA,Dec);
  PierSide side = predictSideOfPier(Axis1_target, Axis2_target, preferedPierSide);
  if (side == 0)  return 6; //fail, outside limit
  InstrtoStep(Axis1_target,Axis2_target,side, &axis1_target,&axis2_target);
  return goTo(axis1_target, axis2_target);
}
// moves the mount to a new Altitude and Azmiuth (Alt,Azm) in degrees
byte goToHor(double *Alt, double *Azm)
{
  double HA, Dec = 0;
  HorToEqu(*Alt, *Azm, &HA, &Dec);
  return goToEqu(HA, Dec, GetPierSide());
}

// Predict Side of Pier
// return 0 if no side can reach the given position
PierSide predictSideOfPier(const double& Axis1_target, const double& Axis2_target, const PierSide& inputSide)
{
  double Axis1 = Axis1_target;
  double Axis2 = Axis2_target;
  Angle2InsrtAngle(inputSide, &Axis1, &Axis2);
  if (withinLimit(Axis1*StepsPerDegreeAxis1, Axis2*StepsPerDegreeAxis2))
  {
    return inputSide;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    PierSide otherside;
    if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;
    Axis1 = Axis1_target;
    Axis2 = Axis2_target;
    Angle2InsrtAngle(otherside, &Axis1, &Axis2);
    if (withinLimit(Axis1*StepsPerDegreeAxis1, Axis2*StepsPerDegreeAxis2))
    {
      return otherside;
    }
  }
  return  PIER_NOTVALID;
}

#endif



// moves the mount to a new Hour Angle and Declination - both are in steps.  Alternate targets are used when a meridian flip occurs

byte goTo(long thisTargetAxis1, long thisTargetAxis2)
{
  // HA goes from +90...0..-90
  //                W   .   E
  if (faultAxis1 || faultAxis2) return 7; // fail, unspecified error
  atHome = false;
  cli();
  movingTo = true;
  SetSiderealClockRate(siderealInterval);

  startAxis1 = posAxis1;
  startAxis2 = posAxis2;

  targetAxis1.part.m = thisTargetAxis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = thisTargetAxis2;
  targetAxis2.part.f = 0;

  timerRateAxis1 = SiderealRate;
  timerRateAxis2 = SiderealRate;
  sei();

  DecayModeGoto();

  return 0;
}





