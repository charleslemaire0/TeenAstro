// All chip specific stuff
#ifdef __ESP32__
#include "Global.h"
#include <SoftwareSerial.h>


#ifdef BOARD_esp32dev
const char BoardVersion[] = "esp32dev";

//EspSoftwareSerial::UART debugOut(2, 4); // rx, tx pins

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
#endif // BOARD_esp32dev

#ifdef BOARD_esp32s3
// Date and time functions using a DS3232 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "DS3232RTC.h"
//TwoWire Wire = TwoWire(0);
DS3232RTC rtc;

const char BoardVersion[] = "esp32ds3";
static unsigned long initialSystemTime = (50 * 365.25 * 24 * 3600);

const char* HAL_getBoardVersion(void)
{
  return (BoardVersion);
}

void HAL_preInit(void)
{
  Serial0.begin(115200);
  UARTSerial.setDebugOutput(true);
  UARTSerial.println("Started UARTSerial for debug");

  for (int k = 0; k < 10; k++)
  {
    neopixelWrite(RGB_BUILTIN, 0, 100, 0);
    delay(50);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    delay(50);
  }

  Wire.setPins(SDA, SCL);             // Start I2C
  Wire.begin();

}

void HAL_initSerial(void)
{ 
  SHCSerial.begin(BAUD);
  S_SHC.attach_Stream((Stream *)&SHCSerial, COMMAND_SERIAL);
  SHC1Serial.begin(BAUD);
  S_USB.attach_Stream((Stream *)&SHC1Serial, COMMAND_SERIAL1);
}

#define debugOut UARTSerial


void HAL_setRealTimeClock(unsigned long t)
{
  rtc.set(t - ((double) xTaskGetTickCount() / 1000.0));
}

unsigned long HAL_getRealTimeClock()
{
  return rtc.get();
}

void HAL_reboot(void)
{
  Serial.end();             // SHC
  Serial1.end();            // GNSS
  Serial0.end();            // HWCDC Serial

  EEPROM.commit();
  delay(1000);

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
#endif  // BOARD_esp32s3

#endif // __ESP32__