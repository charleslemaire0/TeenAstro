#ifdef __ESP32__
#include <Arduino.h>
#endif

#ifdef __arm__
#include "arduino_freertos.h"
#endif

#include "TMCStepper.h"
#include "MotionControl.h"
#include "Mc5160.h"

// not used, needed to keep the linker happy
void Mc5160::initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId)
{
}

void Mc5160::initMc5160(TMC5160Stepper *driverP)
{
  drvP = driverP;
  setCurrentPos(0);
  setTargetPos(0);
};

void Mc5160::setCurrentPos(long pos)
{
  drvP->XACTUAL(pos);
}

long Mc5160::getCurrentPos()
{
  return drvP->XACTUAL();
}

long Mc5160::getTargetPos()
{
  return drvP->XTARGET();
}

double Mc5160::getSpeed()
{
  return drvP->VACTUAL();
}

double Mc5160::getVmax()
{
  return (drvP->VMAX());
}

void Mc5160::setAmax(long a)
{
  drvP->AMAX(a);
  drvP->DMAX(a);
}

void Mc5160::setVmax(double v)
{
  int vint = (int) v;
  drvP->VMAX(vint);
}

void Mc5160::setTargetPos(long pos)
{
  drvP->XTARGET(pos);
}

bool Mc5160::positionReached(void)
{
  return drvP->position_reached();
}

bool Mc5160::isSlewing(void)
{
  return !(drvP->vzero());
}

// not yet implemented
void Mc5160::adjustSpeed(double percent)
{
}

void Mc5160::abort(void)
{
}

void Mc5160::resetAbort(void)
{
}

