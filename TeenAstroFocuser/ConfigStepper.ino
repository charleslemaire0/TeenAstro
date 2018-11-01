// 
// 
// 

#include "ConfigStepper.h"
#include "global.h"
#include "BasicStepperDriver.h"
#include "DS1302.h"

BasicStepperDriver myStepper(200, DirPin, StepPin);

void iniMot()
{
  myStepper.setReverse(storage.reverse);
  myStepper.setSpeed(storage.minSpeed);
}

void MoveTo(unsigned long pos)
{
	if (inlimit (pos))
	{
		long delta = (long)pos - (long)position;
		currSpeed = storage.minSpeed;
		StartMove( delta);
		currSpeed = 0;
	}
}

void StartMove( long&n)
{
	if (n == 0)
	{
		return;
	}
	int sign = (n<0) ? -1 : 1;
	long sumsteps = 0;
	unsigned int stepperspeedini = currSpeed;
	double t = 0;
	int status = 0;

	while (status == 0)
	{
		serCom0.updateGoto();
    serComSHC.updateGoto();
		if (halt)
		{
			status = 1;
			break;
		}
    myStepper.move(sign);
		sumsteps += sign;
		position += sign;
		writePos();
		if (abs(2 * (sumsteps + sign)) > abs(n))
		{
			stepperspeedini = currSpeed;
			status = 1;
			break;
		}

		t += 1. / currSpeed;
		unsigned int stepperspeednew = stepperspeedini + (unsigned int)storage.cmdAcc * t / 10.;
		if (stepperspeednew > storage.maxSpeed)
		{
			status = 2;
			break;
		}
		else
		{
			currSpeed = stepperspeednew;
			myStepper.setSpeed(currSpeed);
		}
	}
	long rest = n - sumsteps;
	if (status == 2)
	{
		niter(rest - sumsteps, sign);
		rest = sumsteps;
	}
	FinishMove( rest);
}


void FinishMove( long n)
{
	if (n == 0)
	{
		return;
	}
	int sign = (n<0) ? -1 : 1;
	long sumsteps = 0;
	int stepperspeedini = currSpeed;
	double t = 0;
	while (sumsteps != n)
	{
    serCom0.updateGoto();
    serComSHC.updateGoto();
		if (halt)
			return;
    myStepper.move(sign);
		sumsteps += sign;
		position += sign;
		writePos();
		t += 1. / currSpeed;
		unsigned int stepperspeednew = stepperspeedini - (unsigned int)storage.cmdAcc * t / 10.;
		if (stepperspeednew < storage.minSpeed)
		{
			currSpeed = storage.minSpeed;
			myStepper.setSpeed(currSpeed);
			break;
		}
		currSpeed = stepperspeednew;
		myStepper.setSpeed(stepperspeednew);
	}
	
	niter(n - sumsteps,sign);
}

void niter(long m, int sign)
{
	while (m != 0)
	{
    serCom0.updateGoto();
    serComSHC.updateGoto();
		if (halt)
		{
			return;
		}
    myStepper.move(sign);
		position += sign;
		writePos();
		m -= sign;
	}
}

void Go(int stepperspeedini, int sign, double& t)
{
	if (currSpeed != storage.maxSpeed)
	{
    if (currSpeed == 0)
      currSpeed = FocCmd_minSpeed;
		double tnew = t + 1. / currSpeed;
    double  a = storage.manAcc * tnew / 10.;
    unsigned int stepperspeednew = min(max(stepperspeedini + a + 0.5*a*a, storage.minSpeed), storage.maxSpeed);
		if (stepperspeednew < storage.maxSpeed)
		{
			t = tnew;
			currSpeed = stepperspeednew;
		}

    myStepper.setSpeed(currSpeed);
	}

  myStepper.move(sign);
	position += sign;
	writePos();

}

void Stop(int sign)
{
	double t = 0;

	unsigned int stepperspeedini = currSpeed;
	while (currSpeed > storage.minSpeed)
	{
    serCom0.updateGoto();
    serComSHC.updateGoto();
		t += 1. / currSpeed;
		if (stepperspeedini < (long)storage.manDec * t / 10.)
		{
			break;
		}
    currSpeed = stepperspeedini - storage.manDec * t / 10.;
		myStepper.setSpeed(currSpeed);
		unsigned long nextposition = position;
		nextposition += sign;
		if (!inlimit(nextposition))
		{
			break;
		}
    myStepper.move(sign);
		position = nextposition;
		writePos();
	}
	currSpeed = 0;
	myStepper.setSpeed(storage.minSpeed);
}

bool inlimit(unsigned long pos)
{
	return 0UL <= pos && pos <= (unsigned long)storage.maxPosition;
}

void writePos()
{
	unsigned long pos[3] = { position,position, position };
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
		position = posini[1];
	}
	else
	{
		position = storage.startPosition;
	}
}