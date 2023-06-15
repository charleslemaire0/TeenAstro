// All chip specific stuff
#ifdef __arm__
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#define ISR(f)  void f(void)

#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "timers.h"


// Teensy40 runs at 600MHz (!)
#define MIN_INTERRUPT_PERIOD       5 // microseconds between interrupts - can probably be lower
#define BacklashTakeupRate         5 // backlash takeup rate (in multiples of the sidereal rate) - not used at this time
#define MAX_TEENASTRO_SPEED       (1000000 / MIN_INTERRUPT_PERIOD)  // in µsteps / S

#ifdef BOARD_240
void      HAL_debug0(uint8_t b);
void      HAL_debug1(uint8_t b);
#else
#define HAL_debug0(b)
#define HAL_debug1(b)
#endif

#endif