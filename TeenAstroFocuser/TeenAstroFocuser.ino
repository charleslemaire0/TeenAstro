#include "ConfigStorage.h"
#include "Global.h"
#include "ConfigStepper.h"
#include "Command.h"

bool printinfo = true;

void setup()
{
	// set the PWM and brake pins so that the direction pins  // can be used to control the motor:
  delay(1000);
  pinMode(ButtonAPin, INPUT);
  pinMode(ButtonBPin, INPUT);
	pinMode(StepPin, OUTPUT);
	pinMode(DirPin, OUTPUT);
  digitalWrite(DirPin, LOW);
  digitalWrite(StepPin, LOW);

	Serial.begin(19200);
  loadConfig();
	iniPos();
  iniStepper();
  sayHello();

}


void loop()
{
	auto mdirA = digitalRead(ButtonAPin);
	auto mdirB = digitalRead(ButtonBPin);
	update();
	if (mdirA == HIGH && mdirB == HIGH)
	{
		time_acc = 0;
		return;
	}
	// first stop the motor if the direction has changed
	if (mdirA != mdirAOld && mdirAOld == HIGH)
	{
		Command_stop(1);
		time_acc = 0;
		mdirAOld = mdirA;
		return;
	}
	if (mdirB != mdirBOld && mdirBOld == HIGH)
	{
		Command_stop(-1);
		time_acc = 0;
		mdirBOld = mdirB;
		return;
	}
	if (mdirA == HIGH)
	{
		Command_move(1, time_acc);
		mdirAOld = mdirA;
		return;
	}
	if (mdirB == HIGH)
	{
		Command_move(-1, time_acc);
		mdirBOld = mdirB;
		return;
	}
	Command_Check();
	time_acc = 0;
	mdirAOld = mdirA;
	mdirBOld = mdirB;
}

void sayHello(void)
{
	Serial.print("$ ");
	Serial.print(PROJECT);
	Serial.print(" ");
	Serial.print(BOARDINFO);
	Serial.print(" ");
	Serial.println(Version);
  lastEvent = millis() / updaterate + 2;
}