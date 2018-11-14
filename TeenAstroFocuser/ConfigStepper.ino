// 
// 
// 

#include "ConfigStepper.h"
#include "global.h"



void iniMot()
{
  pinMode(CSPin, OUTPUT);
  digitalWrite(CSPin, HIGH);
  driver.begin();             // Initiate pins and registeries
  driver.rms_current(10.*curr->get());    // Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  driver.stealthChop(1);      // Enable extremely quiet stepping
  driver.stealth_autoscale(1);
  driver.microsteps(pow(2, micro->get()));
  driver.interpolate(1);
  stepper.setMaxSpeed(highSpeed->get()*pow(2,micro->get())); // 100mm/s @ 80 steps/mm
  stepper.setAcceleration(manAcc->get()*100); // 2000mm/s^2
  stepper.setEnablePin(EnablePin);
  stepper.setPinsInverted(reverse->get(), false, true);
  stepper.enableOutputs();
}
void Run()
{
  stepper.run();
}

void MoveTo(long pos)
{
	if (inlimit (pos))
	{
    stepper.moveTo(pos);
    stepper.enableOutputs();
	}
}

void Stop()
{
  stepper.stop();
}

bool inlimit(unsigned long pos)
{
	return 0UL <= pos && pos <= (unsigned long)maxPosition->get();
}

void writePos()
{
  long posi = stepper.currentPosition();
	long pos[3] = { posi,posi, posi };
	rtc->writeRamBulk((uint8_t*)pos, sizeof(pos));
}

void iniPos()
{
	if (rtc == NULL)
	{
		rtc = new DS1302(kCePin, kIoPin, kSclkPin);
	}

	rtc->writeProtect(false);
	uint8_t ram[DS1302::kRamSize];
	rtc->readRamBulk(ram, DS1302::kRamSize);
	unsigned long* posini = (unsigned long*)ram;
	//char buf[10];
	//for (int k = 0; k < 3, k++;)
	//{
	//	sprintf(buf, "$%06d ", posini[0]);
	//	Serial.println(buf);
	//}
	if (posini[0] == posini[1] && inlimit(posini[0]))
	{
    stepper.setCurrentPosition(posini[1]);
	}
	else
	{
    stepper.setCurrentPosition((long)startPosition->get());
	}
}