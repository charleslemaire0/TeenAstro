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

void Mc5160::initMc5160(TMC5160Stepper *driverP, SemaphoreHandle_t mtx)
{
  drvP = driverP;
  mutex = mtx;
  setCurrentPos(0);
  setTargetPos(0);
  // default ramp parameters
  drvP->VMAX(10000);
  drvP->AMAX(200);
  drvP->DMAX(200);
  drvP->VSTART(50);
  drvP->VSTOP(50);
  drvP->d1(50);
};

void Mc5160::setCurrentPos(long pos)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  drvP->XACTUAL(pos);
  xSemaphoreGive(mutex);
}

long Mc5160::getCurrentPos()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  long p = drvP->XACTUAL();
  xSemaphoreGive(mutex);
  return p;
}

long Mc5160::getTargetPos()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  long p = drvP->XTARGET();
  xSemaphoreGive(mutex);
  return p;
}

double Mc5160::getSpeed()
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  double v = (double) drvP->VACTUAL();
  xSemaphoreGive(mutex);
  return v;
}

void Mc5160::setAmax(long a)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  drvP->AMAX(a);
  drvP->DMAX(a);
  xSemaphoreGive(mutex);
}

void Mc5160::setVmax(double v)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  int vint = (int) v;
  drvP->VMAX(vint);
  xSemaphoreGive(mutex);
}

void Mc5160::setTargetPos(long pos)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  drvP->XTARGET(pos);
  xSemaphoreGive(mutex);
}

bool Mc5160::positionReached(void)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  bool b = drvP->position_reached();
  xSemaphoreGive(mutex);
  return b;
}

bool Mc5160::isMoving(void)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  bool b = !(drvP->vzero());
  xSemaphoreGive(mutex);
  return b;
}

// not yet implemented
void Mc5160::adjustSpeed(double percent)
{
}

void Mc5160::abort(void)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
	drvP->VMAX(0);
  xSemaphoreGive(mutex);
}

void Mc5160::resetAbort(void)
{
}

