#pragma once

class Mc5160 : public MotionControl
{
public:
  TMC5160Stepper *drvP;
  SemaphoreHandle_t mutex = 0;
	void setCurrentPos(long);
  void syncPos(long);
	long getCurrentPos(void);
	long getTargetPos(void);
	double getSpeed(void);
	void setAmax(long);	
	void setVmax(double);	
	void setTargetPos(long targetPos);
	bool positionReached(void);
	bool isMoving(void);
	void setDir(bool);
	void abort(void);
	void resetAbort(void);
	void initMc5160(TMC5160Stepper *driverP, SemaphoreHandle_t mtx, long clkFreq);
  void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId); 
  void setRatios(long f);
  void enable(void);
  void disable(void);
  double speedRatio, accelRatio;
};
