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
  virtual bool isMoving(void);
  virtual void abort(void);
  virtual void resetAbort(void);
  virtual void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId); 
  virtual void initMc5160(TMC5160Stepper *driverP, SemaphoreHandle_t mtx, long);
  virtual void setRatios(long);
};

