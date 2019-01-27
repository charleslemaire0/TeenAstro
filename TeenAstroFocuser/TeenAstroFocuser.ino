
#include <TMCStepper_UTILITY.h>
#include <TMCStepper.h>
#include <TMC26XStepper.h>
#include <TeenAstroStepper.h>
#include "ConfigStorage.h"
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"


float temperature = 0;

void setup()
{
	// set the PWM and brake pins so that the direction pins  // can be used to control the motor:
  pinMode(LEDPin, OUTPUT);
  for (int k = 0; k < 10; k++)
  {
    analogWrite(LEDPin, 255);
    delay(100);
    analogWrite(LEDPin, 0);
    delay(100);
  }

	pinMode(StepPin, OUTPUT);
	pinMode(DirPin, OUTPUT);
  digitalWrite(DirPin, LOW);
  digitalWrite(StepPin, LOW);
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
  lastTemp = tempSensors.getTempCByIndex(0);
  tempSensors.setWaitForConversion(false);
  iniMot();
  iniPos();
  analogWrite(LEDPin,16);
}


void loop()
{
  Command_Run();
  writePos();

  if (serComSHC.Get_Command())
  {
    if (serComSHC.MoveRequest())
      return;
    serComSHC.Command_Check();
  }
  if (serCom0.Get_Command())
  {
    if (serCom0.MoveRequest())
      return;
    serCom0.Command_Check();
  }


  //Command_Run();
}

