// -----------------------------------------------------------------------------------
// fixed point math data type

#include "Arduino.h"

#ifndef FPoint_h
#define FPoint_h

typedef struct {
  uint32_t f;
  uint32_t m;
} fixedBase_t;

typedef union {
  fixedBase_t part;
  uint64_t fixed;
} fixed_t;



// floating point range of +/-8191.999999x
uint64_t doubleToFixed(double d)
{
  fixed_t x;
  x.fixed = (long)(d * 262144.0);   // shift 18 bits
  x.fixed = x.fixed << 14;
  return x.fixed;
}

// floating point range of +/-8191.999999x
double fixedToDouble(fixed_t a)
{
  long    l = a.fixed >> 14;           // shift 14 bits
  return ((double)l / (262144.0));    // and 18 more, for 32 bits total
}
#endif
