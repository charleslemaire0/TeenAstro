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



// floating point range of +/-511.999999x
uint64_t doubleToFixed(double d)
{
  fixed_t x;
  x.fixed = (long)(d * 4194304.0);   // shift 22 bits
  x.fixed = x.fixed << 10;
  return x.fixed;
}

// floating point range of +/-511.999999x
double fixedToDouble(fixed_t a)
{
  long    l = a.fixed >> 10;           // shift 10 bits
  return ((double)l / (4194304.0));    // and 22 more, for 32 bits total
}
#endif
