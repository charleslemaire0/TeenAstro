// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions

// convert string in format MM/DD/YY to julian date
boolean dateToDouble(double *JulianDay, char *date)
{
    int     m1, d1, y1;
    if (dateToYYYYMMDD(&y1, &m1, &d1, date))
    {
      *JulianDay = julian(y1, m1, d1);
      return true;
    }
    return false;
}

// convert string in format HH:MM:SS to floating point

// (also handles)           HH:MM.M
boolean dateToYYYYMMDD(int *y1, int *m1, int *d1, char *date)

{
  char    m[3], d[3], y[3];
  if (strlen(date) != 8) return false;
  m[0] = *date++;
  m[1] = *date++;
  m[2] = 0;
  atoi2(m, m1);
  if (*date++ != '/') return false;
  d[0] = *date++;
  d[1] = *date++;
  d[2] = 0;
  atoi2(d, d1);
  if (*date++ != '/') return false;
  y[0] = *date++;
  y[1] = *date++;
  y[2] = 0;
  atoi2(y, y1);
  if ((*m1 < 1) || (*m1 > 12) || (*d1 < 1) || (*d1 > 31) || (*y1 < 0) || (*y1 > 99))
    return false;
  if (*y1 > 11)
    *y1 = *y1 + 2000;
  else
    *y1 = *y1 + 2100;
  return true;
}
boolean hmsToDouble(double *f, char *hms)
{
  int h1, m1, m2, s1;
  if (hmsToHms(&h1, &m1, &m2, &s1, hms))
  {
    *f = h1 + m1 / 60.0 + m2 / 600.0 + s1 / 3600.0;
    return true;
  }
  return false;
}

boolean hmsToHms(int *h1, int *m1, int *m2, int *s1, char*hms)
{
  char    h[3], m[5], s[3];
  *h1 = -1;
  *m1 = 0;
  *m2 = 0;
  *s1 = 0;

  while (*hms == ' ') hms++;  // strip prefix white-space
  if (highPrecision)
  {
    if (strlen(hms) != 8) return false;
  }
  else if (strlen(hms) != 7)
    return false;

  h[0] = *hms++;
  h[1] = *hms++;
  h[2] = 0;
  atoi2(h, h1);
  if (highPrecision)
  {
    if (*hms++ != ':') return false;
    m[0] = *hms++;
    m[1] = *hms++;
    m[2] = 0;
    atoi2(m, m1);
    if (*hms++ != ':') return false;
    s[0] = *hms++;
    s[1] = *hms++;
    s[2] = 0;
    atoi2(s, s1);
  }
  else
  {
    if (*hms++ != ':') return false;
    m[0] = *hms++;
    m[1] = *hms++;
    m[2] = 0;
    atoi2(m, m1);
    if (*hms++ != '.') return false;
    *m2 = (*hms++) - '0';
  }

  if ((*h1 < 0) || (*h1 > 23) || (*m1 < 0) || (*m1 > 59) || (*m2 < 0) || (*m2 > 599) ||
    (*s1 < 0) || (*s1 > 59))
    return false;
  return true;
}

boolean doubleToHms(char *reply, double *f)
{
    double  h1,
            m1,
            f1,
            s1;

    f1 = fabs(*f) + 0.000139;   // round to 1/2 arc-sec
    h1 = floor(f1);
    m1 = (f1 - h1) * 60;
    s1 = (m1 - floor(m1));

    char    s[] = "%s%02d:%02d:%02d";
    if (highPrecision)
    {
        s1 = s1 * 60.0;
    }
    else
    {
        s1 = s1 * 10.0;
        s[11] = '.';
        s[14] = '1';
    }

    char    sign[2] = "";
    if (((s1 != 0) || (m1 != 0) || (h1 != 0)) && (*f < 0.0))
        strcpy(sign, "-");
    sprintf(reply, s, sign, (int) h1, (int) m1, (int) s1);

    return true;
}

// convert string in format sDD:MM:SS to floating point
// (also handles)           DDD:MM:SS
//                          sDD*MM'SS
//                          sDD:MM
//                          DDD:MM
//                          sDD*MM

//                          DDD*MM
boolean dmsToDouble(double *f, char *dms, boolean sign_present)
{
    char    d[4],
            m[5],
            s[3];
    int     d1,
            m1,
            s1 = 0;
    int     lowLimit = 0,
            highLimit = 360;
    int     checkLen,
            checkLen1;
    double  sign = 1.0;
    boolean secondsOff = false;

    while (*dms == ' ') dms++;  // strip prefix white-space
    checkLen1 = strlen(dms);

    // determine if the seconds field was used and accept it if so
    if (highPrecision)
    {
        checkLen = 9;
        if (checkLen1 != checkLen) return false;
    }
    else
    {
        checkLen = 6;
        if (checkLen1 != checkLen)
        {
            if (checkLen1 == 9)
            {
                secondsOff = false;
                checkLen = 9;
            }
            else
                return false;
        }
        else
            secondsOff = true;
    }

    // determine if the sign was used and accept it if so
    if (sign_present)
    {
        if (*dms == '-')
            sign = -1.0;
        else if (*dms == '+')
            sign = 1.0;
        else
            return false;
        dms++;
        d[0] = *dms++;
        d[1] = *dms++;
        d[2] = 0;
        if (!atoi2(d, &d1)) return false;
    }
    else
    {
        d[0] = *dms++;
        d[1] = *dms++;
        d[2] = *dms++;
        d[3] = 0;
        if (!atoi2(d, &d1)) return false;
    }

    // make sure the seperator is an allowed character
    if ((*dms != ':') && (*dms != '*') && (*dms != char(223)))
        return false;
    else
        dms++;

    m[0] = *dms++;
    m[1] = *dms++;
    m[2] = 0;
    if (!atoi2(m, &m1)) return false;
    if ((highPrecision) && (!secondsOff))
    {
        // make sure the seperator is an allowed character
        if ((*dms != ':') && (*dms != '\''))
          return false;
        else
          dms++;
        s[0] = *dms++;
        s[1] = *dms++;
        s[2] = 0;
        atoi2(s, &s1);
    }

    if (sign_present)
    {
        lowLimit = -90;
        highLimit = 90;
    }

    if ((d1 < lowLimit) || (d1 > highLimit) || (m1 < 0) || (m1 > 59) || (s1 < 0) ||
        (s1 > 59))
        return false;

    *f = sign * (d1 + m1 / 60.0 + s1 / 3600.0);
    return true;
}

boolean doubleToDms(char *reply, const double *f, boolean fullRange,
                    boolean signPresent)
{
    char    sign[] = "+";
    int     o = 0,
            d1,
            s1 = 0;
    double  m1,
            f1;
    f1 = *f;

    // setup formatting, handle adding the sign
    if (f1 < 0)
    {
        f1 = -f1;
        sign[0] = '-';
    }

    f1 = f1 + 0.000139;         // round to 1/2 arc-second
    d1 = floor(f1);
    m1 = (f1 - d1) * 60.0;
    s1 = (m1 - floor(m1)) * 60.0;

    char    s[] = "+%02d*%02d:%02d";
    if (signPresent)
    {
        if (sign[0] == '-')
        {
            s[0] = '-';
        }

        o = 1;
    }
    else
    {
        strcpy(s, "%02d*%02d:%02d");
    }

    if (fullRange) s[2 + o] = '3';

    if (highPrecision)
    {
        sprintf(reply, s, d1, (int) m1, s1);
    }
    else
    {
        s[9 + o] = 0;
        sprintf(reply, s, d1, (int) m1);
    }

    return true;
}

// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion

//Topocentric Apparent convertions

// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
double trueRefrac(double Alt, double Pressure = 1010.0, double Temperature = 10.0) {
  double TPC = (Pressure / 1010.0) * (283.0 / (273.0 + Temperature));
  double r = ((1.02*cot((Alt + (10.3 / (Alt + 5.11))) / Rad))) * TPC;
  if (r < 0.0) r = 0.0;
  return r;
}

// returns the amount of refraction (in arcminutes) at the given apparent altitude (degrees), pressure (millibars), and temperature (celsius)
double apparentRefrac(double Alt, double Pressure = 1010.0, double Temperature = 10.0) {
  double r = -trueRefrac(Alt, Pressure, Temperature);
  r = -trueRefrac(Alt + (r / 60.0), Pressure, Temperature);
  return r;
}

void Topocentric2Apparent(double *Alt, double Pressure=1010.0, double Temperature=10.0)
{
  if (refraction)
    *Alt += trueRefrac(*Alt, Pressure, Temperature) / 60.0;
}

void Apparent2Topocentric(double *Alt, double Pressure=1010.0, double Temperature=10.0)
{
  if (refraction)
    *Alt += apparentRefrac(*Alt, Pressure, Temperature) / 60;
}

// convert equatorial coordinates to horizon
// this takes approx. 363muS on a teensy 3.2 @ 72 Mhz
void EquToHorTopo(double HA, double Dec,  double *Azm, double *Alt)
{
    while (HA < 0.0) HA = HA + 360.0;
    while (HA >= 360.0) HA = HA - 360.0;
    HA = HA / Rad;
    Dec = Dec / Rad;

    //double  SinAlt = (sin(Dec) * localSite.sinLat()) + (cos(Dec) * localSite.cosLat() * cos(HA));
    double cosHA = cos(HA);
    double sinHA = sin(HA);
    double cosDec = cos(Dec);
    double sinDec = sin(Dec);
    double  SinAlt = (sinDec * localSite.sinLat()) + (cosDec * localSite.cosLat() * cosHA);
    *Alt = asin(SinAlt);
    double  t1 = sinHA;
    double  t2 = cosHA * localSite.sinLat() - sinDec/cosDec * localSite.cosLat();
    *Azm = atan2(t1, t2) * Rad;
    *Azm = *Azm + 180.0;
    *Alt = *Alt * Rad;
}

void EquToHorApp(double HA, double Dec,  double *Azm, double *Alt)
{
  EquToHorTopo(HA, Dec, Azm, Alt);
  Topocentric2Apparent(Alt);
}

// convert horizon coordinates to equatorial

// this takes approx. 1.4mS
void HorTopoToEqu( double Azm, double Alt, double *HA, double *Dec)
{
    while (Azm < 0) Azm = Azm + 360.0;
    while (Azm >= 360.0) Azm = Azm - 360.0;

    Alt = Alt / Rad;
    Azm = Azm / Rad;

    double  SinDec = (sin(Alt) * localSite.sinLat()) + (cos(Alt) * localSite.cosLat() * cos(Azm));
    *Dec = asin(SinDec);

    double  t1 = sin(Azm);
    double  t2 = cos(Azm) * localSite.sinLat() - tan(Alt) * localSite.cosLat();
    *HA = atan2(t1, t2) * Rad;
    *HA = *HA + 180.0;
    *Dec = *Dec * Rad;
}

void HorAppToEqu(double Azm, double Alt, double *HA, double *Dec)
{
  Apparent2Topocentric(&Alt);
  HorTopoToEqu(Azm, Alt, HA, Dec);
}

// -----------------------------------------------------------------------------------------------------------------------------
// Refraction rate tracking
double  az_deltaAxis1 = 15.0, az_deltaAxis2 = 0.0;
double  az_deltaRateScale = 1.0;

// az_deltaH/D are in arc-seconds/second

// trackingTimerRateAxis1/2 are x the sidereal rate
void SetDeltaTrackingRate()
{
    trackingTimerRateAxis1 = az_deltaAxis1 / 15.0;
    trackingTimerRateAxis2 = az_deltaAxis2 / 15.0;

    fstepAxis1.fixed = doubleToFixed((StepsPerSecondAxis1 * trackingTimerRateAxis1) / 100.0);
    fstepAxis2.fixed = doubleToFixed((StepsPerSecondAxis2 * trackingTimerRateAxis2) / 100.0);
}

void SetTrackingRate(double r)
{
    az_deltaRateScale = r;
    if (!isAltAZ())
    {
      az_deltaAxis1 = r * 15.0;
      az_deltaAxis2 = 0.0;
    }
    SetDeltaTrackingRate();
}

double GetTrackingRate()
{
    return az_deltaRateScale;
}


#define AltAzTrackingRange  1  // distance in arc-min (20) ahead of and behind the current Equ position, used for rate calculation
boolean do_compensation_calc()
{
  boolean done = false;
  
  static long axis1_before, axis1_after = 0;
  static long axis2_before, axis2_after = 0;
  static double Axis1_tmp, Axis2_tmp = 0;
  static double HA_tmp, HA_now = 0;
  static double Dec_tmp, Dec_now = 0;
  static double Azm_tmp, Alt_tmp = 0;
  static int    az_step = 0;

  // turn off if not tracking at sidereal rate
  if (!sideralTracking)
  {
    az_deltaAxis1 = 0.0;
    az_deltaAxis2 = 0.0;
    az_step = 0;
    return true;
  }
  az_step++;
  // convert units, get ahead of and behind current position
  switch (az_step)
  {
  case 1:
    if (movingTo)
      getEquTarget(&HA_now, &Dec_now, true);
    else
      getEqu(&HA_now, &Dec_now, true);
    break;
  case 10:
    // look ahead of the current position
    HA_tmp = (HA_now - (AltAzTrackingRange / 60.0));
    Dec_tmp = Dec_now;
    break;
  case 15:
    EquToHorApp(HA_tmp, Dec_tmp, &Azm_tmp, &Alt_tmp);
    alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
    InstrtoStep(Axis1_tmp, Axis2_tmp, GetPierSide(), &axis1_before, &axis2_before);
    break;
  case 110:
    // look behind the current position
    HA_tmp = (HA_now + (AltAzTrackingRange / 60.0));
    Dec_tmp = Dec_now;
    break;
  case 115:
    EquToHorApp(HA_tmp, Dec_tmp, &Azm_tmp, &Alt_tmp);
    alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
    InstrtoStep(Axis1_tmp, Axis2_tmp, GetPierSide(), &axis1_after, &axis2_after);
    break;
  case 120:
    // we have both -0.5hr and +0.5hr values // calculate tracking rate deltas'
               // handle coordinate wrap
    if ((axis1_after < -halfRotAxis1) && (axis1_before > halfRotAxis1)) axis1_after += 2*halfRotAxis1;
    if ((axis1_before < -halfRotAxis1) && (axis1_after > halfRotAxis1)) axis1_after += 2*halfRotAxis1;
    // set rates

    az_deltaAxis1 = (distStepAxis1(axis1_before, axis1_after) / StepsPerDegreeAxis1 * (15.0 / (AltAzTrackingRange / 60.0)) / 2.0) * az_deltaRateScale;
    az_deltaAxis2 = (distStepAxis2(axis2_before, axis2_after) / StepsPerDegreeAxis2 * (15.0 / (AltAzTrackingRange / 60.0)) / 2.0) * az_deltaRateScale;
    // override for special case of near a celestial pole
    //if (90.0 - fabs(Dec_now) <= 0.5)
    //{
    //  az_deltaAxis1 = 0.0;
    //  az_deltaAxis2 = 0.0;
    //}
    break;
  case 200:
    az_step = 0;
    done = true;
    break;
  }
  return done;
}

//// Misc. numeric conversion
//double timeRange(double t)
//{
//    while (t >= 24.0) t -= 24.0;
//    while (t < 0.0) t += 24.0;
//    return t;
//}

double haRange(double d)
{
    while (d >= 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

double AzRange(double d)
{
    while (d >= 360.0 ) d -= 360.0;
    while (d < 0.0 ) d += 360.0;
    return d;
}


double degRange(double d)
{
    while (d >= 360.0) d -= 360.0;
    while (d < 0.0) d += 360.0;
    return d;
}

double dist(double a, double b)
{
    if (a > b)
        return a - b;
    else
        return b - a;
}

double angDist(double h, double d, double h1, double d1)
{
    return acos(sin(d / Rad) * sin(d1 / Rad) + cos(d / Rad) * cos(d1 / Rad) * cos((h1 - h) / Rad)) * Rad;
}

void initMaxRate()
{
  double maxslewEEPROM = EEPROM_readInt(EE_maxRate);
  double maxslewCorrected = SetRates(maxslewEEPROM);          // set the new acceleration rate
  if (abs(maxslewEEPROM - maxslewCorrected) > 2)
  {
    EEPROM_writeInt(EE_maxRate, (int)maxslewCorrected);
  }
}


// Acceleration rate calculation
double SetRates(double maxslewrate)
{
   // set the new acceleration rate
  double fact = 3600 / 15 * 1 / ((double)StepsPerDegreeAxis1 * 1 / 16 / 1000000.0);
  maxRate = max(fact / maxslewrate, (MaxRate / 2L) * 16L);
  maxslewrate = fact / maxRate;
  guideRates[9] = maxslewrate;
  guideRates[8] = maxslewrate /2.;
  resetGuideRate();
  SetAcceleration();
  return maxslewrate;
}

void SetAcceleration()
{
  double Vmax = getV(maxRate);
  cli();
    AccAxis1 = Vmax / (2. * DegreesForAcceleration * StepsPerDegreeAxis1)*Vmax;
    AccAxis2 = Vmax / (2. * DegreesForAcceleration * StepsPerDegreeAxis2)*Vmax;
  sei();
}

// calculates the tracking speed for move commands
void enableGuideRate(int g, bool force)
{
  // don't do these lengthy calculations unless we have to

  if (g < 0) g = 0;
  if (g > 9) g = 9;
  if (!force && guideTimerBaseRate == guideRates[g]) return;

  activeGuideRate = g;

  // this enables the guide rate
  guideTimerBaseRate = guideRates[g];

  cli();
  amountGuideAxis1.fixed = doubleToFixed((guideTimerBaseRate * StepsPerSecondAxis1) / 100.0);
  amountGuideAxis2.fixed = doubleToFixed((guideTimerBaseRate * StepsPerSecondAxis2) / 100.0);
  sei();
}

void enableST4GuideRate()
{
  if (guideTimerBaseRate != guideRates[0])
  {
    guideTimerBaseRate = guideRates[0];
    cli();
    amountGuideAxis1.fixed = doubleToFixed((guideTimerBaseRate * StepsPerSecondAxis1) / 100.0);
    amountGuideAxis2.fixed = doubleToFixed((guideTimerBaseRate * StepsPerSecondAxis2) / 100.0);
    sei();
  }
}

void resetGuideRate()
{
  enableGuideRate(activeGuideRate, true);
}

void enableRateAxis1(double vRate)
{
  cli();
  amountGuideAxis1.fixed = doubleToFixed((abs(vRate) * StepsPerSecondAxis1) / 100.0);
  guideTimerRateAxis1 = vRate;
  sei();
}

void enableRateAxis2(double vRate)
{
  cli();
  amountGuideAxis2.fixed = doubleToFixed((abs(vRate) * StepsPerSecondAxis2) / 100.0);
  guideTimerRateAxis2 = vRate;
  sei();
}

void InsrtAngle2Angle(double *AngleAxis1, double *AngleAxis2, PierSide *Side)
{
  if (*AngleAxis2 > 90.0)
  {
    *AngleAxis2 = (90.0 - *AngleAxis2) + 90;
    *AngleAxis1 = *AngleAxis1 - 180.0;
    *Side = PierSide::PIER_WEST;
  }
  else if (*AngleAxis2 < -90.0)
  {
    *AngleAxis2 = (-90.0 - *AngleAxis2) - 90.0;
    *AngleAxis1 = *AngleAxis1 - 180.0;
    *Side = PierSide::PIER_WEST;
  }
  else
    *Side = PierSide::PIER_EAST;
}

void Angle2InsrtAngle(PierSide Side, double *AngleAxis1, double *AngleAxis2)
{
  if (Side >= PIER_WEST)
  {
    //TODO Verify for altaz!!
    if (*localSite.latitude() >= 0)
      *AngleAxis2 = (90.0 - *AngleAxis2) + 90;
    else
      *AngleAxis2 = (-90.0 - *AngleAxis2) - 90;
    *AngleAxis1 = *AngleAxis1 + 180.0;
  }
}

long distStepAxis1( long start, long end)
{
  return end - start;
}

long distStepAxis2(long start, long end)
{
 return end - start;
}

bool isAltAZ()
{
  return mountType == MOUNT_TYPE_ALTAZM || mountType == MOUNT_TYPE_FORK_ALT;
}