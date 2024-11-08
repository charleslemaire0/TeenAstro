// All chip specific stuff
#ifdef __ESP32__
#include "Global.h"
#include <SoftwareSerial.h>
const char BoardVersion[] = "esp32dev";

EspSoftwareSerial::UART debugOut(2, 4); // rx, tx pins

// ESP32 does not have a real-time clock
// Initial time is Jan 1 1970 + 50 years

static unsigned long initialSystemTime = (50 * 365.25 * 24 * 3600);

const char* HAL_getBoardVersion(void)
{
  return (BoardVersion);
}

void HAL_preInit(void)
{
  Serial1.begin(57600);
  debugOut.begin(57600);
}

void HAL_debug0(uint8_t b)
{
  Serial1.write(b);
}

void HAL_debug1(uint8_t b)
{
  debugOut.write(b);
}

void HAL_initSerial(void)
{
  Serial.begin(BAUD);
  S_SHC.attach_Stream((Stream *)&Serial, COMMAND_SERIAL);

  Serial2.begin(BAUD);
  S_USB.attach_Stream((Stream *)&Serial2, COMMAND_SERIAL1);
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

#endif

