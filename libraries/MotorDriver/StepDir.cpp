
#ifdef __ESP32__
#include <Arduino.h>
#define ISR(f) void IRAM_ATTR f(void) 
#define MIN_INTERRUPT_PERIOD 5UL      // µS
#define MAX_INTERRUPT_PERIOD 10000000UL  // 10S 
// the motor task runs every mS
#define TICK_PERIOD_MS 1
#endif

#ifdef __arm__
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#define ISR(f)  void f(void)
//#include "IntervalTimer2.h"


IntervalTimer  itimer3;
IntervalTimer  itimer4;

// Periods in µS, speeds in steps/S
// *** replace by constants from the HAL
#define MIN_INTERRUPT_PERIOD 1L        // µS
#define MAX_INTERRUPT_PERIOD 100000UL  // 100mS
// the motor task runs every 1mS
#define TICK_PERIOD_MS 1
#endif


#include "TMCStepper.h"
#include "MotionControl.h"
#include "StepDir.h"

#define VSTOP 20 
  
#define SD_debug0(b)


/*
 * Compute deceleration distance in steps from current speed and max. acceleration
 * speed in steps/sec
 * aMax in steps/sec^2
 */
long StepDir::decelDistance(double speed, unsigned long aMax)
{
  return (long)((stopDistance + (speed * speed) /  (2 * aMax)));  
} 


// Compute period in microseconds from speed in (micro)steps per second 
double getPeriod(double V)
{
  if (V == 0)
    return MAX_INTERRUPT_PERIOD;

  double period = 1000000UL / fabs(V);

  if (period <= MIN_INTERRUPT_PERIOD)
  {
    period = MIN_INTERRUPT_PERIOD;
  }
 
  if (period >= MAX_INTERRUPT_PERIOD)
  {
    period = MAX_INTERRUPT_PERIOD;
  }
  return period;
}

#ifdef DBG_STEPDIR
void StepDir::logMotion(uint8_t state, long delta, double speed)
{
  lP->t = xTaskGetTickCount();
  lP->state = state;
  lP->delta = delta;
  lP->speed = speed;
  lP++;
  if (lP == (logTable + LOG_SIZE))
    lP = logTable;
}
#endif


// Common interrupt handler code for both motors 
void StepDirInterruptHandler(StepDir *mcP)
{
  if (mcP->currentSpeed == 0)
    return;

  // update internal step counter
  if (mcP->currentSpeed < 0.0)
    mcP->currentPos--;
  else
    mcP->currentPos++;
  // step the motor
  digitalWrite(mcP->stepPin, mcP->edgePos);
  mcP->edgePos = !mcP->edgePos;
}



/*
 * Program speed and direction
 */
void StepDir::programSpeed(double V)
{
  if (V !=0)
    setDir(V<0.0);

  double period = getPeriod(fabs(V));
  #ifdef __arm__

  ((IntervalTimer *) timerP)->update(period); 
  #endif

  #ifdef __ESP32__  
  timerAlarmWrite(timerP, period, true);   // period in µS 
  timerAlarmEnable(timerP);
  #endif  
  currentSpeed = V;
}

void StepDir::setDir(bool rev)
{
  digitalWrite(dirPin, rev);
}

void StepDir::setEvents(unsigned eventBits)
{
  xEventGroupSetBits(motEvents, eventBits);   
}

void StepDir::resetEvents(unsigned eventBits)
{
  xEventGroupClearBits(motEvents, eventBits);
}

unsigned StepDir::getEvents(void)
{
  return (xEventGroupGetBits(motEvents)); 
}

long StepDir::getDelta(void)
{
  long delta;
  cli();
  delta = targetPos - currentPos;
  sei();
  return delta;
}

// allows logging state changes
void StepDir::state(PositionState newState)
{
  if (newState != posState)
  {
    SD_debug0(newState);
    posState = newState;
  }
}

PositionState StepDir::state()
{
  return posState;
}

/*
 * Positioning mode
 * Handle movement from start to target positions
 */
void StepDir::positionMode(void)
{
  int sign;
  newSpeed = fabs(currentSpeed);
  
  if (!enabled)
    return;
  
  // check target distance
  // perform calculations on absolute speed, then apply sign 
  delta = getDelta();

  if (delta >= 0)
  {
    sign = 1;
  }
  else
  {
    delta = -delta;
    sign = -1;
  }

  d1 = decelDistance(newSpeed, aMax);

  // Check for abort 
  if (getEvents() & EV_MOT_ABORT)
  {
    resetEvents(EV_MOT_ABORT);
    if (newSpeed != 0)
    {
      cli();
      targetPos = currentPos + (currentSpeed >= 0 ? 1:-1) * d1;
      sei();
      if (newSpeed < vStop)
      {
        state(PS_STOPPING);
      }
      else
      {
        state(PS_DECEL_TARGET);
      }      
    }
  }

  // check distances
  if (delta != 0)
  {
    // no point in computing acceleration if we are very close
    if (delta < stopDistance)
    {
      programSpeed(sign * fmin (vMax, vStop));
      state(PS_STOPPING);
    }
    else
    {
      if (delta < d1)
      {
        state(PS_DECEL_TARGET);
      }
    }
  }
 
  // perform actions according to current state
  switch (state())
  {
    case PS_IDLE:
      resetEvents(EV_MOT_ABORT);
      break;

    case PS_ACCEL: 
      {
        // Accelerate to VMax
        newSpeed = fmin(vMax, newSpeed + ((double) (aMax * TICK_PERIOD_MS)) / 1000);
        programSpeed(sign * newSpeed); 
        if (newSpeed == vMax)
          state(PS_CRUISE);               
      }
      break;

    case PS_CRUISE: 
      // check if vMax has changed
      if (newSpeed > vMax) // check if the speed has changed (ie guiding command)
      {
        state(PS_ACCEL);
      }
      if (newSpeed < vMax) 
      {
        state(PS_DECEL);
      }
      if (newSpeed == 0.0)
      {
        programSpeed(0.0);
        resetEvents(EV_MOT_GOTO);
        state(PS_IDLE);
      }
      break;

    case PS_DECEL:
      {
        // Decelerate at default deceleration aMax
        newSpeed = fmax(vMax, newSpeed - ((double)(aMax * TICK_PERIOD_MS)) / 1000);
        programSpeed(sign * newSpeed);                
        if (newSpeed == vMax)
          state(PS_CRUISE);
      }
      break;

    case PS_DECEL_TARGET:
      {
        // Decelerate at computed deceleration dMax
        dMax = newSpeed * newSpeed / (2 * delta);
        newSpeed = newSpeed - (dMax * TICK_PERIOD_MS) / 1000;
        programSpeed(sign * newSpeed);                
      }
      break;

    case PS_STOPPING:
      if (delta == 0)
      {
        programSpeed(0.0);
        resetEvents(EV_MOT_GOTO);
        state(PS_IDLE);
      }
      break;
  }
}


/*
 * Motor Task
 * Normally runs every 10mS 
 */
void motorTask(void *arg)
{
  StepDir *mcP = (StepDir*) arg;
  TickType_t xLastWakeTime;
  const TickType_t xPeriod =  pdMS_TO_TICKS(TICK_PERIOD_MS);  // in mS

  while (1)
  { 
    xLastWakeTime = xTaskGetTickCount();
  
    // update image of position
    cli();
    mcP->currentPosImage = mcP->currentPos;
    sei(); 

    mcP->positionMode();
  
    unsigned msgBuffer[SD_MAX_MESSAGE_SIZE];

    // Wait for next message
    BaseType_t res = xQueueReceive( mcP->motQueue, &msgBuffer, 0);

    if (res == pdPASS)                       // received a message?
    {
      switch(msgBuffer[0])
      {
        case MSG_SET_CUR_POS:
          cli();
          mcP->currentPosImage = mcP->currentPos = msgBuffer[1];
          sei();            
          if (mcP->currentPos != mcP->targetPos)
          {
            mcP->state(PS_ACCEL);
          }
          else
          {
            mcP->resetEvents(EV_MOT_GOTO);  // no movement requested
          }
          break;

        case MSG_SET_TARGET_POS:
          cli();
          mcP->targetPos = msgBuffer[1];
          sei(); 
          if (mcP->currentPos != mcP->targetPos)
          {
            mcP->state(PS_ACCEL);
          }
          else
          {
            mcP->resetEvents(EV_MOT_GOTO);  // no movement requested
          }
          break;

        case MSG_SYNC_POS:
          cli();
          mcP->currentPosImage = mcP->currentPos = mcP->targetPos = msgBuffer[1];
          sei(); 
          break;

        case MSG_SET_VMAX:
          if ((mcP->state() != PS_CRUISE) && (mcP->state() != PS_IDLE))  // ignore speed change when accelerating etc. 
            break;
          double v;
          memcpy(&v, &msgBuffer[1], sizeof(double));  // always positive 
          mcP->vMax = v;
          mcP->vStop = fmin(VSTOP, mcP->vMax); 
          break;

        case MSG_SET_AMAX:
          mcP->aMax = msgBuffer[1];
          break;

        default:
          break;
        }     
    }
    vTaskDelayUntil( &xLastWakeTime, xPeriod );
  }
}

// not used, needed to keep the linker happy
void StepDir::initMc5160(TMC5160Stepper *driverP, SemaphoreHandle_t, long)
{
}

void StepDir::initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId)
{
  char taskName[20];
  snprintf(taskName, 20, "Motor%02d", timerId-1); // different name for each task

  // initialize hardware pins
  dirPin = DirPin;
  stepPin = StepPin;

  // address of hardware timer register
  // Start timers and attach interrupt handlers
  #ifdef __arm__

  if (timerId == 2)
    timerP = &itimer3;
  else
    timerP = &itimer4;

  ((IntervalTimer *) timerP)->begin(isrP, 1000);  // start at 1mS
  ((IntervalTimer *) timerP)->priority(1);
  #endif

  #ifdef __ESP32__  
  timerP = timerBegin(timerId, 80, true);            // we run at 80MHz - period is 1 µS
  timerAttachInterrupt(timerP, (void (*)()) isrP, true);  
  timerSetAutoReload(timerP, true);
  timerStart(timerP);
  #endif  

  // Set initial values
  currentPos = targetPos = 0;
  currentSpeed = 0.0;

  // default parameters
  aMax = 2000.0;
  vStop = VSTOP;                       // last steps to reach target
  stopDistance = (long) vStop / 10;    // steps in 100mS
  vMax = 10000.0;

  // queue for receiving messages from other tasks
  motQueue = xQueueCreate( SD_QUEUE_SIZE, SD_MAX_MESSAGE_SIZE * sizeof(unsigned));

  // Event group for notifying other tasks 
  motEvents = xEventGroupCreate();
  resetEvents(EV_MOT_GOTO);

  // Start the motor task
  xTaskCreate(
      motorTask,          // Function that should be called
      taskName,           // Name of the task (for debugging)
      2000,               // Stack size (bytes)
      this,               // Parameter to pass 
      timerId+6,                  // priority
      &motTaskHandle      // Task handle
    );  

  initialized = true;
}



void StepDir::setCurrentPos(long pos)
{
  unsigned msg[SD_MAX_MESSAGE_SIZE];

  msg[0] = MSG_SET_CUR_POS;
  msg[1] = pos;
  xQueueSend(motQueue, &msg, 0);
  setEvents(EV_MOT_GOTO);
}

long StepDir::getCurrentPos()
{
  if (!initialized)
    return 0;
  return currentPosImage;
}

long StepDir::getTargetPos()
{
  long pos;
  if (!initialized)
    return 0;
  cli();
  pos = targetPos;
  sei();
  return pos;
}

double StepDir::getSpeed()
{
  double speed;
  if (!initialized)
    return 0;
  cli();
  speed = currentSpeed;
  sei();
  return speed;
}

void StepDir::setAmax(long amax)
{
  unsigned msg[SD_MAX_MESSAGE_SIZE];

  msg[0] = MSG_SET_AMAX; 
  msg[1] = amax;
  xQueueSend( motQueue, &msg, 0);
}

void StepDir::setVmax(double vmax)
{
  unsigned msg[SD_MAX_MESSAGE_SIZE];

  msg[0] = MSG_SET_VMAX;
  memcpy(&msg[1], &vmax, sizeof(double));
  xQueueSend( motQueue, &msg, 0);
}

void StepDir::setTargetPos(long targetPos)
{
  unsigned msg[SD_MAX_MESSAGE_SIZE];

  msg[0] = MSG_SET_TARGET_POS; 
  msg[1] = targetPos;
  xQueueSend( motQueue, &msg, 0);
  setEvents(EV_MOT_GOTO);
}

void StepDir::syncPos(long pos)
{
  unsigned msg[SD_MAX_MESSAGE_SIZE];

  msg[0] = MSG_SYNC_POS; 
  msg[1] = pos;
  xQueueSend( motQueue, &msg, 0);
}

void StepDir::abort(void)
{
  setEvents(EV_MOT_ABORT);
}

// Reset abort flag
void StepDir::resetAbort(void)
{
  resetEvents(EV_MOT_ABORT);
}

bool StepDir::positionReached(void)
{
  return ((currentPos == targetPos) && !isMoving());
}

bool StepDir::isMoving(void)
{
  return((getEvents() & EV_MOT_GOTO) !=0);
}

void StepDir::enable(void)
{
  enabled = true;
}
void StepDir::disable(void)
{
  enabled = false;
}
// never called
void StepDir::setRatios(long fkHz)
{
}


