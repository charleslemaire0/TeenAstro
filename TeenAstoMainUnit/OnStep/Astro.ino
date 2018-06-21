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
        if (*dms++ != ':') return false;
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


//
//// Set RTC with give input
//void setClock(int y1, int m1, int d1, int h1, int mi1, int s1)
//{
//  tmElements_t t1;
//  t1.Year = y1 - 1970;
//  t1.Month = m1;
//  t1.Day = d1;
//  t1.Hour = h1;
//  t1.Minute = mi1;
//  t1.Second = s1;
//  time_t t = makeTime(t1);
//  Teensy3Clock.set(t);
//  setTime(t);
//}
//
//// Syn Telescope Drive with RTC
//void syncClock()
//{
//  time_t RTClock = Teensy3Clock.get();
//  JD = julian(year(RTClock), month(RTClock), day(RTClock));
//  LMT = hour(RTClock) + minute(RTClock) / 60.0 + second(RTClock) / 3600.0;
//  UT1 = LMT + timeZone;
//  UT1_start = UT1;
//  UT1mS_start = millis();
//  update_lst(jd2last(JD, UT1));
//}
//// -----------------------------------------------------------------------------------------------------------------------------
//// Date Time conversion
//
//// converts Gregorian date (Y,M,D) to Julian day number
//double julian(int Year, int Month, int Day)
//{
//    if ((Month == 1) || (Month == 2))
//    {
//        Year--;
//        Month = Month + 12;
//    }
//
//    double  B = 2.0 - floor(Year / 100.0) + floor(Year / 400.0);
//    return
//        (
//            B +
//                floor(365.25 * Year) +
//                floor(30.6001 * (Month + 1.0)) +
//                Day +
//                1720994.5
//        );  //+(Time/24.0);
//}
//
//// converts Julian day number to Gregorian date (Y,M,D)
//void greg(double JulianDay, int *Year, int *Month, int *Day)
//{
//    double  A,
//            B,
//            C,
//            D,
//            D1,
//            E,
//            F,
//            G,
//            I;
//
//    JulianDay = JulianDay + 0.5;
//    I = floor(JulianDay);
//
//    F = 0.0;    //  JD-I;
//    if (I > 2299160.0)
//    {
//        A = int((I - 1867216.25) / 36524.25);
//        B = I + 1.0 + A - floor(A / 4.0);
//    }
//    else
//        B = I;
//
//    C = B + 1524.0;
//    D = floor((C - 122.1) / 365.25);
//    E = floor(365.25 * D);
//    G = floor((C - E) / 30.6001);
//
//    D1 = C - E + F - floor(30.6001 * G);
//    *Day = floor(D1);
//    if (G < 13.5)
//        *Month = floor(G - 1.0);
//    else
//        *Month = floor(G - 13.0);
//    if (*Month > 2.5)
//        *Year = floor(D - 4716.0);
//    else
//        *Year = floor(D - 4715.0);
//}
//
//// convert date/time to Greenwich Apparent Sidereal time
//double jd2gast(double JulianDay, double ut1)
//{
//    int y,
//        m,
//        d;
//    greg(JulianDay, &y, &m, &d);
//
//    double  JulianDay0 = julian(y, m, d);
//    double  D = (JulianDay - 2451545.0) + (ut1 / 24.0);
//    double  D0 = (JulianDay0 - 2451545.0);
//    double  H = ut1;
//    double  T = D / 36525.0;
//    double  gmst = 6.697374558 + 0.06570982441908 * D0;
//    gmst = timeRange(gmst);
//    gmst = gmst + 1.00273790935 * H + 0.000026 * T * T;
//    gmst = timeRange(gmst);
//
//    // equation of the equinoxes
//    double  O = 125.04 - 0.052954 * D;
//    double  L = 280.47 + 0.98565 * D;
//    double  E = 23.4393 - 0.0000004 * D;
//    double  W = -0.000319 * sin(O / Rad) - 0.000024 * sin((2 * L) / Rad);
//    double  eqeq = W * cos(E / Rad);
//    double  gast = gmst + eqeq;
//    return timeRange(gast);
//}
//
//// convert date/time to Local Apparent Sidereal Time
//
//// uses longitude
//double jd2last(double JulianDay, double ut1)
//{
//    // JulianDay is the Local date, jd2gast requires a universal time
//    // this is a hack that leaves the date alone and lets the UT1 cover
//    // the difference in time to the next (or previous) day
//    double  gast = jd2gast(JulianDay, ut1);
//    return timeRange(gast - (*localSite.longitude() / 15.0));
//}




// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion
// convert equatorial coordinates to horizon

// this takes approx. 1.4mS on a 16MHz Mega2560
void EquToHor(double HA, double Dec, double *Alt, double *Azm)
{
    while (HA < 0.0) HA = HA + 360.0;
    while (HA >= 360.0) HA = HA - 360.0;
    HA = HA / Rad;
    Dec = Dec / Rad;

    double  SinAlt = (sin(Dec) * localSite.sinLat()) + (cos(Dec) * localSite.cosLat() * cos(HA));
    *Alt = asin(SinAlt);

    double  t1 = sin(HA);
    double  t2 = cos(HA) * localSite.sinLat() - tan(Dec) * localSite.cosLat();
    *Azm = atan2(t1, t2) * Rad;
    *Azm = *Azm + 180.0;
    *Alt = *Alt * Rad;
}

// convert horizon coordinates to equatorial

// this takes approx. 1.4mS
void HorToEqu(double Alt, double Azm, double *HA, double *Dec)
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

// -----------------------------------------------------------------------------------------------------------------------------
// Refraction rate tracking
int     az_step = 0;
double  az_Axis1 = 0, az_Axis2 = 0;
double  az_Dec = 0, az_HA = 0;
double  az_Dec1 = 0, az_HA1 = 0, az_Dec2 = -91, az_HA2 = 0;
double  az_Alt, az_Azm, _az_Alt;
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
    if (mountType != MOUNT_TYPE_ALTAZM)
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

// -----------------------------------------------------------------------------------------------------------------------------
// Low overhead altitude calculation, 16 calls to complete
byte    ac_step = 0;
double  ac_HA = 0, ac_De = 0, ac_Dec = 0;
double  ac_sindec, ac_cosdec, ac_cosha;
double  ac_sinalt;

double getApproxDec()
{
    return ac_De;
}

boolean do_fastalt_calc()
{
    boolean done = false;
    ac_step++;

    // load HA/Dec
    if (ac_step == 1)
    {
        getApproxEqu(&ac_HA, &ac_De, true);
        ac_Dec = ac_De;
    }
    else    // convert units
    if (ac_step == 2)
    {
        ac_HA = ac_HA / Rad;
        ac_Dec = ac_Dec / Rad;
    }
    else    // prep Dec
    if (ac_step == 3)
    {
        ac_sindec = sin(ac_Dec);
    }
    else    // prep Dec
    if (ac_step == 4)
    {
        ac_cosdec = cos(ac_Dec);
    }
    else    // prep HA
    if (ac_step == 5)
    {
        ac_cosha = cos(ac_HA);
    }
    else    // calc Alt, phase 1
    if (ac_step == 6)
    {
        ac_sinalt = (ac_sindec * localSite.sinLat()) + (ac_cosdec * localSite.cosLat() * ac_cosha);
    }
    else    // calc Alt, phase 2
    if (ac_step == 7)
    {
        currentAlt = asin(ac_sinalt) * Rad;
    }
    else    // finish
    if (ac_step == 8)
    {
        ac_step = 0;
        done = true;
    }

    return done;
}

// -----------------------------------------------------------------------------------------------------------------------------
// Refraction adjusted tracking

// returns the amount of refraction (in arcminutes) at given altitude (degrees), pressure (millibars), and temperature (celsius)
double Refrac(double Alt, double Pressure = 1010.0, double Temperature = 15.0)
{
    double  TPC = (Pressure / 1010.0) * (283.0 / (273.0 + Temperature));
    double  r = ((1.02 * cot((Alt + (10.3 / (Alt + 5.11))) / Rad))) * TPC;
    if (r < 0.0) r = 0.0;
    return r;
}

// Alternate tracking rate calculation method
double ZenithTrackingRate()
{
    double  Alt1 = currentAlt + 0.5;
    if (Alt1 < 0.0) Alt1 = 0.0;

    double  Alt2 = currentAlt - 0.5;
    if (Alt2 < 0.0) Alt2 = 0.0;
    if (currentAlt > 89.8) return 15.0;
    if (currentAlt > 89.5) return 14.998;

    double  Alt1_ = Alt1 - (Refrac(Alt1) / 60.0);
    double  Alt2_ = Alt2 - (Refrac(Alt2) / 60.0);

    return 15.0 * ((double) ((Alt1 - Alt2) / (Alt1_ - Alt2_)));
}

// distance in arc-min ahead of and behind the current Equ position, used for rate calculation
#if defined(__AVR_ATmega2560__)
#define RefractionRateRange 30
#else
#define RefractionRateRange 10
#endif
boolean do_refractionRate_calc()
{
    boolean done = false;

    // turn off if not tracking at sidereal rate
    if (trackingState != TrackingON)
    {
        az_deltaAxis1 = 15.0;
        az_deltaAxis2 = 0.0;
        return true;
    }

    az_step++;

    // load HA/Dec
    if (az_step == 1)
    {
        if (onTrack)
            getEqu(&az_Axis1, &az_Axis2, true);
        else
            getApproxEqu(&az_Axis1, &az_Axis2, true);
    }
    else    // convert units,  get ahead of and behind current position
    if ((az_step == 5) || (az_step == 105))
    {
        az_Dec = az_Axis2;
        az_HA = az_Axis1;
        if (az_step == 5) az_HA = az_HA - (RefractionRateRange / 60.0);
        if (az_step == 105) az_HA = az_HA + (RefractionRateRange / 60.0);
    }
    else    // get the Horizon coords
    if ((az_step == 10) || (az_step == 110))
    {
        if (onTrack)
            GeoAlign.EquToInstr(localSite.latitude(), az_HA, az_Dec, &az_HA, &az_Dec);
    }

    // get the Horizon coords
    if ((az_step == 15) || (az_step == 115))
    {
        EquToHor(az_HA, az_Dec, &az_Alt, &az_Azm);
    }
    else    // apply refraction
    if ((az_step == 20) || (az_step == 120))
    {
        az_Alt += Refrac(az_Alt) / 60.0;
    }
    else    // convert back to the Equtorial coords
    if ((az_step == 25) || (az_step == 125))
    {
        HorToEqu(az_Alt, az_Azm, &az_HA1, &az_Dec1);
        if (az_HA1 > 180.0) az_HA1 -= 360.0;    // HA range +/-180
    }
    else                        // calculate refraction rate deltas'
    if ((az_step == 30) || (az_step == 130))
    {
        // store first calc
        if (az_step == 30)
        {
            az_HA2 = az_HA1;
            az_Dec2 = az_Dec1;
        }

        // we have both -0.5hr and +0.5hr values
        if (az_step == 130)
        {
            // set rates
            // handle coordinate wrap
            if ((az_HA1 < -90.0) && (az_HA2 > 90.0)) az_HA1 += 360.0;
            if ((az_HA2 < -90.0) && (az_HA1 > 90.0)) az_HA2 += 360.0;

            // set rates
            double  dax1 = (az_HA1 - az_HA2) *
                (15.0 / (RefractionRateRange / 60.0)) / 2.0;
            az_deltaAxis1 = (az_deltaAxis1 * 9.0 + dax1) / 10.0;

            double  dax2 = (az_Dec1 - az_Dec2) *
                (15.0 / (RefractionRateRange / 60.0)) / 2.0;
            az_deltaAxis2 = (az_deltaAxis2 * 9.0 + dax2) / 10.0;

            // override for special case of near a celestial pole
            if (90.0 - fabs(az_Dec) < (1.0 / 3600.0))
            {
                az_deltaAxis1 = 15.0;
                az_deltaAxis2 = 0.0;
            }

            // override for special case of near the zenith
            if (currentAlt > (90.0 - 7.5))
            {
                az_deltaAxis1 = ZenithTrackingRate();
                az_deltaAxis2 = 0.0;
            }
        }
    }
    else                        // finish once every 200 calls
    if (az_step == 200)
    {
        az_step = 0;
        done = true;
    }

    return done;
}

// -----------------------------------------------------------------------------------------------------------------------------
// AltAz tracking

#define AltAzTrackingRange  10  // distance in arc-min (20) ahead of and behind the current Equ position, used for rate calculation
double  az_Alt1, az_Alt2, az_Azm1, az_Azm2;

boolean do_altAzmRate_calc()
{
    boolean done = false;

    // turn off if not tracking at sidereal rate
    if (((trackingState != TrackingON) && (trackingState != TrackingMoveTo)
        ))
    {
        az_deltaAxis1 = 0.0;
        az_deltaAxis2 = 0.0;
        return true;
    }

    az_step++;

    // convert units, get ahead of and behind current position
    if (az_step == 1)
    {
        if (trackingState == TrackingMoveTo)
        {
            cli();
            az_Axis1 = targetAxis1.part.m;
            az_Axis2 = targetAxis2.part.m;
            sei();
        }
        else
        {
            cli();
            az_Axis1 = posAxis1;
            az_Axis2 = posAxis2;
            sei();
        }

        // get the Azm
        az_Azm = (double) az_Axis1 / (double) StepsPerDegreeAxis1;

        // get the Alt
        az_Alt = (double) az_Axis2 / (double) StepsPerDegreeAxis2;
    }
    else                        // convert to Equatorial coords
    if ((az_step == 5))
    {
        HorToEqu(az_Alt, az_Azm, &az_HA1, &az_Dec1);
    }
    else                        // look ahead of and behind the current position
    if ((az_step == 10) || (az_step == 110))
    {
        if (az_step == 10) az_HA = (az_HA1 - (AltAzTrackingRange / 60.0));
        if (az_step == 110) az_HA = (az_HA1 + (AltAzTrackingRange / 60.0));
        az_Dec = az_Dec1;
    }
    else                        // each back to the Horizon coords
    if ((az_step == 15) || (az_step == 115))
    {
        EquToHor(az_HA, az_Dec, &az_Alt, &az_Azm);
        if (az_Azm > 180.0) az_Azm -= 360.0;
        if (az_Azm < -180.0) az_Azm += 360.0;

        if (az_step == 15)
        {
            az_Alt2 = az_Alt;
            az_Azm2 = az_Azm;
        }

        if (az_step == 115)
        {
            az_Alt1 = az_Alt;
            az_Azm1 = az_Azm;
        }
    }
    else                        // calculate tracking rate deltas'
    if ((az_step == 20) || (az_step == 120))
    {
        // we have both -0.5hr and +0.5hr values
        if (az_step == 120)
        {
            // handle coordinate wrap
            if ((az_Azm1 < -90.0) && (az_Azm2 > 90.0)) az_Azm1 += 360.0;
            if ((az_Azm2 < -90.0) && (az_Azm1 > 90.0)) az_Azm2 += 360.0;

            // set rates
            az_deltaAxis1 =
                (
                    (az_Azm1 - az_Azm2) *
                    (15.0 / (AltAzTrackingRange / 60.0)) /
                    2.0
                ) *
                az_deltaRateScale;
            az_deltaAxis2 =
                (
                    (az_Alt1 - az_Alt2) *
                    (15.0 / (AltAzTrackingRange / 60.0)) /
                    2.0
                ) *
                az_deltaRateScale;

            // override for special case of near a celestial pole
            if (90.0 - fabs(az_Dec) <= 0.5)
            {
                az_deltaAxis1 = 0.0;
                az_deltaAxis2 = 0.0;
            }
        }
    }
    else                        // finish once every 200 calls
    if (az_step == 200)
    {
        az_step = 0;
        done = true;
    }

    return done;
}


// -----------------------------------------------------------------------------------------------------------------------------

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


// Acceleration rate calculation
void SetAccelerationRates()
{
    // set the new acceleration rate
  double slewrate = (1.0 / (((double)StepsPerDegreeAxis1 * ((maxRate / 16L) / 1000000.0))) * 3600.0) / 15.0;
  DegreesForAcceleration = slewrate / 100;
  DegreesForRapidStop = 0.5 *DegreesForAcceleration;
  guideRates[9] = slewrate;
  guideRates[8] = slewrate/2.;
  if (activeGuideRate > 7)
  {
    resetGuideRate();
  }
  cli();
    StepsForRateChangeAxis1 = ((double) DegreesForAcceleration / sqrt( StepsPerDegreeAxis1)) * 0.3333333 * StepsPerDegreeAxis1 * maxRate;
    StepsForRateChangeAxis2 = ((double) DegreesForAcceleration / sqrt( StepsPerDegreeAxis2)) * 0.3333333 * StepsPerDegreeAxis2 * maxRate;
  sei();
}

// calculates the tracking speed for move commands
void enableGuideRate(int g, bool force)
{
  // don't do these lengthy calculations unless we have to

  if (g < 0) g = 0;
  if (g > 9) g = 9;
  if (activeGuideRate == g && !force) return;

  activeGuideRate = g;

  // this enables the guide rate
  guideTimerBaseRate = guideRates[g];

  cli();
  amountGuideHA.fixed = doubleToFixed((guideTimerBaseRate * StepsPerSecondAxis1) / 100.0);
  amountGuideDec.fixed = doubleToFixed((guideTimerBaseRate * StepsPerSecondAxis2) / 100.0);
  sei();
}
void resetGuideRate()
{
  enableGuideRate(activeGuideRate, true);
}

void enableRateAxis1(double vRate)
{
  cli();
  amountGuideHA.fixed = doubleToFixed((abs(vRate) * StepsPerSecondAxis1) / 100.0);
  guideTimerRateAxis1 = vRate;
  sei();
}

void enableRateAxis2(double vRate)
{
  cli();
  amountGuideDec.fixed = doubleToFixed((abs(vRate) * StepsPerSecondAxis2) / 100.0);
  guideTimerRateAxis2 = vRate;
  sei();
}



// Remap HA DEC value between -180 +180 and -90 +90
void CorrectHADec(double *HA, double *Dec)
{
  if (mountType != MOUNT_TYPE_ALTAZM)
  {
    // switch from under the pole coordinates
    if (*Dec > 90.0)
    {
      *Dec = (90.0 - *Dec) + 90;
      *HA = *HA - 180.0;
    }
    else if (*Dec < -90.0)
    {
      *Dec = (-90.0 - *Dec) - 90.0;
      *HA = *HA - 180.0;
    }
  }
  while (*HA > 180.0) *HA -= 360.0;
  while (*HA < -180.0) *HA += 360.0;
}

void InsrtHADec2HADec(double *HA, double *Dec, byte *Side)
{

  if (*Side >= PierSideWest)
  {
    if (*Dec >= 90.0)
    {
      *Dec = (90.0 - *Dec) + 90;
      *HA = *HA - 180.0;
    }
    else if (*Dec <= -90.0)
    {
      *Dec = (-90.0 - *Dec) - 90.0;
      *HA = *HA - 180.0;
    }
    while (*HA > 180.0) *HA -= 360.0;
    while (*HA < -180.0) *HA += 360.0;
  }
  else
  {
    CorrectHADec(HA,Dec);
  }
}

void HADec2InsrtHADec(double *HA, double *Dec, byte *Side )
{
  if (*Side >= PierSideWest)
  {
    if (*localSite.latitude() >= 0)
      *Dec = (90.0 - *Dec) + 90;
    else
      *Dec = (-90.0 - *Dec) - 90;
    *HA = *HA - 180.0;
    while (*HA > 180.0) *HA -= 360.0;
    while (*HA < -180.0) *HA += 360.0;
  }
}