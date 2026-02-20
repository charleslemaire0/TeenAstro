/**
 * @file  TeenAstroMath.cpp
 * @brief Implementation of the TeenAstro math utility library.
 *
 * @author  Charles Lemaire
 */

#include <cmath>
#include <cstdint>
#include <Arduino.h>
#include <TeenAstroMath.h>

/* ======================================================================== */
/*  Coordinate formatting  (merged from TeenAstroFunction)                   */
/* ======================================================================== */

void gethms(const long& v, uint8_t& h, uint8_t& m, uint8_t& s)
{
  s = v % 60;
  m = (v / 60) % 60;
  h = v / 3600;
}

void getdms(const long& v, bool& ispos, uint16_t& deg, uint8_t& min, uint8_t& sec)
{
  ispos = v >= 0;
  long vabs = ispos ? v : -v;
  sec = vabs % 60;
  min = (vabs / 60) % 60;
  deg = vabs / 3600;
}

/* ======================================================================== */
/*  String-to-integer parsing                                                */
/* ======================================================================== */

bool atoi2(char* a, int* i)
{
  char*   conv_end;
  int32_t l = strtol(a, &conv_end, 10);

  if ((l < INT16_MIN) || (l > INT16_MAX) || (&a[0] == conv_end))
    return false;

  *i = static_cast<int16_t>(l);
  return true;
}

bool atoui2(char* a, unsigned int* i)
{
  char*    conv_end;
  uint32_t l = strtoul(a, &conv_end, 10);

  if ((l > UINT16_MAX) || (&a[0] == conv_end))
    return false;

  *i = static_cast<uint16_t>(l);
  return true;
}

/* ======================================================================== */
/*  Basic math helpers                                                       */
/* ======================================================================== */

double frac(double v)
{
  return v - floor(v);
}

double cot(double n)
{
  return 1.0 / tan(n);
}

/* ======================================================================== */
/*  Angle normalisation                                                      */
/* ======================================================================== */

double haRange(double d)
{
  return remainder(d, 360.0);
}

double haRangeRad(double d)
{
  return remainder(d, 2.0 * M_PI);
}

double AzRange(double d)
{
  d = remainder(d, 360.0);
  if (d < 0.0) d += 360.0;
  return d;
}

double degRange(double d)
{
  d = remainder(d, 360.0);
  if (d < 0.0) d += 360.0;
  return d;
}

/* ======================================================================== */
/*  Angular distance                                                         */
/* ======================================================================== */

double angDist(double h, double d, double h1, double d1)
{
  double dRad  = d  / Rad;
  double d1Rad = d1 / Rad;
  double dhRad = (h1 - h) / Rad;

  return acos(sin(dRad) * sin(d1Rad) +
              cos(dRad) * cos(d1Rad) * cos(dhRad)) * Rad;
}

/* ======================================================================== */
/*  Atmospheric refraction  [DEPRECATED]                                     */
/*                                                                           */
/*  Degree-based formulas kept for UniversalMainUnit compatibility.          */
/*  New code should use LA3::Topocentric2Apparent / Apparent2Topocentric     */
/*  (radian-based Meeus Saemundsson / Bennett formulas).                     */
/* ======================================================================== */

double trueRefrac(double Alt, double Pressure, double Temperature)
{
  double TPC = (Pressure / 1010.0) * (283.0 / (273.0 + Temperature));
  double r   = 1.02 * cot((Alt + 10.3 / (Alt + 5.11)) / Rad) * TPC;
  if (r < 0.0) r = 0.0;
  return r;
}

/**
 * [DEPRECATED] Internal helper: refraction in arc-minutes for an apparent altitude.
 * Uses iterative inversion of trueRefrac.
 */
static double apparentRefrac(double Alt, double Pressure, double Temperature)
{
  double r = -trueRefrac(Alt, Pressure, Temperature);
  r = -trueRefrac(Alt + r / 60.0, Pressure, Temperature);
  return r;
}

void Topocentric2Apparent(double* Alt, double Pressure, double Temperature)
{
  *Alt += trueRefrac(*Alt, Pressure, Temperature) / 60.0;
}

void Apparent2Topocentric(double* Alt, double Pressure, double Temperature)
{
  *Alt += apparentRefrac(*Alt, Pressure, Temperature) / 60.0;
}

/* ======================================================================== */
/*  Coordinate conversion  [LEGACY]                                          */
/*                                                                           */
/*  Degree-based free functions kept for UniversalMainUnit.                  */
/*  New code should use TeenAstroCoord classes (Coord_EQ, Coord_HO).        */
/* ======================================================================== */

/**
 * [LEGACY] Internal: Equatorial -> Topocentric Horizontal (degrees).
 */
static void EquToHorTopo(double HA, double Dec,
                          double* Azm, double* Alt,
                          const double* cosLat, const double* sinLat)
{
  /* Normalise HA to [0, 360) */
  while (HA <    0.0) HA += 360.0;
  while (HA >= 360.0) HA -= 360.0;

  HA  = HA  / Rad;
  Dec = Dec / Rad;

  double cosHA  = cos(HA);
  double sinHA  = sin(HA);
  double cosDec = cos(Dec);
  double sinDec = sin(Dec);

  double sinAlt = sinDec * (*sinLat) + cosDec * (*cosLat) * cosHA;
  *Alt = asin(sinAlt);

  double t1 = sinHA;
  double t2 = cosHA * (*sinLat) - sinDec / cosDec * (*cosLat);
  *Azm = atan2(t1, t2) * Rad + 180.0;
  *Alt = *Alt * Rad;
}

/**
 * [LEGACY] Internal: Equatorial -> Apparent Horizontal (degrees).
 */
static void EquToHorApp(double HA, double Dec,
                         double* Azm, double* Alt,
                         const double* cosLat, const double* sinLat)
{
  EquToHorTopo(HA, Dec, Azm, Alt, cosLat, sinLat);
  Topocentric2Apparent(Alt);
}

void EquToHor(double HA, double Dec, bool refraction,
              double* Azm, double* Alt,
              const double* cosLat, const double* sinLat)
{
  EquToHorTopo(HA, Dec, Azm, Alt, cosLat, sinLat);
  if (refraction)
  {
    Topocentric2Apparent(Alt);
  }
}

void HorTopoToEqu(double Azm, double Alt,
                   double* HA, double* Dec,
                   const double* cosLat, const double* sinLat)
{
  /* Normalise Azm to [0, 360) */
  while (Azm <    0.0) Azm += 360.0;
  while (Azm >= 360.0) Azm -= 360.0;

  Alt = Alt / Rad;
  Azm = Azm / Rad;

  double sinDec = sin(Alt) * (*sinLat) + cos(Alt) * (*cosLat) * cos(Azm);
  *Dec = asin(sinDec);

  double t1 = sin(Azm);
  double t2 = cos(Azm) * (*sinLat) - tan(Alt) * (*cosLat);
  *HA  = atan2(t1, t2) * Rad + 180.0;
  *Dec = *Dec * Rad;
}

void HorAppToEqu(double Azm, double Alt,
                  double* HA, double* Dec,
                  const double* cosLat, const double* sinLat)
{
  Apparent2Topocentric(&Alt);
  HorTopoToEqu(Azm, Alt, HA, Dec, cosLat, sinLat);
}

/* ======================================================================== */
/*  Step-distance helpers                                                    */
/* ======================================================================== */

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
  return static_cast<long>(*end - *start);
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
  return static_cast<long>(*end - *start);
}
