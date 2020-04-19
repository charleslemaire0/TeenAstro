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
  for (int k = 0; k < 20; k++)
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
  Serial.begin(9600);

#ifdef VERSION220
  Serial2.setRX(FocuserRX);
  Serial2.setTX(FocuserTX);
  Serial2.begin(56000);
  Serial2.setTimeout(10);
#endif
#if defined(VERSION230) || defined (VERSION240)
  Serial1.setRX(FocuserRX);
  Serial1.setTX(FocuserTX);
  Serial1.begin(56000);
  Serial1.setTimeout(10);
#endif

  loadConfig();
  tempSensors.begin();
  tempSensors.setResolution(12);
  tempSensors.requestTemperaturesByIndex(0);
  lastTemp = max(min(tempSensors.getTempCByIndex(0), 99.9999), -99.9999);
  tempSensors.setWaitForConversion(false);
  tickTimer.priority(255); // lowest priority, potentially long caclulations need to be interruptable by TeensyStep
  tickTimer.begin(updateTemperature, 5000000);
  iniMot();
  iniPos();
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

void updateTemperature()
{
  if (!isMoving)
  {
    tempSensors.requestTemperaturesByIndex(0);
    lastTemp = max(min(tempSensors.getTempCByIndex(0), 99.9999F), -99.9999F);
    lastTempTick = millis();
  }
}