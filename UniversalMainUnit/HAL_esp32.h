// All chip specific stuff
#ifdef __ESP32__

#include <Arduino.h>
#define ISR(f) void IRAM_ATTR f(void) 

#define MIN_INTERRUPT_PERIOD      6 // microseconds

#define MAX_TEENASTRO_SPEED       (1000000 / MIN_INTERRUPT_PERIOD)  // in µsteps / S
#define BacklashTakeupRate         5 // backlash takeup rate (in multiples of the sidereal rate) - not used at this time
void      HAL_debug0(uint8_t b);
void      HAL_debug1(uint8_t b);

#endif