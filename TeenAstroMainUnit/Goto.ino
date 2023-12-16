void StepToAngle(long Axis1, long Axis2, double* AngleAxis1, double* AngleAxis2, PoleSide* Side)
{
  if (Axis2 > geoA2.poleDef)
  {
    Axis2 = geoA2.poleDef - (Axis2 - geoA2.poleDef);
    Axis1 -= geoA1.halfRot;
    *Side = PoleSide::POLE_OVER;
  }
  else if (Axis2 < -geoA2.poleDef)
  {
    Axis2 = -geoA2.poleDef - (Axis2 + geoA2.poleDef);
    Axis1 -= geoA1.halfRot;
    *Side = PoleSide::POLE_OVER;
  }
  else
  {
    *Side = PoleSide::POLE_UNDER;
  }
  *AngleAxis1 = Axis1 / geoA1.stepsPerDegree;
  *AngleAxis2 = Axis2 / geoA2.stepsPerDegree;
}

void Angle2Step(double AngleAxis1, double AngleAxis2, PoleSide Side, long* Axis1, long* Axis2)
{
  *Axis1 = (long)(AngleAxis1 * geoA1.stepsPerDegree);
  *Axis2 = (long)(AngleAxis2 * geoA2.stepsPerDegree);
  if (Side >= POLE_OVER)
  {
    *Axis2 = geoA2.poleDef - (*Axis2 - geoA2.poleDef);
    *Axis1 += geoA1.halfRot;
    while (*Axis1 < -geoA1.halfRot) *Axis1 += geoA1.stepsPerRot;
    while (*Axis1 > geoA1.halfRot) *Axis1 -= geoA1.stepsPerRot;
  }
}


//--------------------------------------------------------------------------------------------------
// GoTo, commands to move the telescope to an location or to report the current location
void syncAxis(const long* axis1, const  long* axis2)
{
  cli();
  staA1.start = *axis1;
  staA2.start = *axis2;
  staA1.pos = *axis1;
  staA2.pos = *axis2;
  staA1.target = *axis1;
  staA2.target = *axis2;
  sei();
}

void GotoAxis(const long* axis1Target, const long* axis2Target )
{
  cli();
  movingTo = true;
  SetsiderealClockSpeed(siderealClockSpeed);
  staA1.resetToSidereal();
  staA2.resetToSidereal();
  staA1.start = staA1.pos;
  staA2.start = staA2.pos;
  staA1.target = *axis1Target;
  staA2.target = *axis2Target;
  sei();
  DecayModeGoto();
}

bool SyncInstr(Coord_IN* instr, PoleSide Side)
{
  long axis1, axis2 = 0;
  Angle2Step(instr->Axis1() * RAD_TO_DEG, instr->Axis2() * RAD_TO_DEG, Side, &axis1, &axis2);
  syncAxis(&axis1, &axis2);
  atHome = false;
  return true;
}

bool syncEqu(Coord_EQ *EQ_T, PoleSide Side, double Lat)
{
  Coord_IN instr = EQ_T->To_Coord_IN(Lat, RefrOptForGoto(), alignment.Tinv);
  return SyncInstr(&instr, Side);
}

// syncs the telescope/mount to the sky
bool syncAzAlt(Coord_HO *HO_T, PoleSide Side)
{
  Coord_IN instr = HO_T->To_Coord_IN(alignment.Tinv);
  return SyncInstr(&instr, Side);
}

void syncTwithE()
{
#if HASEncoder
  long axis1 = (long)(encoderA1.r_deg() * geoA1.stepsPerDegree);
  long axis2 = (long)(encoderA2.r_deg() * geoA2.stepsPerDegree);
  syncAxis(&axis1, &axis2);
  atHome = false;
#endif
}

void syncEwithT()
{
#if HASEncoder
  long axis1, axis2;
  cli();
  axis1 = staA1.pos;
  axis2 = staA2.pos;
  sei();
  encoderA1.w_deg(axis1 / geoA1.stepsPerDegree);
  encoderA2.w_deg(axis2 / geoA2.stepsPerDegree);
#endif
}

bool autoSyncWithEncoder(EncoderSync mode)
{
  bool synced = false;
#if HASEncoder
  static EncoderSync lastmode = ES_OFF;
  static double tol = 0;

  if (lastmode != mode)
  {
    switch (mode)
    {
    case ES_60:
      tol = 1;
      break;
    case ES_30:
      tol = 0.5;
      break;
    case ES_15:
      tol = 0.25;
      break;
    case ES_8:
      tol = 8. / 60.;
      break;
    case ES_4:
      tol = 4. / 60.;
      break;
    case ES_2:
      tol = 2. / 60.;
      break;
    case ES_ALWAYS:
      tol = 0.;
      break;
    case ES_OFF:
      break;
    default:
      break;
    }
    lastmode = mode;
  }
  if (mode == ES_OFF)
  {
    return false;
  }
  long axis1T, axis2T;
  cli();
  axis1T = staA1.pos;
  axis2T = staA2.pos;
  sei();
  long axis1E = (long)(encoderA1.r_deg() * geoA1.stepsPerDegree);
  long axis2E = (long)(encoderA2.r_deg() * geoA2.stepsPerDegree);
  if (abs(axis1T - axis1E) > tol * geoA1.stepsPerDegree)
  {
    cli();
    staA1.pos = axis1E;
    staA1.target = staA1.pos;
    sei();
    synced = true;
  }
  if (abs(axis2T - axis2E) > tol * geoA2.stepsPerDegree)
  {
    cli();
    staA2.pos = axis2E;
    staA2.target = staA2.pos;
    sei();
    synced = true;
  }
#endif
  return synced;
}

void getInstrDeg(double* A1, double* A2, double* A3)
{
  long axis1, axis2;
  cli();
  axis1 = staA1.pos;
  axis2 = staA2.pos;
  sei();
  *A1 = axis1 / geoA1.stepsPerDegree;
  *A2 = axis2 / geoA2.stepsPerDegree;
  *A3 = 0;
}

Coord_IN getInstr()
{
  double Axis1, Axis2, Axis3;
  getInstrDeg(&Axis1, &Axis2, &Axis3);
  return Coord_IN(Axis3 * DEG_TO_RAD, Axis2 * DEG_TO_RAD, Axis1 * DEG_TO_RAD);
}

Coord_IN getInstrE()
{
  return Coord_IN(0, encoderA2.r_deg() * DEG_TO_RAD, encoderA1.r_deg() * DEG_TO_RAD);
}

// gets the telescopes current Topocentric HA and Dec
Coord_EQ getEqu(double Lat)
{
  return getInstr().To_Coord_EQ(alignment.T, RefrOptForGoto(), Lat);
}

Coord_EQ getEquE(double Lat)
{
  return getInstrE().To_Coord_EQ(alignment.T, RefrOptForGoto(), Lat);
}


// gets the telescopes current Topocentric Target RA and Dec, set returnHA to true for Horizon Angle instead of RA
Coord_EQ getEquTarget(double Lat)
{
  return getInstrTarget().To_Coord_EQ(alignment.T, RefrOptForGoto(), Lat);
}



// gets the telescopes current Topo Alt and Azm!
Coord_HO getHorTopo()
{
  return getInstr().To_Coord_HO(alignment.T, { false, 10, 110 });
}


Coord_HO getHorETopo()
{
#if HASEncoder
  return getInstrE().To_Coord_HO(alignment.T, { false, 10, 110 });
#else
  return getHorTopo();
#endif
}

Coord_IN getInstrTarget()
{
  cli();
  double Axis1 = staA1.target / geoA1.stepsPerDegree;
  double Axis2 = staA2.target / geoA2.stepsPerDegree;
  double Axis3 = 0;
  sei();
  return Coord_IN(Axis3 * DEG_TO_RAD, Axis2 * DEG_TO_RAD, Axis1 * DEG_TO_RAD);
}

// gets the telescopes current Apparent Target Alt and Azm!
Coord_HO getHorAppTarget()
{
  return getInstrTarget().To_Coord_HO(alignment.T, RefrOptForGoto());
}


// moves the mount to a new Right Ascension and Declination (RA,Dec) in degrees
byte goToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat)
{
  return goToHor(EQ_T.To_Coord_HO(Lat,RefrOptForGoto()), preferedPoleSide);
}

// moves the mount to a new Altitude and Azmiuth (Alt,Azm) in degrees
byte goToHor(Coord_HO HO_T, PoleSide preferedPoleSide)
{
  double Axis1_target, Axis2_target = 0;
  long axis1_target, axis2_target = 0;
  PoleSide selectedSide = PoleSide::POLE_NOTVALID;

  if (HO_T.Alt() * RAD_TO_DEG < minAlt) return ERRGOTO_BELOWHORIZON;   // fail, below min altitude
  if (HO_T.Alt() * RAD_TO_DEG > maxAlt) return ERRGOTO_ABOVEOVERHEAD;   // fail, above max altitude

  Coord_IN instr_T = HO_T.To_Coord_IN(alignment.Tinv);
  Axis1_target = instr_T.Axis1() * RAD_TO_DEG;
  Axis2_target = instr_T.Axis2() * RAD_TO_DEG;
  if (!predictTarget(Axis1_target, Axis2_target, preferedPoleSide,
    axis1_target, axis2_target, selectedSide))
  {
    return ERRGOTO_LIMITS; //fail, outside limit
  }

  return goTo(axis1_target, axis2_target);
}



// Predict Target
// return 0 if no side can reach the given position
// Axis1_in and Axis2_in are angle coordinates not instr angle coordinates
bool predictTarget(const double& Axis1_in, const double& Axis2_in, const PoleSide& inputSide,
  long& Axis1_out, long& Axis2_out, PoleSide& outputSide)
{
  double Axis1 = Axis1_in;
  double Axis2 = Axis2_in;
  outputSide = inputSide;
  Angle2Step(Axis1, Axis2, outputSide, &Axis1_out, &Axis2_out);
  if (withinLimit(Axis1_out, Axis2_out))
  {
    return true;
  }
  else if (meridianFlip == FLIP_ALWAYS)
  {
    double Axis1 = Axis1_in;
    double Axis2 = Axis2_in;
    if (inputSide == POLE_UNDER) outputSide = POLE_OVER; else outputSide = POLE_UNDER;
    Angle2Step(Axis1, Axis2, outputSide, &Axis1_out, &Axis2_out);
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

  atHome = false;
  GotoAxis(&thisTargetAxis1, &thisTargetAxis2);

  return ErrorsGoTo::ERRGOTO_NONE;
}


ErrorsGoTo Flip()
{
  long Axis1, Axis2, axis1Flip, axis2Flip;
  double Angle1, Angle2;
  PoleSide selectedSide = POLE_NOTVALID;
  PoleSide CurrentSide = POLE_NOTVALID;
  setAtMount(Axis1, Axis2);
  StepToAngle(Axis1, Axis2, &Angle1, &Angle2, &CurrentSide);
  PoleSide preferedPoleSide = (CurrentSide == POLE_UNDER) ? POLE_OVER : POLE_UNDER;
  if (!predictTarget(Angle1, Angle2, preferedPoleSide, axis1Flip, axis2Flip, selectedSide))
  {
    return ErrorsGoTo::ERRGOTO_LIMITS;
  }
  if (selectedSide == GetPoleSide())
  {
    return ErrorsGoTo::ERRGOTO_SAMESIDE;
  }
  return goTo(axis1Flip, axis2Flip);
}

