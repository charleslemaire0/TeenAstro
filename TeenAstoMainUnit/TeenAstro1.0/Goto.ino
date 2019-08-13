//--------------------------------------------------------------------------------------------------
// GoTo, commands to move the telescope to an location or to report the current location


// syncs the telescope/mount to the sky
boolean syncEqu(double RA, double Dec)
{
  // hour angleTrackingMoveTo
  double  HA = haRange(rtk.LST() * 15.0 - RA);
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

    EquToStep(localSite.latitude(), HA, Dec, &axis1, &axis2);
  }

  // compute index offsets indexAxis1/indexAxis2, if they're within reason
  // actual posAxis1/posAxis2 are the coords of where this really is
  // indexAxis1/indexAxis2 are the amount to add to the actual RA/Dec to arrive at the correct position
  // double's are really single's on the ATMega's, and we're a digit or two shy of what's required to
  // hold the steps in some cases but it's still getting down to the arc-sec level
  // HA goes from +180...0..-180
  //                 W   .   E
  // indexAxis1 and indexAxis2 values get subtracted to arrive at the correct location
  //indexAxis1 = InstrHA - ((double)(long)targetAxis1.part.m) / (double)StepsPerDegreeAxis1;
  //indexAxis2 = InstrDec - ((double)(long)targetAxis2.part.m) / (double)StepsPerDegreeAxis2;
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
boolean syncAltAz(double Az, double Alt)
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
    EquToStep(localSite.latitude(), Ha, Dec, &axis1, &axis2);
  }

  // compute index offsets indexAxis1/indexAxis2, if they're within reason
  // actual posAxis1/posAxis2 are the coords of where this really is
  // indexAxis1/indexAxis2 are the amount to add to the actual RA/Dec to arrive at the correct position
  // double's are really single's on the ATMega's, and we're a digit or two shy of what's required to
  // hold the steps in some cases but it's still getting down to the arc-sec level
  // HA goes from +180...0..-180
  //                 W   .   E
  // indexAxis1 and indexAxis2 values get subtracted to arrive at the correct location
  //indexAxis1 = InstrHA - ((double)(long)targetAxis1.part.m) / (double)StepsPerDegreeAxis1;
  //indexAxis2 = InstrDec - ((double)(long)targetAxis2.part.m) / (double)StepsPerDegreeAxis2;
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



// this returns the telescopes HA and Dec (index corrected for Alt/Azm)
void getHADec(double *HA, double *Dec) {
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

    // instrument to corrected horizon
    z += 0 /*+ indexAxis1*/;
    a += 0 /*+ indexAxis2*/;

    HorToEqu(a, z, HA, Dec); // convert from Alt/Azm to HA/Dec
  }
  else
  {
    // get the hour angle (or Azm)
    *HA = Axis1 / StepsPerDegreeAxis1;
    // get the declination (or Alt)
    *Dec = Axis2 / StepsPerDegreeAxis2;
  }
}


// gets the telescopes current RA and Dec, set returnHA to true for Horizon Angle instead of RA
boolean getEqu(double *RA, double *Dec, boolean returnHA)
{
  double  HA;

  if (!isAltAZ())
  {
    // get the HA and Dec
    GetEqu(localSite.latitude(), &HA, Dec);
  }
  else
  {
    // get the HA and Dec (already index corrected on AltAzm)
    getHADec(&HA, Dec);
  }

  // return either the RA or the HA depending on returnHA
  if (!returnHA)
  {
    *RA = degRange(rtk.LST() * 15.0 - HA);
  }
  else
    *RA = HA;

  return true;
}

// gets the telescopes current RA and Dec, set returnHA to true for Horizon Angle instead of RA
boolean getApproxEqu(double *RA, double *Dec, boolean returnHA)
{
  double  HA;

  // get the HA and Dec (already index corrected on AltAzm)
  GetInstr(&HA, Dec);


  // return either the RA or the HA depending on returnHA
  if (!returnHA)
  {
    *RA = degRange(rtk.LST() * 15.0 - HA);
  }
  else
    *RA = HA;
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
byte goToEqu(double RA, double Dec, PierSide preferedPierSide)
{
  double  a, z;
  long Axis1, Axis2;

  // Convert RA into hour angle, get altitude
  double  HA = haRange(rtk.LST() * 15.0 - RA);
  EquToHor(HA, Dec, &a, &z);

  // Check to see if this goto is valid
  if ((parkStatus != PRK_UNPARKED) && (parkStatus != PRK_PARKING)) return 4; // fail, PRK_PARKED
  if (lastError != ERR_NONE) return lastError + 10;   // fail, telescop has Errors State
  if (a < minAlt) return 1;   // fail, below horizon
  if (a > maxAlt) return 6;   // fail, outside limits
  if (Dec > MaxDec) return 6; // fail, outside limits
  if (Dec < MinDec) return 6; // fail, outside limits
  if (movingTo)
  {
    abortSlew = true;
    return 5;
  }   // fail, prior goto cancelled

  if (guideDirAxis1 || guideDirAxis2) return 7;   // fail, unspecified error
  if (isAltAZ())
  {

    EquToHor(HA, Dec, &a, &z);
    z = AzRange(z);

    if (DegreePastAZ>0)
    {
      cli();
      double  a1 = (posAxis1 /*+ indexAxis1Steps*/) / StepsPerDegreeAxis1;
      sei();
      //compute alternative position
       double z1 = z + 360;
       double z2 = z - 360;
       if (Azok((long)(z1 * StepsPerDegreeAxis1)) && dist(a1, z) > dist(a1, z1))
         z = z1;
       else if (Azok((long)(z2 * StepsPerDegreeAxis1)) && dist(a1, z) > dist(a1, z2))
         z = z2;
    }

    // corrected to instrument horizon
    z -= 0;
    a -= 0;

    Axis1 = z * StepsPerDegreeAxis1;
    Axis2 = a * StepsPerDegreeAxis2;
    //long    Axis1Alt = Axis1;
    //long    Axis2Alt = Axis2;

  }
  else
  {
    // correct for polar offset, refraction, coordinate systems, operation past pole, etc. as required
    double h, d;
    PierSide oldPierSide = pierSide;
    pierSide = preferedPierSide;
    EquToInstr(localSite.latitude(), HA, Dec, &h, &d);
    pierSide = oldPierSide;
    PierSide side = predictSideOfPier(h, preferedPierSide);
    if (side == 0)  return 6; //fail, outside limit
    if (side != pierSide)
    {
      oldPierSide = pierSide;
      pierSide = side;
      EquToStep(localSite.latitude(), HA, Dec, &Axis1, &Axis2);
      //Serial.println("-doGoto at-");
      //sprintf(reply, "axis 1 %ld", Axis1);
      //Serial.println(reply);
      //sprintf(reply, "axis 2 %ld", Axis2);
      //Serial.println(reply);
      pierSide = oldPierSide;
    }
    else
    {
      EquToStep(localSite.latitude(), HA, Dec, &Axis1, &Axis2);
    }

  }
  return goTo(Axis1, Axis2);
}


// moves the mount to a new Altitude and Azmiuth (Alt,Azm) in degrees
byte goToHor(double *Alt, double *Azm)
{
  double  HA, Dec;
  HorToEqu(*Alt, *Azm, &HA, &Dec);
  double  RA = degRange(rtk.LST() * 15.0 - HA);
  return goToEqu(RA, Dec,pierSide);
}


// moves the mount to a new Hour Angle and Declination - both are in steps.  Alternate targets are used when a meridian flip occurs

byte goTo(long thisTargetAxis1, long thisTargetAxis2)
{
  // HA goes from +90...0..-90
  //                W   .   E
  if (faultAxis1 || faultAxis2) return 7; // fail, unspecified error
  atHome = false;


  // final validation
if (isAltAZ())
{
  if (!Azok(thisTargetAxis1) || !Altok(thisTargetAxis2))
    return 7;   // fail, unspecified error
}
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


