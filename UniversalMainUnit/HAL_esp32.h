// All chip specific stuff
#include <Arduino.h>
#define ISR(f) void IRAM_ATTR f(void) 

#define MIN_INTERRUPT_PERIOD      5 // microseconds

#define MAX_TEENASTRO_SPEED       (1000000 / MIN_INTERRUPT_PERIOD)  // in µsteps / S
#define BacklashTakeupRate         5 // backlash takeup rate (in multiples of the sidereal rate) - not used at this time
