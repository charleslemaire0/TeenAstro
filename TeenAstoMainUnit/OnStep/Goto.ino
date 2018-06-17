//--------------------------------------------------------------------------------------------------
// GoTo, commands to move the telescope to an location or to report the current location


// syncs the telescope/mount to the sky
boolean syncEqu(double RA, double Dec)
{
  // hour angleTrackingMoveTo
  double  HA = haRange(rtk.LST() * 15.0 - RA);
  long axis1, axis2;
  // correct for polar misalignment only by clearing the index offsets

  if (mountType == MOUNT_TYPE_ALTAZM)
  {
    double Axis1, Axis2;
    if (Align.isReady())
    {
      // B=RA, D=Dec, H=Elevation, F=Azimuth (all in degrees)
      Align.EquToInstr(HA, Dec, &Axis2, &Axis1);
    }
    else
    {
      EquToHor(HA, Dec, &Axis2, &Axis1);
    }

    while (Axis1 > 180.0) Axis1 -= 360.0;
    while (Axis1 < -180.0) Axis1 += 360.0;
    axis1 = Axis1 * StepsPerDegreeAxis1;
    axis2 = Axis2 * StepsPerDegreeAxis2;
  }
  else
  {

    GeoAlign.EquToStep(localSite.latitude(), HA, Dec, &axis1, &axis2);
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
  deltaSyncAxis1 =  (double)(axis1 - (long)targetAxis1.part.m ) / StepsPerDegreeAxis1;
  deltaSyncAxis2 =  (double)(axis2 - (long)targetAxis2.part.m ) / StepsPerDegreeAxis2;
  posAxis1 = axis1;
  posAxis2 = axis2;
  targetAxis1.part.m = axis1;
  targetAxis1.part.f = 0;
  targetAxis2.part.m = axis2;
  targetAxis2.part.f = 0;
  sei();
  return true;
}

bool deltaSyncEqu(double RA, double Dec)
{
  long axis1, axis2;
  // hour angleTrackingMoveTo
  double  HA = haRange(rtk.LST() * 15.0 - RA);

  // correct for polar misalignment only by clearing the index offsets

  if (mountType == MOUNT_TYPE_ALTAZM)
  {
    double Axis1, Axis2;
    if (Align.isReady())
    {
      // B=RA, D=Dec, H=Elevation, F=Azimuth (all in degrees)
      Align.EquToInstr(HA, Dec, &Axis2, &Axis1);
    }
    else
    {
      EquToHor(HA, Dec, &Axis2, &Axis1);
    }

    while (Axis1 > 180.0) Axis1 -= 360.0;
    while (Axis1 < -180.0) Axis1 += 360.0;
    axis1 = Axis1 * StepsPerDegreeAxis1;
    axis1 = Axis2 * StepsPerDegreeAxis1;
  }
  else
  {
    GeoAlign.EquToStep(localSite.latitude(), HA, Dec, &axis1, &axis2);
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
  deltaSyncAxis1 = (double)(axis1 - (long)targetAxis1.part.m) / StepsPerDegreeAxis1;
  deltaSyncAxis2 = (double)(axis2 - (long)targetAxis2.part.m) / StepsPerDegreeAxis2;
  sei();
  return true;
}

// this returns the telescopes HA and Dec (index corrected for Alt/Azm)
void getHADec(double *HA, double *Dec) {
  cli();
  double Axis1 = posAxis1;
  double Axis2 = posAxis2;
  sei();

  if (mountType == MOUNT_TYPE_ALTAZM)
  {
    // get the hour angle (or Azm)
    double z = Axis1 / (double)StepsPerDegreeAxis1;
    // get the declination (or Alt)
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

  if (mountType != MOUNT_TYPE_ALTAZM)
  {
    // get the HA and Dec

    GeoAlign.GetEqu(localSite.latitude(), &HA, Dec);
  }
  else
  {
    if (Align.isReady())
    {
      cli();

      // get the Azm/Alt
      double  F = (double)(posAxis1 /*+ indexAxis1Steps*/) / StepsPerDegreeAxis1;
      double  H = (double)(posAxis2 /*+ indexAxis2Steps*/) / StepsPerDegreeAxis2;
      sei();

      // H=Elevation, F=Azimuth, B=RA, D=Dec (all in degrees)
      Align.InstrToEqu(H, F, &HA, Dec);
    }
    else
    {
      // get the HA and Dec (already index corrected on AltAzm)
      getHADec(&HA, Dec);
    }
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
  GeoAlign.GetInstr(&HA, Dec);


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
byte goToEqu(double RA, double Dec)
{
  double  a, z;
  long Axis1, Axis2;

  // Convert RA into hour angle, get altitude
  double  HA = haRange(rtk.LST() * 15.0 - RA);
  EquToHor(HA, Dec, &a, &z);

  // Check to see if this goto is valid
  if ((parkStatus != NotParked) && (parkStatus != Parking)) return 4; // fail, Parked
  if (lastError != ERR_NONE) return (10 + lastError);   // fail, telescop has Errors State
  if (a < minAlt) return 1;   // fail, below horizon
  if (a > maxAlt) return 6;   // fail, outside limits
  if (Dec > MaxDec) return 6; // fail, outside limits
  if (Dec < MinDec) return 6; // fail, outside limits
  if (trackingState == TrackingMoveTo)
  {
    abortSlew = true;
    return 5;
  }   // fail, prior goto cancelled

  if (guideDirAxis1 || guideDirAxis2) return 7;   // fail, unspecified error
  if (mountType == MOUNT_TYPE_ALTAZM)
  {
    if (Align.isReady())
    {
      // B=RA, D=Dec, H=Elevation, F=Azimuth (all in degrees)
      Align.EquToInstr(HA, Dec, &a, &z);
    }
    else
    {
      EquToHor(HA, Dec, &a, &z);
    }

    z = haRange(z);

    cli();

    double  a1 = (posAxis1 /*+ indexAxis1Steps*/) / StepsPerDegreeAxis1;
    sei();


    if ((MaxAzm > 180) && (MaxAzm <= 360))
    {
      // adjust coordinate range to allow going past 180 deg.
      // position a1 is 0..180
      if (a1 >= 0)
      {
        // and goto z is in -0..-180
        if (z < 0)
        {
          // the alternate z1 is in 180..360
          double  z1 = z + 360.0;
          if ((z1 < MaxAzm) && (dist(a1, z) > dist(a1, z1))) z = z1;
        }
      }

      // position a1 -0..-180
      if (a1 < 0)
      {
        // and goto z is in 0..180
        if (z > 0)
        {
          // the alternate z1 is in -360..-180
          double  z1 = z - 360.0;
          if ((z1 > -MaxAzm) && (dist(a1, z) > dist(a1, z1))) z = z1;
        }
      }
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

    GeoAlign.EquToInstr(localSite.latitude(), HA, Dec, &h, &d);

    byte side = predictSideOfPier(h, pierSide);
    if (side == 0)  return 6; //fail, outside limit
    if (side != pierSide)
    {
      byte oldPierSide = pierSide;
      pierSide = side;
      GeoAlign.EquToStep(localSite.latitude(), HA, Dec, &Axis1, &Axis2);
      //Serial.println("-doGoto at-");
      //sprintf(reply, "axis 1 %ld", Axis1);
      //Serial.println(reply);
      //sprintf(reply, "axis 2 %ld", Axis2);
      //Serial.println(reply);
      if (oldPierSide == PierSideEast)
        pierSide = PierSideFlipEW1;
      else
        pierSide = PierSideFlipWE1;
    }
    else
    {
      GeoAlign.EquToStep(localSite.latitude(), HA, Dec, &Axis1, &Axis2);
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

  return goToEqu(RA, Dec);
}


// moves the mount to a new Hour Angle and Declination - both are in steps.  Alternate targets are used when a meridian flip occurs

byte goTo(long thisTargetAxis1, long thisTargetAxis2)
{
  // HA goes from +90...0..-90
  //                W   .   E
  if (faultAxis1 || faultAxis2) return 7; // fail, unspecified error
  atHome = false;


  // final validation
if (mountType== MOUNT_TYPE_ALTAZM)
{
    // allow +/- 360 in Az
  if (((thisTargetAxis1 /*+ indexAxis1Steps*/ > (long)StepsPerDegreeAxis1 *
    MaxAzm) || (thisTargetAxis1 /*+ indexAxis1Steps*/ < -(long)StepsPerDegreeAxis1 * MaxAzm)) || ((
        thisTargetAxis2 /*+ indexAxis2Steps*/ > (long) StepsPerDegreeAxis2 *
        180L) || (thisTargetAxis2 /*+ indexAxis2Steps*/ < -(long)StepsPerDegreeAxis2 *
          180L)))
    return 7;   // fail, unspecified error
}
  lastTrackingState = trackingState;

  cli();
  trackingState = TrackingMoveTo;
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


