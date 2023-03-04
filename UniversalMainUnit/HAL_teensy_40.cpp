// All chip specific stuff
#ifdef __arm__
#include "Global.h"

void HAL_setRealTimeClock(unsigned long t)
{
  Teensy3Clock.set(t);
}
unsigned long HAL_getRealTimeClock()
{
  return Teensy3Clock.get();
}
  
#endif
