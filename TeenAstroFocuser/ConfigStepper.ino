// 
// 
// 

#include "ConfigStepper.h"
#include "global.h"
#include <BasicStepperDriver.h>
#include "DS1302.h"

const int stepsPerMotorRevolution = 200;  // change this to fit the number of steps per revolution
BasicStepperDriver myStepper(stepsPerMotorRevolution, DirPin, StepPin);

void iniStepper()
{
  myStepper.setMicrostep(micro);
}


void MoveTo(int pos)
{
	if (inlimit (pos))
	{
		int delta = pos - position;
		if (storage.reverse)
		{
			delta = -delta;
		}
		currSpeed = storage.minSpeed;
		StartMove( delta);
		currSpeed = 0;
	}
}

void StartMove( int&n)
{
	if (n == 0)
	{
		return;
	}
	int sign = (n<0) ? -1 : 1;
	int sumsteps = 0;
	int stepperspeedini = currSpeed;
	double t = 0;
	int status = 0;

	while (status == 0)
	{
		updateGoto();
		if (halt)
		{
			status = 1;
			break;
		}
    myStepper.move(sign*micro);
		sumsteps += sign;
		position += storage.reverse ? -sign : sign;
		writePos();
		if (abs(2 * (sumsteps + sign)) > abs(n))
		{
			stepperspeedini = currSpeed;
			status = 1;
			break;
		}

		t += 1. / (currSpeed*stepsPerMotorRevolution);
		int stepperspeednew = stepperspeedini + storage.cmdAcc * t;
		if (stepperspeednew > storage.maxSpeed)
		{
			status = 2;
			break;
		}
		else
		{
			currSpeed = stepperspeednew;
			myStepper.setRPM(currSpeed);
		}
	}
	int rest = n - sumsteps;
	if (status == 2)
	{
		niter(rest - sumsteps, sign);
		rest = sumsteps;
	}
	FinishMove( rest);
}


void FinishMove( int n)
{
	if (n == 0)
	{
		return;
	}
	int sign = (n<0) ? -1 : 1;
	int sumsteps = 0;
	int stepperspeedini = currSpeed;
	double t = 0;
	while (sumsteps != n)
	{
		updateGoto();
		if (halt)
			return;
		myStepper.move(sign*micro);
		sumsteps += sign;
		position += storage.reverse ? -sign : sign;
		writePos();
		t += 1. / (currSpeed*stepsPerMotorRevolution);
		int stepperspeednew = stepperspeedini - storage.cmdAcc * t;
		if (stepperspeednew < storage.minSpeed)
		{
			currSpeed = storage.minSpeed;
			myStepper.setRPM(currSpeed);
			break;
		}
		currSpeed = stepperspeednew;
		myStepper.setRPM(stepperspeednew);
	}
	
	niter(n - sumsteps,sign);
}

void niter(int m, int sign)
{
	while (m != 0)
	{
		updateGoto();
		if (halt)
		{
			return;
		}
		myStepper.move(sign*micro);
		position += storage.reverse ? -sign : sign;
		writePos();
		m -= sign;
	}
}

void Go(int stepperspeedini, int sign, double& t)
{
	if (currSpeed == 0)
	{
		currSpeed = storage.minSpeed;
	}

	if (currSpeed != storage.maxSpeed)
	{
		double tnew = t + 1. / (currSpeed*stepsPerMotorRevolution);
		double  a = storage.manAcc * tnew;
		int stepperspeednew = stepperspeedini + a + 0.5*a*a;
		if (stepperspeednew < storage.maxSpeed)
		{
			t = tnew;
			currSpeed = stepperspeednew;
		}
		else
		{
			currSpeed = storage.maxSpeed;
		}
		myStepper.setRPM(currSpeed);
	}

	myStepper.move(sign*micro);
	position += storage.reverse ? -sign : sign;
	writePos();

}

void Stop(int sign)
{
	double t = 0;
	int stepperspeed = currSpeed;
	int stepperspeedini = currSpeed;
	while (currSpeed != storage.minSpeed)
	{
		t += 1. / (currSpeed*stepsPerMotorRevolution);
		stepperspeed = stepperspeedini - storage.manDec * t;

		if (stepperspeed < storage.minSpeed )
		{
			currSpeed = storage.minSpeed;
			myStepper.setRPM(currSpeed);
			break;
		}
		currSpeed = stepperspeed;
		myStepper.setRPM(currSpeed);
		int nextposition = position;
		nextposition += storage.reverse ? -sign : sign;
		if (!inlimit(nextposition))
		{
			break;
		}
		myStepper.move(sign*micro);
		position = nextposition;
		writePos();
	}
	currSpeed = 0;
	myStepper.setRPM(currSpeed);
}

bool inlimit(int pos)
{
	return 0 <= pos && pos <= storage.maxPosition;
}

void writePos()
{
	unsigned int pos[3] = { position,position, position };
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
	unsigned int* posini = (unsigned int*)ram;
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