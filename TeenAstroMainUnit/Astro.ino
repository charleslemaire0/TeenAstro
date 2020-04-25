// -----------------------------------------------------------------------------------------------------------------------------
// Astronomy related functions
#include "ValueToString.h"


// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion

//Topocentric Apparent convertions

// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
double trueRefrac(double Alt, double Pressure = 1010., double Temperature = 10.)
{
  double TPC = (Pressure / 1010.) * (283. / (273. + Temperature));
  double r = ((1.02*cot((Alt + (10.3 / (Alt + 5.11))) / Rad))) * TPC;
  if (r < 0.) r = 0.;
  return r;
}

// returns the amount of refraction (in arcminutes) at the given apparent altitude (degrees), pressure (millibars), and temperature (celsius)
double apparentRefrac(double Alt, double Pressure = 1010., double Temperature = 10.)
{
  double r = -trueRefrac(Alt, Pressure, Temperature);
  r = -trueRefrac(Alt + (r / 60.), Pressure, Temperature);
  return r;
}

void Topocentric2Apparent(double *Alt, double Pressure = 1010., double Temperature = 10.)
{
  if (refraction)
    *Alt += trueRefrac(*Alt, Pressure, Temperature) / 60.;
}

void Apparent2Topocentric(double *Alt, double Pressure = 1010., double Temperature = 10.)
{
  if (refraction)
    *Alt += apparentRefrac(*Alt, Pressure, Temperature) / 60.;
}

// convert equatorial coordinates to horizon
// this takes approx. 363muS on a teensy 3.2 @ 72 Mhz
void EquToHorTopo(double HA, double Dec, double *Azm, double *Alt)
{
  while (HA < 0.) HA = HA + 360.;
  while (HA >= 360.) HA = HA - 360.;
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
  double  t2 = cosHA * localSite.sinLat() - sinDec / cosDec * localSite.cosLat();
  *Azm = atan2(t1, t2) * Rad;
  *Azm = *Azm + 180.;
  *Alt = *Alt * Rad;
}

void EquToHorApp(double HA, double Dec, double *Azm, double *Alt)
{
  EquToHorTopo(HA, Dec, Azm, Alt);
  Topocentric2Apparent(Alt);
}

// convert horizon coordinates to equatorial

// this takes approx. 1.4mS
void HorTopoToEqu(double Azm, double Alt, double *HA, double *Dec)
{
  while (Azm < 0.) Azm = Azm + 360.;
  while (Azm >= 360.) Azm = Azm - 360.;

  Alt = Alt / Rad;
  Azm = Azm / Rad;

  double  SinDec = (sin(Alt) * localSite.sinLat()) + (cos(Alt) * localSite.cosLat() * cos(Azm));
  *Dec = asin(SinDec);

  double  t1 = sin(Azm);
  double  t2 = cos(Azm) * localSite.sinLat() - tan(Alt) * localSite.cosLat();
  *HA = atan2(t1, t2) * Rad;
  *HA = *HA + 180.;
  *Dec = *Dec * Rad;
}

void HorAppToEqu(double Azm, double Alt, double *HA, double *Dec)
{
  Apparent2Topocentric(&Alt);
  HorTopoToEqu(Azm, Alt, HA, Dec);
}

// -----------------------------------------------------------------------------------------------------------------------------
// Refraction rate tracking
double  az_deltaAxis1 = 15., az_deltaAxis2 = 0.;
double  az_deltaRateScale = 1.;

// az_deltaH/D are in arc-seconds/second

// trackingTimerRateAxis1/2 are x the sidereal rate
void SetDeltaTrackingRate()
{
  trackingTimerRateAxis1 = az_deltaAxis1 / 15.;
  trackingTimerRateAxis2 = az_deltaAxis2 / 15.;

  fstepAxis1 = (long)(StepsPerSecondAxis1 * trackingTimerRateAxis1 / 100.);
  fstepAxis2 = (long)(StepsPerSecondAxis2 * trackingTimerRateAxis2 / 100.);
}

void SetTrackingRate(double r)
{
  az_deltaRateScale = r;
  if (!isAltAZ())
  {
    az_deltaAxis1 = r * 15.;
    az_deltaAxis2 = 0.;
  }
  SetDeltaTrackingRate();
}

double GetTrackingRate()
{
  return az_deltaRateScale;
}


#define AltAzTrackingRange  1  // distance in arc-min (20) ahead of and behind the current Equ position, used for rate calculation
bool do_compensation_calc()
{
  bool done = false;

  static long axis1_before, axis1_after = 0;
  static long axis2_before, axis2_after = 0;
  static double Axis1_tmp, Axis2_tmp = 0.;
  static double HA_tmp, HA_now = 0.;
  static double Dec_tmp, Dec_now = 0.;
  static double Azm_tmp, Alt_tmp = 0.;
  static int    az_step = 0;

  // turn off if not tracking at sidereal rate
  if (!sideralTracking)
  {
    az_deltaAxis1 = 0.;
    az_deltaAxis2 = 0.;
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
    HA_tmp = (HA_now - (AltAzTrackingRange / 60.));
    Dec_tmp = Dec_now;
    break;
  case 15:
    EquToHorApp(HA_tmp, Dec_tmp, &Azm_tmp, &Alt_tmp);
    alignment.toInstrumentalDeg(Axis1_tmp, Axis2_tmp, Azm_tmp, Alt_tmp);
    InstrtoStep(Axis1_tmp, Axis2_tmp, GetPierSide(), &axis1_before, &axis2_before);
    break;
  case 110:
    // look behind the current position
    HA_tmp = (HA_now + (AltAzTrackingRange / 60.));
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
    if ((axis1_after < -halfRotAxis1) && (axis1_before > halfRotAxis1)) axis1_after += 2 * halfRotAxis1;
    if ((axis1_before < -halfRotAxis1) && (axis1_after > halfRotAxis1)) axis1_after += 2 * halfRotAxis1;
    // set rates

    az_deltaAxis1 = (distStepAxis1(&axis1_before, &axis1_after) / StepsPerDegreeAxis1 * (15. / (AltAzTrackingRange / 60.)) / 2.) * az_deltaRateScale;
    az_deltaAxis2 = (distStepAxis2(&axis2_before, &axis2_after) / StepsPerDegreeAxis2 * (15. / (AltAzTrackingRange / 60.)) / 2.) * az_deltaRateScale;
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

double haRange(double d)
{
  while (d >= 180.) d -= 360.;
  while (d < -180.) d += 360.;
  return d;
}

double AzRange(double d)
{
  while (d >= 360.) d -= 360.;
  while (d < 0.) d += 360.;
  return d;
}


double degRange(double d)
{
  while (d >= 360.) d -= 360.;
  while (d < 0.) d += 360.;
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
  double maxslewEEPROM = XEEPROM.readInt(EE_maxRate);
  double maxslewCorrected = SetRates(maxslewEEPROM);          // set the new acceleration rate
  if (abs(maxslewEEPROM - maxslewCorrected) > 2)
  {
    XEEPROM.writeInt(EE_maxRate, (int)maxslewCorrected);
  }
}


// Acceleration rate calculation
double SetRates(double maxslewrate)
{
  // set the new acceleration rate
  double fact = 3600. / 15. * 1. / ( StepsPerDegreeAxis1 * 1. / 16. / 1000000.);
  maxRate = max(fact / maxslewrate, (MaxRate / 2L) * 16L);
  maxslewrate = fact / maxRate;
  guideRates[9] = maxslewrate;
  guideRates[8] = maxslewrate / 2.;
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
  amountGuideAxis1 = (long)(guideTimerBaseRate * StepsPerSecondAxis1 / 100.);
  amountGuideAxis2 = (long)(guideTimerBaseRate * StepsPerSecondAxis2 / 100.);
  sei();
}

void enableST4GuideRate()
{
  if (guideTimerBaseRate != guideRates[0])
  {
    guideTimerBaseRate = guideRates[0];
    cli();
    amountGuideAxis1 = (long)(guideTimerBaseRate * StepsPerSecondAxis1 / 100.);
    amountGuideAxis2 = (long)(guideTimerBaseRate * StepsPerSecondAxis2 / 100.);
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
  amountGuideAxis1 = (long)((abs(vRate) * StepsPerSecondAxis1) / 100.);
  guideTimerRateAxis1 = vRate;
  sei();
}

void enableRateAxis2(double vRate)
{
  cli();
  amountGuideAxis2 = (long)((abs(vRate) * StepsPerSecondAxis2) / 100.);
  guideTimerRateAxis2 = vRate;
  sei();
}

void InsrtAngle2Angle(double *AngleAxis1, double *AngleAxis2, PierSide *Side)
{
  if (*AngleAxis2 > 90.)
  {
    *AngleAxis2 = (90. - *AngleAxis2) + 90.;
    *AngleAxis1 = *AngleAxis1 - 180.;
    *Side = PierSide::PIER_WEST;
  }
  else if (*AngleAxis2 < -90.)
  {
    *AngleAxis2 = (-90. - *AngleAxis2) - 90.;
    *AngleAxis1 = *AngleAxis1 - 180.;
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
      *AngleAxis2 = (90. - *AngleAxis2) + 90.;
    else
      *AngleAxis2 = (-90. - *AngleAxis2) - 90.;
    *AngleAxis1 = *AngleAxis1 + 180.;
  }
}



bool isAltAZ()
{
  return mountType == MOUNT_TYPE_ALTAZM || mountType == MOUNT_TYPE_FORK_ALT;
}