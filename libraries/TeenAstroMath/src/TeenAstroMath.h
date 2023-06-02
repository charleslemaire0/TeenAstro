#pragma once
#ifndef __TeenAstroMath_h__
#define __TeenAstroMath_h__

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.295779513082320876798154814105
#endif 
#ifndef HOUR_TO_RAD
#define HOUR_TO_RAD 0.26179938779914943653855361527329
#endif
#ifndef RAD_TO_HOUR
#define RAD_TO_HOUR 3.8197186342054880584532103209403
#endif 


#define Rad 57.29577951308232

enum PierSide
{
  PIER_NOTVALID, PIER_EAST, PIER_WEST
};

double frac(double v);
double cot(double n);
// integer numeric conversion with error checking
bool atoi2(char *a, int *i);
bool atoui2(char* a, unsigned int* i);

double haRange(double d);
double haRangeRad(double d);
double AzRange(double d);
double degRange(double d);

double angDist(double h, double d, double h1, double d1);

// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion
//Topocentric Apparent convertions
// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
// !!!obsolete for Main unit!!!
double trueRefrac(double Alt, double Pressure = 1010., double Temperature = 10.);
void Topocentric2Apparent(double *Alt, double Pressure = 1010., double Temperature = 10.);
void Apparent2Topocentric(double *Alt, double Pressure = 1010., double Temperature = 10.);
void EquToHor(double HA, double Dec, bool refraction, double* Azm, double* Alt, const double* cosLat, const double* sinLat);
void HorTopoToEqu(double Azm, double Alt, double *HA, double *Dec, const double *cosLat, const double *sinLat);
void HorAppToEqu(double Azm, double Alt, double *HA, double *Dec, const double *cosLat, const double *sinLat);
void InsrtAngle2Angle(double *AngleAxis1, double *AngleAxis2, PierSide *Side);
void Angle2InsrtAngle(PierSide Side, double *AngleAxis1, double *AngleAxis2, const double *Lat, const double poleAxis1);

//steps operations
long distStepAxis1(long* start, long* end);
long distStepAxis1(volatile long* start, volatile long* end);
long distStepAxis1(volatile long* start, volatile double* end);
long distStepAxis2(long* start, long* end);
long distStepAxis2(volatile long* start, volatile long* end);
long distStepAxis2(volatile long* start, volatile double* end);

#endif