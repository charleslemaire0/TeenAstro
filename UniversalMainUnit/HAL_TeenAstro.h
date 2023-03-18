// HAL.h
#ifdef BOARD_250        // the teensy40 board
#include "HAL_teensy_40.h"   
#include "HAL_pins_250.h"    
#endif

#ifdef BOARD_teensy41    	// Standalone teensy41
#include "HAL_teensy_40.h" 	// same as teensy40  
#include "HAL_pins_teensy41.h"    
#endif

#ifdef BOARD_esp32dev      // Standalone esp32dev
#include "HAL_esp32.h"         
#include "HAL_pins_esp32.h"  
#endif


struct EE_Site
{
  float lat;
  float lon;
  uint16_t elev;
  uint8_t tz;
  char name[16];
};

void          HAL_setRealTimeClock(unsigned long t);
unsigned long HAL_getRealTimeClock(void);
void          HAL_reboot(void);
void          HAL_beginTimer(void f(void), unsigned long);
void          HAL_EEPROM_begin(void);

