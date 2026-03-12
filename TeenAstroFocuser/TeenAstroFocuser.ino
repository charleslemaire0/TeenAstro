#include <DallasTemperature.h>
#include <TMCStepper_UTILITY.h>
#include <TMCStepper.h>
#include <TMC26XStepper.h>
#include <TeenAstroStepper.h>
#include "ConfigStorage.h"
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"

IntervalTimer tickTimer;
bool isMoving = false;
void setup()
{
  // set the PWM and brake pins so that the direction pins  // can be used to control the motor:
  pinMode(LEDPin, OUTPUT);
  for (int k = 0; k < 10; k++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(10);
    digitalWrite(LEDPin, LOW);
    delay(50);
  }
  pinMode(_StepPin, OUTPUT);
  pinMode(_DirPin, OUTPUT);
  digitalWrite(_DirPin, LOW);
  digitalWrite(_StepPin, LOW);
  digitalWrite(LEDPin, HIGH);
  loadConfig();
  // OneWire/Dallas: set pin state and allow bus/sensor to settle so begin() can find the device
  pinMode(TempPin, INPUT_PULLUP);
  delay(250);
  for (int tries = 0; tries < 3 && tempSensors.getDeviceCount() == 0; tries++)
  {
    if (tries > 0) delay(100);
    tempSensors.begin();
  }
  tempSensors.setResolution(11);
  tempSensors.setWaitForConversion(true);
  tempSensors.requestTemperaturesByIndex(0);
  lastTemp = max(min(tempSensors.getTempCByIndex(0), 99.9999), -99.9999);
  tempSensors.setWaitForConversion(false);
  tickTimer.priority(255); // lowest priority, potentially long caclulations need to be interruptable by TeensyStep
  tickTimer.begin(updateTemperature, 5000000);
  iniMot();
  iniPos();

  Serial.begin(9600);
#if TEMP_DEBUG
  delay(100);
  Serial.println("[TEMP] --- Focuser Dallas temperature debug ---");
  Serial.print("[TEMP] board=");
  Serial.print(BOARDINFO);
  Serial.print(" OneWire pin=");
  Serial.println(TempPin);
  Serial.print("[TEMP] oneWire.reset()=");
  Serial.println(oneWire.reset() ? "1 (presence)" : "0 (no device)");
  Serial.print("[TEMP] setup getDeviceCount()=");
  Serial.println(tempSensors.getDeviceCount());
  Serial.print("[TEMP] setup getTempCByIndex(0)=");
  Serial.println(tempSensors.getTempCByIndex(0));
  Serial.print("[TEMP] setup lastTemp=");
  Serial.println(lastTemp);
  Serial.println("[TEMP] (periodic: reset, getDeviceCount, rawC, lastTemp every 10s)");
#endif
#if VERSION == 220
  Serial2.setRX(FocuserRX);
  Serial2.setTX(FocuserTX);
  Serial2.begin(56000);
  Serial2.setTimeout(10);
#endif
#if VERSION == 230 || VERSION == 240 
  Serial1.setRX(FocuserRX);
  Serial1.setTX(FocuserTX);
  Serial1.begin(56000);
  Serial1.setTimeout(10);
#endif
  digitalWrite(LEDPin, LOW);
}


void loop()
{
  isMoving = controller.isRunning() || rotateController.isRunning();
  writePos();
  pid();
  if (serComSHC.Do())
    return;
  if (serCom0.Do())
    return;
}

// Two-phase: request conversion one tick, read result next tick (5s later) so we don't read
// before conversion completes. With setWaitForConversion(false), reading immediately after
// request returns -127 (no valid reading). lastTemp -99.99 means no sensor or read error.
void updateTemperature()
{
  if (!isMoving)
  {
    static bool requestPhase = true;
    if (requestPhase)
    {
      tempSensors.requestTemperaturesByIndex(0);
    }
    else
    {
      float rawC = tempSensors.getTempCByIndex(0);
      lastTemp = max(min(rawC, 99.9999F), -99.9999F);
#if TEMP_DEBUG
      Serial.print("[TEMP] reset=");
      Serial.print(oneWire.reset() ? "1" : "0");
      Serial.print(" getDeviceCount()=");
      Serial.print(tempSensors.getDeviceCount());
      Serial.print(" rawC=");
      Serial.print(rawC);
      Serial.print(" lastTemp=");
      Serial.println(lastTemp);
#endif
    }
    requestPhase = !requestPhase;
  }
}