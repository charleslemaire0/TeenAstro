// All chip specific stuff
#ifdef __arm__
#include "Global.h"


void HAL_preInit(void)
{
  Serial.begin(BAUD);
  S_USB.attach_Stream((Stream *)&Serial, COMMAND_SERIAL);

#ifdef BOARD_240
  Serial2.begin(BAUD);
#endif
}

#ifdef BOARD_240
// currently use only one debug port
void HAL_debug0(uint8_t b)
{
  Serial2.write(b);
}
void HAL_debug1(uint8_t b)
{
}
#endif

void HAL_initSerial(void)
{
  Serial1.begin(BAUD);
  S_SHC.attach_Stream((Stream *)&Serial1, COMMAND_SERIAL1);
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
