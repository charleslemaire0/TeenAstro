#ifdef __ESP32__
#include <Arduino.h>
#endif

#ifdef __arm__
#include "arduino_freertos.h"
#include "semphr.h"
#endif

#include "TMCStepper.h"
#include "MotionControl.h"
#include "Mc5160.h"


// From 5160 spec, section 12.1, Real-World Unit Conversions
// These are computed for the 12MHz internal clock - change for 16MHz external clock!
// v[Hz] = v[5160] * ( fCLK[Hz]/2 / 2^23 )   ~=   0.715 * v[5160]
#define SPEED_RATIO (0.715)		// Ratio of computed clock to VMAX register value 

// a[Hz/s] = a[5160] * fCLK[Hz]^2 / (512*256) / 2^24 ~= 65 * a[5160]
#define ACCEL_RATIO (65.0)    // Ratio of computed acceleration (Hz/S) to AMAX register value 
#define MIN_SPEED   (1.0)     // min. speed to program in steps per second (below this, program zero)

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
  drvP->VSTART(0);
  drvP->VSTOP(10);  // must be higher than VSTART
  drvP->d1(50);     // must not be set to zero
  drvP->v1(0);      // disable A1 and D1 phases
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
  double v = ((double) drvP->VACTUAL()) * SPEED_RATIO;
  xSemaphoreGive(mutex);
  return v;
}

void Mc5160::setAmax(long a)
{
  long regValue;
  regValue = (int) ((double) a / ACCEL_RATIO);
  xSemaphoreTake(mutex, portMAX_DELAY);
  drvP->AMAX(regValue);
  drvP->DMAX(regValue);
  xSemaphoreGive(mutex);
}

// v is always positive
void Mc5160::setVmax(double v)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  int vint = (int) (v / SPEED_RATIO); 
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

void Mc5160::abort(void)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
	drvP->VMAX(0);
  xSemaphoreGive(mutex);
}

void Mc5160::resetAbort(void)
{
}

