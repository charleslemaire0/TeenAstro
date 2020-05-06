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

