#include "ConfigStorage.h"
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"

uint8_t mdirIN = LOW;
uint8_t mdirOUT = LOW;

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
#ifdef VERSION230
  Serial1.setRX(FocuserRX);
  Serial1.setTX(FocuserTX);
  Serial1.begin(56000);
  Serial1.setTimeout(10);
#endif

  loadConfig();
  iniMot();
	iniPos();
  analogWrite(LEDPin,16);
}


void loop()
{
  writePos();
  if (serComSHC.Get_Command())
  {
    serComSHC.MoveRequest();
  }
  if (serCom0.Get_Command())
  {
    serCom0.MoveRequest();
  }
	if (mdirOUT == HIGH && mdirIN == HIGH)
	{
		return;
	}
	else if (mdirOUT == HIGH && mdirOUTOld != mdirOUT)
	{
    modeMan();
		stepper.moveTo(maxPosition->get());
    mdirOUTOld = mdirOUT;
		return;
	}
	else if (mdirIN == HIGH && mdirINOld != mdirIN)
	{
    modeMan();
    stepper.moveTo(0);
    mdirINOld = mdirIN;
		return;
	}
  else if ((mdirIN == LOW && mdirINOld != mdirIN) || (mdirOUT == LOW && mdirOUTOld != mdirOUT))
  {
    stepper.setAcceleration(100.*manDec->get());
    stepper.stop();
    mdirINOld = mdirIN;
    mdirOUTOld = mdirOUT;
    return;
  }
	mdirOUTOld = mdirOUT;
	mdirINOld = mdirIN;
  serComSHC.Command_Check();
  serCom0.Command_Check();
  Command_Run();
}

