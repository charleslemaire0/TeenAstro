#include <TeenAstroMath.h>
#include <math.h>
#include <Arduino.h>
double frac(double v)
{
  return v - ((long)v);
}

double cot(double n)
{
  return 1.0 / tan(n);
}

// integer numeric conversion with error checking
bool atoi2(char *a, int *i)
{
  char    *conv_end;
  long    l = strtol(a, &conv_end, 10);

  if ((l < -32767) || (l > 32768) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
}

bool atoui2(char* a, unsigned int* i)
{
  char* conv_end;
  unsigned long l = strtoul(a, &conv_end, 10);

  if ( (l > 65535) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
}



double haRange(double d)
{
  while (d >= 180.) d -= 360.;
  while (d < -180.) d += 360.;
  return d;
}

double haRangeRad(double d)
{
  while (d >= M_PI) d -= M_PI *2;
  while (d < -M_PI) d += M_PI * 2;
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

double angDist(double h, double d, double h1, double d1)
{
  return acos(sin(d / Rad) * sin(d1 / Rad) + cos(d / Rad) * cos(d1 / Rad) * cos((h1 - h) / Rad)) * Rad;
}

// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion
//Topocentric Apparent convertions
// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
double trueRefrac(double Alt, double Pressure, double Temperature)
{
  double TPC = (Pressure / 1010.) * (283. / (273. + Temperature));
  double r = ((1.02*cot((Alt + (10.3 / (Alt + 5.11))) / Rad))) * TPC;
  if (r < 0.) r = 0.;
  return r;
}

// returns the amount of refraction (in arcminutes) at the given apparent altitude (degrees), pressure (millibars), and temperature (celsius)
double apparentRefrac(double Alt, double Pressure, double Temperature)
{
  double r = -trueRefrac(Alt, Pressure, Temperature);
  r = -trueRefrac(Alt + (r / 60.), Pressure, Temperature);
  return r;
}

void Topocentric2Apparent(double *Alt, double Pressure, double Temperature)
{
  *Alt += trueRefrac(*Alt, Pressure, Temperature) / 60.;
}

void Apparent2Topocentric(double *Alt, double Pressure, double Temperature)
{
  *Alt += apparentRefrac(*Alt, Pressure, Temperature) / 60.;
}

// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion

//Topocentric Apparent convertions


// convert equatorial coordinates to horizon
// this takes approx. 363muS on a teensy 3.2 @ 72 Mhz
void EquToHorTopo(double HA, double Dec, double *Azm, double *Alt, const double *cosLat, const double *sinLat)
{
  while (HA < 0.) HA = HA + 360.;
  while (HA >= 360.) HA = HA - 360.;
  HA = HA / Rad;
  Dec = Dec / Rad;;
  double cosHA = cos(HA);
  double sinHA = sin(HA);
  double cosDec = cos(Dec);
  double sinDec = sin(Dec);
  double  SinAlt = (sinDec * *sinLat) + (cosDec * *cosLat * cosHA);
  *Alt = asin(SinAlt);
  double  t1 = sinHA;
  double  t2 = cosHA * *sinLat - sinDec / cosDec * *cosLat;
  *Azm = atan2(t1, t2) * Rad;
  *Azm = *Azm + 180.;
  *Alt = *Alt * Rad;
}
void EquToHorApp(double HA, double Dec, double *Azm, double *Alt, const double *cosLat, const double *sinLat)
{
  EquToHorTopo(HA, Dec, Azm, Alt, cosLat, sinLat);
  Topocentric2Apparent(Alt);
}

void EquToHor(double HA, double Dec, bool refraction, double* Azm, double* Alt, const double* cosLat, const double* sinLat)
{
  EquToHorTopo(HA, Dec, Azm, Alt, cosLat, sinLat);
  if (refraction)
  {
    Topocentric2Apparent(Alt);
  }
}


// convert horizon coordinates to equatorial
// this takes approx. 1.4mS
void HorTopoToEqu(double Azm, double Alt, double *HA, double *Dec, const double *cosLat, const double *sinLat)
{
  while (Azm < 0.) Azm = Azm + 360.;
  while (Azm >= 360.) Azm = Azm - 360.;

  Alt = Alt / Rad;
  Azm = Azm / Rad;

  double  SinDec = (sin(Alt) * *sinLat) + (cos(Alt) * *cosLat * cos(Azm));
  *Dec = asin(SinDec);

  double  t1 = sin(Azm);
  double  t2 = cos(Azm) * *sinLat - tan(Alt) * *cosLat;
  *HA = atan2(t1, t2) * Rad;
  *HA = *HA + 180.;
  *Dec = *Dec * Rad;
}
void HorAppToEqu(double Azm, double Alt, double *HA, double *Dec, const double *cosLat, const double *sinLat)
{
  Apparent2Topocentric(&Alt);
  HorTopoToEqu(Azm, Alt, HA, Dec, cosLat, sinLat);
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
void Angle2InsrtAngle(PierSide Side, double *AngleAxis1, double *AngleAxis2, const double *Lat, const double poleAxis1 )
{
  double angle = *AngleAxis1;
  if (Side >= PIER_WEST)
  {
    //TODO Verify for altaz!!
    if (*Lat >= 0)
      *AngleAxis2 = (90. - *AngleAxis2) + 90.;
    else
      *AngleAxis2 = (-90. - *AngleAxis2) - 90.;
    angle += 180.;
    while (angle < -180.) angle += 360.;
    while (angle > 180.) angle -= 360.;
    *AngleAxis1 = angle;
  }
}

long distStepAxis1(long* start, long* end)
{
  return *end - *start;
}
long distStepAxis1(volatile long* start, volatile long* end)
{
  return *end - *start;
}
long distStepAxis1(volatile long* start, volatile double* end)
{
  return *end - *start;
}
long distStepAxis2(long* start, long* end)
{
  return *end - *start;
}
long distStepAxis2(volatile long* start, volatile long* end)
{
  return *end - *start;
}
long distStepAxis2(volatile long* start, volatile double* end)
{
  return *end - *start;
}