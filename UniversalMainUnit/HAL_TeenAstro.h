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
