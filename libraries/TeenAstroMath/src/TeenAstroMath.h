#pragma once
#ifndef __TeenAstroMath_h__
#define __TeenAstroMath_h__
#define Rad 57.29577951308
double frac(double v);
double cot(double n);
// integer numeric conversion with error checking
bool atoi2(char *a, int *i);


// -----------------------------------------------------------------------------------------------------------------------------
// Coordinate conversion
//Topocentric Apparent convertions
// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)

double trueRefrac(double Alt, double Pressure = 1010., double Temperature = 10.);
void Topocentric2Apparent(double *Alt, double Pressure = 1010., double Temperature = 10.);
void Apparent2Topocentric(double *Alt, double Pressure = 1010., double Temperature = 10.);



#endif