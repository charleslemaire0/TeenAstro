// All chip specific stuff
#ifdef __arm__
#include "Global.h"

#ifdef BOARD_240
const char BoardVersion[] = "240";
#elif BOARD_250
const char BoardVersion[] = "250";
#else
const char BoardVersion[] = "unknown";
#endif


const char* HAL_getBoardVersion(void)
{
  return (BoardVersion);
}

void HAL_preInit(void)
{
//  Serial.begin(BAUD); // not needed according to Teensy documentation
  S_USB.attach_Stream((Stream *)&Serial, COMMAND_SERIAL);
  Serial1.begin(BAUD);
  S_SHC.attach_Stream((Stream *)&Serial1, COMMAND_SERIAL1);

  // process SHC initial commands (GVP etc.) to avoid losing connection 
  // this is because FreeRTOS initialization is very slow (7sec) for unknown reason
  for (int j = 0; j<800; j++)
  {
    processCommands();
    delay(1);
  }

}

void HAL_initSerial(void)
{
// has already been done in preInit  
//  Serial1.begin(BAUD);
//  S_SHC.attach_Stream((Stream *)&Serial1, COMMAND_SERIAL1);
}

void HAL_setRealTimeClock(unsigned long t)
{
  Teensy3Clock.set(t);
}
unsigned long HAL_getRealTimeClock()
{
  return Teensy3Clock.get();
}
void HAL_reboot(void)
{
  Serial.end();
  Serial1.end();
  Serial2.end();
  delay(1000);

  _reboot_Teensyduino_();
}

IntervalTimer  itimer1;
void HAL_beginTimer(ISR(f), unsigned long ticks)
{
  // interrupt every 10mS (sidereal) or 10000 µS (sidereal)
  itimer1.begin(f, ticks/10);  // argument is in µS   
  itimer1.priority(32);
}


void HAL_EEPROM_begin(void)
{
  EEPROM.begin();
}

#endif
