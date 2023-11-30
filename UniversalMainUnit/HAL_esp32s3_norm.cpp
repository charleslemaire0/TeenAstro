// All chip specific stuff
#ifdef __ESP32__
#include "Global.h"
#include <SoftwareSerial.h>


#ifdef BOARD_esp32s3_norm
const char BoardVersion[] = "esp32s3_norm";

// real-time clock not yet implemented
// Initial time is Jan 1 1970 + 50 years

static unsigned long initialSystemTime = (50 * 365.25 * 24 * 3600);

const char* HAL_getBoardVersion(void)
{
  return (BoardVersion);
}

void HAL_preInit(void)
{
  Serial.begin(BAUD);
  Serial1.begin(BAUD, SERIAL_8N1, SHCRx, SHCTx);
}

void HAL_initSerial(void)
{
  S_USB.attach_Stream((Stream *)&Serial, COMMAND_SERIAL);
  S_SHC.attach_Stream((Stream *)&Serial1, COMMAND_SERIAL1);
}

 
void HAL_setRealTimeClock(unsigned long t)
{
  initialSystemTime = t - ((double) xTaskGetTickCount() / 1000.0);
}

unsigned long HAL_getRealTimeClock()
{
  return initialSystemTime + ((double) xTaskGetTickCount() / 1000.0);
}

void HAL_reboot(void)
{
  Serial.end();
  Serial1.end();
  Serial2.end();
  delay(1000);
  EEPROM.commit();
  ESP.restart();
}

// We run at 80MHz 
hw_timer_t *timer1P;

void HAL_beginTimer(void f(void), unsigned long ticks)
{
  timer1P = timerBegin(0, 8, true);   // set divider to 8: count increments every µS / 10
  timerAttachInterrupt(timer1P, f, true);
  timerAlarmWrite(timer1P, ticks, true);      // interrupt every 10mS (sidereal)  
  timerAlarmEnable(timer1P);
}


void HAL_EEPROM_begin(void)
{
  #define EEPROM_SIZE 1024  
  EEPROM.begin(EEPROM_SIZE);
}
#endif // BOARD_esp32s3_norm

#endif // __ESP32__