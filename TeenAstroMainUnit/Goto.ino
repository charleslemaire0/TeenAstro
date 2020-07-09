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
  EquToHorApp(HA, Dec, &Azm, &Alt, cosLat, sinLat);
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
  HorAppToEqu(azm, alt, HA, Dec, cosLat, sinLat);
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
  HorAppToEqu(azm, alt, HA, Dec, cosLat, sinLat);
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
  EquToHorApp(HA, Dec, &azm, &alt, cosLat, sinLat);
  return goToHor(&azm, &alt, preferedPierSide);
}
// moves the mount to a new Altitude and Azmiuth (Alt,Azm) in degrees
byte goToHor(double *Azm, double *Alt, PierSide preferedPierSide)
{
  double Axis1_target, Axis2_target = 0;
  long axis1_target, axis2_target = 0;
  if (movingTo)
  {
    abortSlew = true;
    return 5;
  }   // fail, prior goto cancelled
  if (guideA1.dir || guideA2.dir) return 7;   // fail, unspecified error

  //z = AzRange(z);
  // Check to see if this goto is valid
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return 4; // fail, PRK_PARKED
  if (lastError != ERR_NONE) return lastError + 10;   // fail, telescop has Errors State
  if (*Alt < minAlt) return 1;   // fail, below min altitude
  if (*Alt > maxAlt) return 8;   // fail, above max altitude


  alignment.toInstrumentalDeg(Axis1_target, Axis2_target, *Azm, *Alt);
  PierSide side = predictSideOfPier(Axis1_target, Axis2_target, preferedPierSide);
  if (side == 0)  return 6; //fail, outside limit
  InstrtoStep(Axis1_target, Axis2_target, side, &axis1_target, &axis2_target);
  return goTo(axis1_target, axis2_target);
}

// Predict Side of Pier
// return 0 if no side can reach the given position
PierSide predictSideOfPier(const double& Axis1_target, const double& Axis2_target, const PierSide& inputSide)
{
  double Axis1 = Axis1_target;
  double Axis2 = Axis2_target;
  Angle2InsrtAngle(inputSide, &Axis1, &Axis2, localSite.latitude());
  if (withinLimit(Axis1*geoA1.stepsPerDegree, Axis2*geoA2.stepsPerDegree))
  {
    return inputSide;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    PierSide otherside;
    if (inputSide == PIER_EAST) otherside = PIER_WEST; else otherside = PIER_EAST;
    Axis1 = Axis1_target;
    Axis2 = Axis2_target;
    Angle2InsrtAngle(otherside, &Axis1, &Axis2, localSite.latitude());
    if (withinLimit(Axis1*geoA1.stepsPerDegree, Axis2*geoA2.stepsPerDegree))
    {
      return otherside;
    }
  }
  return  PIER_NOTVALID;
}

// moves the mount to a new Hour Angle and Declination - both are in steps.  Alternate targets are used when a meridian flip occurs

byte goTo(long thisTargetAxis1, long thisTargetAxis2)
{
  // HA goes from +90...0..-90
  //                W   .   E
  if (staA1.fault || staA2.fault) return 7; // fail, unspecified error
  atHome = false;
  cli();
  movingTo = true;
  SetSiderealClockRate(siderealInterval);

  staA1.start = staA1.pos;
  staA2.start = staA2.pos;

  staA1.target = thisTargetAxis1;
  staA2.target = thisTargetAxis2;

  staA1.timerRate = SiderealRate;
  staA2.timerRate = SiderealRate;
  sei();

  DecayModeGoto();

  return 0;
}
