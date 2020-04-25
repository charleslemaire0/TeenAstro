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

