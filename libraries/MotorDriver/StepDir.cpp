#ifdef __ESP32__
#include <Arduino.h>
#define ISR(f) void IRAM_ATTR f(void) 
#endif
#include "TMCStepper.h"
#include "MotionControl.h"

#ifdef __arm__
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include "queue.h"
#include "event_groups.h"
#define ISR(f)  void f(void)
IntervalTimer  itimer3;
IntervalTimer  itimer4;
#endif


#include "StepDir.h"

// Periods in µS, speeds in steps/S
// *** replace by constants from the main program
#define MIN_PERIOD 20UL     // 20 µS
#define MAX_PERIOD 200000UL  // 200 mS 
// the motor task runs every 10mS
#define TICK_PERIOD_MS 10

/*
 * Compute deceleration distance in steps from current speed and max. acceleration
 * speed in steps/sec
 * aMax in steps/sec^2
 */
long decelDistance(double speed, unsigned long aMax)
{
  return (long)((speed * speed) /  (2 * aMax));  
} 




// Compute period in microseconds from speed in (micro)steps per second 
unsigned long getPeriod(double V)
{
  if (V == 0)
    return MAX_PERIOD;

  unsigned long period = 1000000UL / fabs(V);

  if (period <= MIN_PERIOD)
  {
    period = MIN_PERIOD;
  }
 
  if (period >= MAX_PERIOD)
  {
    period = MAX_PERIOD;
  }
  return period;
}

#ifdef LOG_MOTION
void StepDir::logMotion(long pos, double speed)
{
  lP->t = xTaskGetTickCount();
  lP->pos = pos;
  lP->speed = (long) speed;
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

  // step the motor
  digitalWrite(mcP->stepPin, 1);

  // update internal step counter
  if (mcP->currentSpeed < 0.0)
    mcP->currentPos--;
  else
    mcP->currentPos++;
  digitalWrite(mcP->stepPin, 0);
}



/*
 * Program speed and direction
 */
void StepDir::programSpeed(double V)
{
  setDir(V<0.0);

  long period = getPeriod(fabs(V));
  #ifdef __arm__
  timerP->update(period); 
  #endif

  #ifdef __ESP32__  
  timerAlarmWrite(timerP, period, true);   // period in µS 
  timerAlarmEnable(timerP);
  #endif  
  currentSpeed = V;
}

void StepDir::setDir(bool rev)
{
  digitalWrite(dirPin, !rev);
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

/*
 * Positioning mode
 * Handle movement from start to target positions
 */
void StepDir::positionMode(void)
{
  int sign;
  double newSpeed = fabs(currentSpeed);
  long delta = getDelta();

  if (delta >= 0)
  {
    sign = 1;
  }
  else
  {
    delta = -delta;
    sign = -1;
  }
#ifdef DBG_STEPDIR
    logMotion (currentSpeed, currentPos);
#endif
  // Check for abort 
  if (xEventGroupGetBits(motEvents) & EV_MOT_ABORT)
  {
    if (posState != PS_IDLE)
    {
      cli();
      targetPos = currentPos + sign * (min(delta, decelDistance(newSpeed, aMax)));
      sei();
      posState = PS_DECEL;
    }
  }

  // perform calculations on absolute speed, then apply sign 
  switch (posState)
  {
    case PS_IDLE:
      resetEvents(EV_MOT_ABORT);
      break;
    case PS_ACCEL: 
      speedAdjustment = 1.0;    // reset speed adjustment when starting the Goto
      // Accelerate to VMax
      newSpeed = newSpeed + (aMax * TICK_PERIOD_MS) / 1000;
      if (delta < decelDistance(newSpeed, aMax))
      {
        posState = PS_DECEL;
      }
      else 
      {
        if (newSpeed >= vMax)
        {
          newSpeed = vMax;
          posState = PS_CRUISE;
        }
      }
      programSpeed(sign * newSpeed);        
      break;

    case PS_CRUISE: 
      // Maintain VMax speed until close to target
      newSpeed = vMax;
      if (speedAdjustment != 1.0)
      {
        newSpeed *= speedAdjustment;
      }
      programSpeed(sign * newSpeed);

      if (delta < decelDistance(newSpeed, aMax))
      {
        posState = PS_DECEL;
      }
      break;

    case PS_DECEL:
      // Decelerate then stop when at target
      if (delta > stopDistance)
      {
        if (newSpeed > (aMax * TICK_PERIOD_MS) / 1000) 
        {
          newSpeed = newSpeed - (aMax * TICK_PERIOD_MS) / 1000;
        }
        else
        {
          newSpeed = vSlow;
        }
        programSpeed(sign * newSpeed);
      }
      else if (delta != 0)
      {
        newSpeed = vStop;
        programSpeed(sign * newSpeed);
      }
      else    // delta == 0
      {
        programSpeed(0.0);
        resetEvents(EV_MOT_GOTO);
        posState = PS_IDLE;
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
  double k;
  double percent;

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
          mcP->currentPos = msgBuffer[1];
          if (mcP->currentPos != mcP->targetPos)
          {
            mcP->posState = PS_ACCEL;
          }
          else
          {
            mcP->resetEvents(EV_MOT_GOTO);  // no movement requested
          }
          sei();            
          break;

        case MSG_SET_TARGET_POS:
          cli();
          mcP->targetPos = msgBuffer[1];
          if (mcP->currentPos != mcP->targetPos)
          {
            mcP->posState = PS_ACCEL;
          }
          else
          {
            mcP->resetEvents(EV_MOT_GOTO);  // no movement requested
          }
          sei();            
          break;

        case MSG_SET_VMAX:
          memcpy(&mcP->vMax, &msgBuffer[1], sizeof(double));  // always positive 
          break;

        case MSG_SET_AMAX:
          mcP->aMax = msgBuffer[1];
          break;

        case MSG_ADJUST_SPEED:          // used during Goto (positioning mode, cruise state)
          memcpy(&percent, &msgBuffer[1], sizeof(double));
          if (percent == 0.0)
          {
            mcP->speedAdjustment = 1.0;   // reset adjustment
          }
          else
          {
            k = (100.0 + percent) / 100.0;
            if (k > 1.0)
              k = 1.0;    // cannot go faster than max speed
            if (mcP->speedAdjustment > 0.1) // do not slow down too much
              mcP->speedAdjustment = mcP->speedAdjustment * k;    
          }
          break;
        default:
          break;
        }     
    }
    vTaskDelayUntil( &xLastWakeTime, xPeriod );
  }
}

// not used, needed to keep the linker happy
void StepDir::initMc5160(TMC5160Stepper *driverP)
{
}

void StepDir::initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId)
{
  char taskName[20];
  snprintf(taskName, 20, "Motor%02d", timerId); // different name for each task

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

  timerP->begin(isrP, 1000);  // start at 1mS
  timerP->priority(1);
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
  aMax = 200.0;
  vSlow = 500.0;                      // while slowing down
  vStop = 50.0;                     // last steps to reach target
  vMax = 10000.0;
  stopDistance = (20 * vStop) * ((double) TICK_PERIOD_MS / 1000);   // set stop distance to 20 times the steps in one tick

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
      1,                  // priority
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

double StepDir::getVmax()
{
  if (!initialized)
    return 0;
  return vMax;
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

void StepDir::adjustSpeed(double percent)
{
  unsigned msg[SD_MAX_MESSAGE_SIZE];

  msg[0] = MSG_ADJUST_SPEED; 
  memcpy(&msg[1], &percent, sizeof(double));
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
  return ((currentPos == targetPos) && !isSlewing());
}

bool StepDir::isSlewing(void)
{
  return((getEvents() & EV_MOT_GOTO) !=0);
}



