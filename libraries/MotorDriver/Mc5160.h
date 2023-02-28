#pragma once

class Mc5160 : public MotionControl
{
public:
  TMC5160Stepper *drvP;
	void setCurrentPos(long);
	long getCurrentPos(void);
	long getTargetPos(void);
	double getSpeed(void);
	void setAmax(long);	
	void setVmax(double);	
	double getVmax(void);
	void adjustSpeed(double percent);
	void setTargetPos(long targetPos);
	bool positionReached(void);
	bool isSlewing(void);
	void setDir(bool);
	void abort(void);
	void resetAbort(void);
	void initMc5160(TMC5160Stepper *driverP);
  void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId); 
};
