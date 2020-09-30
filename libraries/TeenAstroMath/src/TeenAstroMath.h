#pragma once
#ifndef __TeenAstroMath_h__
#define __TeenAstroMath_h__
#define Rad 57.29577951308232

enum PierSide
{
  PIER_NOTVALID, PIER_EAST, PIER_WEST
};

double frac(double v);
double cot(double n);
// integer numeric conversion with error checking
bool atoi2(char *a, int *i);

double haRange(double d);
double AzRange(double d);
double degRange(double d);

double angDist(double h, double d, double h1, double d1);

// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion
//Topocentric Apparent convertions
// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)

double trueRefrac(double Alt, double Pressure = 1010., double Temperature = 10.);
void Topocentric2Apparent(double *Alt, double Pressure = 1010., double Temperature = 10.);
void Apparent2Topocentric(double *Alt, double Pressure = 1010., double Temperature = 10.);
void EquToHorTopo(double HA, double Dec, double *Azm, double *Alt, const double *cosLat, const double *sinLat);
void EquToHorApp(double HA, double Dec, double *Azm, double *Alt, const double *cosLat, const double *sinLat);
void HorTopoToEqu(double Azm, double Alt, double *HA, double *Dec, const double *cosLat, const double *sinLat);
void HorAppToEqu(double Azm, double Alt, double *HA, double *Dec, const double *cosLat, const double *sinLat);
void InsrtAngle2Angle(double *AngleAxis1, double *AngleAxis2, PierSide *Side);
void Angle2InsrtAngle(PierSide Side, double *AngleAxis1, double *AngleAxis2, const double *Lat);

//steps operations
long distStepAxis1(long* start, long* end);
long distStepAxis1(volatile long* start, volatile long* end);
long distStepAxis1(volatile long* start, volatile double* end);
long distStepAxis2(long* start, long* end);
long distStepAxis2(volatile long* start, volatile long* end);
long distStepAxis2(volatile long* start, volatile double* end);

#endif