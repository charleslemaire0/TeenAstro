#pragma once

class MotionControl
{
public:
  virtual void setCurrentPos(long);
  virtual void setTargetPos(long);
  virtual void setVmax(double);
  virtual void setAmax(long);
  virtual long getCurrentPos(void);
  virtual long getTargetPos(void);
  virtual double getSpeed(void);
  virtual bool positionReached(void);
  virtual bool isSlewing(void);
  virtual void adjustSpeed(double);
  virtual void abort(void);
  virtual double getVmax(void);
  virtual void resetAbort(void);
  virtual void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId); 
  virtual void initMc5160(TMC5160Stepper *driverP);
};

