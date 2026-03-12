// HAL.h
#ifdef BOARD_250        // the teensy40 board
#include "HAL_teensy_40.h"   
#include "HAL_pins_250.h"    
#endif

#ifdef BOARD_240        // the old 240 board with Teensy 4.0 and without ST4 interface - debug only!
#include "HAL_teensy_40.h"   
#include "HAL_pins_240.h"    
#endif

#ifdef BOARD_teensy41    	// Standalone teensy41
#include "HAL_teensy_40.h" 	// same as teensy40  
#include "HAL_pins_teensy41.h"    
#endif

#ifdef BOARD_esp32dev      // Standalone esp32dev
#include "HAL_esp32.h"      // Common ESP32          
#include "HAL_pins_esp32.h"  
#endif

#ifdef BOARD_esp32s3      // Standalone esp32s3
#include "HAL_esp32.h"      // Common ESP32          
#include "HAL_pins_esp32s3.h"  
#endif

#ifdef BOARD_esp32s3_norm // Norman's TeenAstro debug board
#include "HAL_esp32.h"      // Common ESP32          
#include "HAL_pins_esp32s3_norm.h"  
#endif



struct EE_Site
{
  float lat;
  float lon;
  uint16_t elev;
  uint8_t tz;
  char name[16];
};

void          HAL_preInit(void);
void          HAL_initSerial(void);
void          HAL_setRealTimeClock(unsigned long t);
const char*   HAL_getBoardVersion(void);
unsigned long HAL_getRealTimeClock(void);
void          HAL_reboot(void);
void          HAL_beginTimer(void f(void), unsigned long);
void          HAL_EEPROM_begin(void);

