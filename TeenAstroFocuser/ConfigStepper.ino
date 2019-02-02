// 
// 
// 

#include "ConfigStepper.h"
#include "global.h"



void iniMot()
{
  pinMode(CSPin, OUTPUT);
  digitalWrite(CSPin, HIGH);
  teenAstroStepper.initMotor(static_cast<Motor::Motor_Driver>(TMC), 200, EnablePin, CSPin, DirPin, StepPin, 10.*curr->get(), micro->get());
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
  if (stepper.speed() == 0)
  {
    long posi = stepper.currentPosition();
    if (posi == oldposition)
      return;
    oldposition = posi;
    long pos[3] = { posi,posi, posi };
    rtc->writeRamBulk((uint8_t*)pos, sizeof(pos));
  }
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