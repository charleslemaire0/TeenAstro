#include "ConfigStorage.h"
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"

uint8_t mdirIN = LOW;
uint8_t mdirOUT = LOW;
void setup()
{
	// set the PWM and brake pins so that the direction pins  // can be used to control the motor:
  delay(500);

	pinMode(StepPin, OUTPUT);
	pinMode(DirPin, OUTPUT);
  digitalWrite(DirPin, LOW);
  digitalWrite(StepPin, LOW);

	Serial.begin(115200);
  Serial.setTimeout(5);
#ifdef VERSION22
  Serial2.setRX(FocuserRX);
  Serial2.setTX(FocuserTX);
  Serial2.begin(115200);
  Serial2.setTimeout(5);
#endif
#ifdef VERSION23
  Serial1.setRX(FocuserRX);
  Serial1.setTX(FocuserTX);
  Serial1.begin(115200);
  Serial1.setTimeout(5);
#endif


  loadConfig();
	iniPos();
  iniStepper();
}


void loop()
{

  serCom0.Get_Command();
  serComSHC.Get_Command();

  serCom0.MoveRequest();
  serComSHC.MoveRequest();

  
	if (mdirOUT == HIGH && mdirIN == HIGH)
	{
		time_acc = 0;
		return;
	}
	// first stop the motor if the direction has changed
	else if (mdirOUT != mdirOUTOld && mdirOUTOld == HIGH)
	{
		Command_stop(1);
		time_acc = 0;
    mdirOUTOld = mdirOUT;
		return;
	}
	else if (mdirIN != mdirINOld && mdirINOld == HIGH)
	{
		Command_stop(-1);
		time_acc = 0;
    mdirINOld = mdirIN;
		return;
	}
	else if (mdirOUT == HIGH)
	{
		Command_move(1, time_acc);
    mdirOUTOld = mdirOUT;
		return;
	}
	else if (mdirIN == HIGH)
	{
		Command_move(-1, time_acc);
    mdirINOld = mdirIN;
		return;
	}

  serCom0.Command_Check();
  serComSHC.Command_Check();
	time_acc = 0;
	mdirOUTOld = mdirOUT;
	mdirINOld = mdirIN;
}

