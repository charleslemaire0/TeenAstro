void StepToInstr(long Axis1, long Axis2, double *AngleAxis1, double *AngleAxis2, PierSide* Side)
{
  *AngleAxis1 = ((double)Axis1) / geoA1.stepsPerDegree;
  *AngleAxis2 = ((double)Axis2) / geoA2.stepsPerDegree;
  InsrtAngle2Angle(AngleAxis1, AngleAxis2, Side);
}

void InstrtoStep(double AngleAxis1, double AngleAxis2, PierSide Side, long *Axis1, long *Axis2)
{
  Angle2InsrtAngle(Side, &AngleAxis1, &AngleAxis2, localSite.latitude());
  *Axis1 = (long)(AngleAxis1 * geoA1.stepsPerDegree);
  *Axis2 = (long)(AngleAxis2 * geoA2.stepsPerDegree);
}



//--------------------------------------------------------------------------------------------------
// GoTo, commands to move the telescope to an location or to report the current location
bool syncEqu(double HA, double Dec, PierSide Side, const double *cosLat, const double *sinLat)
{
  double Azm, Alt = 0;
  EquToHor(HA, Dec, doesRefraction.forGoto, &Azm, &Alt, cosLat, sinLat);
  return syncAzAlt(Azm, Alt, Side);
}
// syncs the telescope/mount to the sky
bool syncAzAlt(double Azm, double Alt, PierSide Side)
{
  long axis1, axis2 = 0;
  double Axis1, Axis2 = 0;
  alignment.toInstrumentalDeg(Axis1, Axis2, Azm, Alt);
  InstrtoStep(Axis1, Axis2, Side, &axis1, &axis2);
  cli();
  staA1.pos = axis1;
  staA2.pos = axis2;
  staA1.target = staA1.pos;
  staA2.target = staA2.pos;
  sei();
  atHome = false;
  return true;
}

// gets the telescopes current Topocentric RA and Dec, set returnHA to true for Horizon Angle instead of RA
bool getEqu(double *HA, double *Dec, const double *cosLat, const double *sinLat, bool returnHA)
{
  double  azm, alt = 0;
  getHorApp(&azm, &alt);
  if (doesRefraction.forGoto)
  {
    HorAppToEqu(azm, alt, HA, Dec, cosLat, sinLat);
  }
  else
  {
    HorTopoToEqu(azm, alt, HA, Dec, cosLat, sinLat);
  }
  if (!returnHA)
  {
    *HA = degRange(rtk.LST() * 15.0 - *HA);
  }
  return true;
}

bool getEquE(double* HA, double* Dec, const double* cosLat, const double* sinLat, bool returnHA)
{
  double  azm, alt = 0;
  getHorAppE(&azm, &alt);
  if (doesRefraction.forGoto)
  {
    HorAppToEqu(azm, alt, HA, Dec, cosLat, sinLat);
  }
  else
  {
    HorTopoToEqu(azm, alt, HA, Dec, cosLat, sinLat);
  }
  if (!returnHA)
  {
    *HA = degRange(rtk.LST() * 15.0 - *HA);
  }
  return true;
}


// gets the telescopes current Topocentric Target RA and Dec, set returnHA to true for Horizon Angle instead of RA
bool getEquTarget(double *HA, double *Dec, const double *cosLat, const double *sinLat, bool returnHA)
{
  double  azm, alt = 0;
  getHorAppTarget(&azm, &alt);
  if (doesRefraction.forGoto)
  {
    HorAppToEqu(azm, alt, HA, Dec, cosLat, sinLat);
  }
  else
  {
    HorTopoToEqu(azm, alt, HA, Dec, cosLat, sinLat);
  }
  if (!returnHA)
  {
    *HA = degRange(rtk.LST() * 15.0 - *HA);
  }
  return true;
}

// gets the telescopes current Apparent Alt and Azm!
bool getHorApp(double *Azm, double *Alt)
{
  cli();
  double Axis1 = staA1.pos / (double)geoA1.stepsPerDegree;
  double Axis2 = staA2.pos / (double)geoA2.stepsPerDegree;
  sei();
  alignment.toReferenceDeg(*Azm, *Alt, Axis1, Axis2);
  return true;
}

bool getHorAppE(double* Azm, double* Alt)
{
  double Axis1, Axis2;
  encoderA1.r_deg(Axis1);
  encoderA2.r_deg(Axis2);
  alignment.toReferenceDeg(*Azm, *Alt, Axis1, Axis2);
  return true;
}


// gets the telescopes current Apparent Target Alt and Azm!
bool getHorAppTarget(double *Azm, double *Alt)
{
  cli();
  double Axis1 = staA1.target / geoA1.stepsPerDegree;
  double Axis2 = staA2.target / geoA2.stepsPerDegree;
  sei();
  alignment.toReferenceDeg(*Azm, *Alt, Axis1, Axis2);
  return true;
}

// moves the mount to a new Right Ascension and Declination (RA,Dec) in degrees
byte goToEqu(double HA, double Dec, PierSide preferedPierSide, const double *cosLat, const double *sinLat)
{
  double azm, alt = 0;
  EquToHor(HA, Dec, doesRefraction.forGoto,&azm, &alt, cosLat, sinLat);
  return goToHor(&azm, &alt, preferedPierSide);
}
// moves the mount to a new Altitude and Azmiuth (Alt,Azm) in degrees
byte goToHor(double *Azm, double *Alt, PierSide preferedPierSide)
{
  double Axis1_target, Axis2_target = 0;
  long axis1_target, axis2_target = 0;
  PierSide selectedSide = PierSide::PIER_NOTVALID;

  if (*Alt < minAlt) return ERRGOTO_BELOWHORIZON;   // fail, below min altitude
  if (*Alt > maxAlt) return ERRGOTO_ABOVEOVERHEAD;   // fail, above max altitude

  alignment.toInstrumentalDeg(Axis1_target, Axis2_target, *Azm, *Alt);
  if (!predictTarget(Axis1_target, Axis2_target, preferedPierSide,
    axis1_target, axis2_target, selectedSide))
  {
    return ERRGOTO_LIMITS; //fail, outside limit
  }

  return goTo(axis1_target, axis2_target);
}

// Predict Target
// return 0 if no side can reach the given position
bool predictTarget(const double& Axis1_in, const double& Axis2_in, const PierSide& inputSide,
                           long& Axis1_out, long& Axis2_out, PierSide& outputSide)
{
  double Axis1 = Axis1_in;
  double Axis2 = Axis2_in;
  outputSide = inputSide;
  Angle2InsrtAngle(outputSide, &Axis1, &Axis2, localSite.latitude());
  Axis1_out = Axis1 * geoA1.stepsPerDegree;
  Axis2_out = Axis2 * geoA2.stepsPerDegree;
  if (withinLimit(Axis1_out , Axis2_out ))
  {
    return true;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    double Axis1 = Axis1_in;
    double Axis2 = Axis2_in;
    if (inputSide == PIER_EAST) outputSide = PIER_WEST; else outputSide = PIER_EAST;
    Angle2InsrtAngle(outputSide, &Axis1, &Axis2, localSite.latitude());
    Axis1_out = Axis1 * geoA1.stepsPerDegree;
    Axis2_out = Axis2 * geoA2.stepsPerDegree;
    if (withinLimit(Axis1_out, Axis2_out))
    {
      return true;
    }
  }
  return  false;
}

// moves the mount to a new Hour Angle and Declination - both are in steps.  Alternate targets are used when a meridian flip occurs

ErrorsGoTo goTo(long thisTargetAxis1, long thisTargetAxis2)
{
  // HA goes from +90...0..-90
  //                W   .   E

  if (movingTo)
  {
    abortSlew = true;
    return ErrorsGoTo::ERRGOTO_SLEWING;
  }   // fail, prior goto cancelled
  if (guideA1.isBusy() || guideA2.isBusy()) return ErrorsGoTo::ERRGOTO_GUIDINGBUSY;   // fail, unspecified error

  //z = AzRange(z);
  // Check to see if this goto is valid
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return ErrorsGoTo::ERRGOTO_PARKED; // fail, PRK_PARKED
  if (lastError != ERRT_NONE)
  {
    return static_cast<ErrorsGoTo>(lastError + 10);   // fail, telescop has Errors State
  }
  if (staA1.fault || staA2.fault) return ErrorsGoTo::ERRGOTO_MOTOR; // fail, unspecified error
  atHome = false;
  cli();
  movingTo = true;
  SetsiderealClockSpeed(siderealClockSpeed);

  staA1.start = staA1.pos;
  staA2.start = staA2.pos;

  staA1.target = thisTargetAxis1;
  staA2.target = thisTargetAxis2;

  staA1.resetToSidereal();
  staA2.resetToSidereal();

  sei();

  DecayModeGoto();

  return ErrorsGoTo::ERRGOTO_NONE;
}


ErrorsGoTo Flip()
{
  long axis1Flip, axis2Flip;
  PierSide selectedSide = PIER_NOTVALID;
  PierSide preferedPierSide = (GetPierSide() == PIER_EAST) ? PIER_WEST : PIER_EAST;
  cli();
  double Axis1 = staA1.pos / (double)geoA1.stepsPerDegree;
  double Axis2 = staA2.pos / (double)geoA2.stepsPerDegree;
  sei();
  if (!predictTarget(Axis1, Axis2, preferedPierSide, axis1Flip, axis2Flip, selectedSide))
  {
    return ErrorsGoTo::ERRGOTO_SAMESIDE;
  }
  return goTo(axis1Flip, axis2Flip);
}

