#include "Global.h"


Speeds slewingSpeeds;


void setEvents(unsigned ev)
{
  xEventGroupSetBits(mountEvents, ev);   
}

void resetEvents(unsigned ev)
{
  xEventGroupClearBits(mountEvents, ev);   
}

// Returns true if all events in a group are set
bool getEvent(unsigned ev)
{
  if ((xEventGroupGetBits(mountEvents) & ev) == ev)
    return true;
  else 
    return false;      
}

// block until slewing bit is set
void waitSlewing(void)
{
  xEventGroupWaitBits(mountEvents, EV_SLEWING, pdFALSE, pdFALSE, 100);
}


bool isSlewing(void)
{
  return getEvent(EV_SLEWING);
}

bool isTracking(void)
{
  return getEvent(EV_TRACKING);
}

void stopMoving(void)
{
  setEvents(EV_ABORT);
}

void resetAbort(void)
{
  resetEvents(EV_ABORT);
  motorA1.resetAbort();
  motorA2.resetAbort();
}

// if stepper drive can switch decay mode, set it here
void DecayModeTracking()
{
//  motorA1.setCurrent((unsigned int)motorA1.lowCurr); 
//   motorA2.setCurrent((unsigned int)motorA2.lowCurr);
}

void DecayModeGoto()
{
//  motorA1.setCurrent((unsigned int)motorA1.highCurr); 
//  motorA2.setCurrent((unsigned int)motorA2.highCurr);
}

// Weird algorithm to prevent GEM mounts from pointing below horizon while slewing 
void adjustSpeeds(void)
{
  bool dangerZone=false;
  int decDir = mount.mP->decDirection();
  static double adj = 1.0;
  double k = 1.01;

  if (decDir == 0)
    return;

  xSemaphoreTake(swMutex, portMAX_DELAY); 
  deltaAlt = currentAzAlt.alt - prevAzAlt.alt;
  xSemaphoreGive(swMutex);

  if ((currentAzAlt.alt - limits.minAlt) < 8)  // arbitrary value that works
  {
    dangerZone = true;
    adj = fmin(adj * k, 3.0);                 // speed adjustment coefficient

    if (decDir < 0)  // decreasing declination?
    {
      motorA1.setVmax(slewingSpeeds.speed1 * fmin(adj, 1.0));
      motorA2.setVmax(slewingSpeeds.speed2 / fmax(adj, 0.5));
    }
    else
    {
      motorA1.setVmax(slewingSpeeds.speed1 / fmax(adj, 1.0));
      motorA2.setVmax(slewingSpeeds.speed2 * fmin(adj, 0.5));
    }
  }
  else
  {
    if (dangerZone)
    {
      // well above altitude limit: reset speed to max
      dangerZone = false;
      adj = 1.0;
      motorA1.setVmax(slewingSpeeds.speed1);
      motorA2.setVmax(slewingSpeeds.speed2);
    }
  }  
}




void controlTask(UNUSED(void *arg))
{
  TickType_t xLastWakeTime;
  const TickType_t xPeriod =  pdMS_TO_TICKS(CTRL_TASK_PERIOD);  // in mS
  CTL_MODE currentMode = CTL_MODE_IDLE;
  long axis1Target, axis2Target;

  while (1)
  { 
    xLastWakeTime = xTaskGetTickCount();  
    // Perform action according to current mode
    switch (currentMode)
    {
      case CTL_MODE_IDLE:
        if (getEvent(EV_START_TRACKING))
        {
          Speeds speeds;
          currentMode = CTL_MODE_TRACKING;
          resetEvents(EV_AT_HOME);
          mount.mP->getTrackingSpeeds(&speeds);
          motorA1.setVmax(fabs(speeds.speed1));
          motorA1.setTargetPos(speeds.speed1>0 ? geoA1.stepsPerRot : -geoA1.stepsPerRot);

          motorA2.setVmax(fabs(speeds.speed2));
          motorA2.setTargetPos(speeds.speed2>0 ? geoA2.stepsPerRot : -geoA2.stepsPerRot);
          resetEvents(EV_START_TRACKING);
          setEvents(EV_TRACKING);
        }
        break;

      case CTL_MODE_TRACKING:
        {
          Speeds speeds;

          // if we are centering, don't do anything
          if (getEvent(EV_CENTERING))
            break;
          // Only need to update speed in case of guiding/spiral, or for an AltAz mount
          if (getEvent(EV_SPEED_CHANGE) | isAltAz())
          {
            mount.mP->getTrackingSpeeds(&speeds);
            resetEvents(EV_SPEED_CHANGE);
            motorA1.setVmax(fabs(speeds.speed1));
            if (speeds.speed1 > 0)
              motorA1.setTargetPos(geoA1.stepsPerRot);
            if (speeds.speed1 < 0)
              motorA1.setTargetPos(-geoA1.stepsPerRot);            

            motorA2.setVmax(fabs(speeds.speed2));
            if (speeds.speed2 > 0)
              motorA2.setTargetPos(geoA2.stepsPerRot);
            if (speeds.speed2 < 0)
              motorA2.setTargetPos(-geoA2.stepsPerRot);
          }
        }
        break;

      case CTL_MODE_GOTO:
        // check if the mount has reached target
        if (motorA1.positionReached() &&  motorA2.positionReached())
        {
          mount.mP->updateRaDec();
          currentMode = CTL_MODE_IDLE;
          resetEvents(EV_SLEWING | EV_TRACKING | EV_GUIDING_AXIS1 | EV_GUIDING_AXIS2 | EV_CENTERING);
          resetEvents(EV_SOUTH | EV_NORTH | EV_WEST | EV_EAST);
          if (getEvent(EV_GOING_HOME))
          {
            resetEvents(EV_GOING_HOME);
            setEvents(EV_AT_HOME);
          }
          if (parkStatus() == PRK_PARKING)
          {
            parkStatus(PRK_PARKED);
          }
        }         

        if (!isAltAz())
        {
          adjustSpeeds(); 
        }
        break;

      case CTL_MODE_STOPPING:
        resetAbort();
        // wait until motors stopped, reset mode to idle
        if (! (motorA1.isMoving() ||  motorA2.isMoving()))
        {
          mount.mP->updateRaDec();
          resetEvents(EV_SLEWING | EV_TRACKING | EV_GUIDING_AXIS1 | EV_GUIDING_AXIS2 | EV_CENTERING);
          resetEvents(EV_SOUTH | EV_NORTH | EV_WEST | EV_EAST);
          currentMode = CTL_MODE_IDLE;
        }
        break;

      default:
        break;
    }
  
   // check for abort
    if (getEvent(EV_ABORT))
    {
      motorA1.abort();
      motorA2.abort();
      currentMode = CTL_MODE_STOPPING;
    }
  
  unsigned msgBuffer[CTL_MAX_MESSAGE_SIZE];
  double speed;
  long target;

    // Wait for next message
  BaseType_t res = xQueueReceive( controlQueue, &msgBuffer, 0);

    if (res == pdPASS)                       // received a message?
    {
      switch(msgBuffer[0])
      {
        case CTL_MSG_GOTO:
          if (lastError() != ERRT_NONE)
            break;
          currentMode = CTL_MODE_GOTO;
          axis1Target = msgBuffer[1];
          axis2Target = msgBuffer[2];
          motorA1.setVmax(slewingSpeeds.speed1);
          motorA1.setTargetPos(axis1Target);
          motorA2.setVmax(slewingSpeeds.speed2);
          motorA2.setTargetPos(axis2Target);
          setEvents(EV_SLEWING);
          resetEvents(EV_AT_HOME | EV_TRACKING);
          break;

        case CTL_MSG_GOTO_HOME:
          if (lastError() != ERRT_NONE)
            break;
          currentMode = CTL_MODE_GOTO;
          motorA1.setVmax(slewingSpeeds.speed1);
          motorA1.setTargetPos(geoA1.homeDef);
          motorA2.setVmax(slewingSpeeds.speed2);
          motorA2.setTargetPos(geoA2.homeDef);
          setEvents(EV_GOING_HOME);
          setEvents(EV_SLEWING);
          resetEvents(EV_AT_HOME | EV_TRACKING);
          break;

        case CTL_MSG_START_TRACKING:
          // set the event to start tracking as soon as we are in idle mode
          setEvents(EV_START_TRACKING);
          break;

        case CTL_MSG_STOP_TRACKING:
          stopMoving();
          break;

        case CTL_MSG_MOVE_AXIS1:
          target = msgBuffer[1];
          memcpy(&speed, &msgBuffer[2], sizeof(double));
          resetEvents(EV_AT_HOME);
          motorA1.setVmax(fabs(speed)*geoA1.stepsPerSecond/SIDEREAL_SECOND);
          motorA1.setTargetPos(target);  
          if (target == geoA1.westDef)
            setEvents(EV_CENTERING | EV_WEST);
          else
            setEvents(EV_CENTERING | EV_EAST);
          resetEvents(EV_AT_HOME);
          break;

        case CTL_MSG_MOVE_AXIS2:
          target = msgBuffer[1];
          memcpy(&speed, &msgBuffer[2], sizeof(double));
          resetEvents(EV_AT_HOME);
          motorA2.setVmax(fabs(speed)*geoA2.stepsPerSecond/SIDEREAL_SECOND);
          motorA2.setTargetPos(target);  
          if (target == 0)                        // fix this for S hemisphere
            setEvents(EV_CENTERING | EV_NORTH);   
          else
            setEvents(EV_CENTERING | EV_SOUTH);   
          resetEvents(EV_AT_HOME);
          break;

        case CTL_MSG_SET_SLEW_SPEED:
          double speed;
          memcpy(&speed, &msgBuffer[1], sizeof(double));
          slewingSpeeds.speed1 = fabs(speed)*geoA1.stepsPerSecond/SIDEREAL_SECOND;
          slewingSpeeds.speed2 = fabs(speed)*geoA2.stepsPerSecond/SIDEREAL_SECOND;
          break;

        case CTL_MSG_STOP_AXIS1:
          motorA1.abort();
          resetEvents(EV_CENTERING | EV_WEST | EV_EAST);
          break;

        case CTL_MSG_STOP_AXIS2:
          motorA2.abort();
          resetEvents(EV_CENTERING | EV_NORTH | EV_SOUTH);
          break;

        default:
          break;
        }     
    }
    vTaskDelayUntil( &xLastWakeTime, xPeriod );
  }
}

void startTracking(void)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_START_TRACKING; 
  xQueueSend( controlQueue, &msg, 0);
}

void stopTracking(void)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_STOP_TRACKING; 
  xQueueSend( controlQueue, &msg, 0);
}

void MoveAxis1(const byte dir)
{
  MoveAxis1AtRate(guideRates[activeGuideRate], dir);
}

void MoveAxis1AtRate(double speed, const byte dir)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];

  if ((parkStatus() != PRK_UNPARKED) || isSlewing())
    return;

  msg[0] = CTL_MSG_MOVE_AXIS1; 
  msg[1] = (dir == 'w' ? geoA1.westDef : geoA1.eastDef);
  memcpy(&msg[2], &speed, sizeof(double));
  xQueueSend(controlQueue, &msg, 0);
}

void StopAxis1()
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
 
  msg[0] = CTL_MSG_STOP_AXIS1; 
  xQueueSend(controlQueue, &msg, 0);
}

void MoveAxis2(const byte dir)
{
  MoveAxis2AtRate(guideRates[activeGuideRate], dir);
}

void MoveAxis2AtRate(double speed, const byte dir)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  long target;

  if ((parkStatus() != PRK_UNPARKED) || isSlewing())
    return;

  target = mount.mP->axis2Target(dir);

  msg[0] = CTL_MSG_MOVE_AXIS2; 
  msg[1] = target;
  memcpy(&msg[2], &speed, sizeof(double));
  xQueueSend(controlQueue, &msg, 0);
}

void StopAxis2()
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
 
  msg[0] = CTL_MSG_STOP_AXIS2; 
  xQueueSend(controlQueue, &msg, 0);
}

byte goTo(Steps *sP)
{
  if (getEvent(EV_ABORT))
    return ERRGOTO____;
  unsigned msg[CTL_MAX_MESSAGE_SIZE];

  // set max slew speed without updating current guide rate
  msg[0] = CTL_MSG_SET_SLEW_SPEED;
  memcpy (&msg[1], &guideRates[RXX], sizeof(double)); 
  xQueueSend(controlQueue, &msg, 0);

  // initiate the goto
  msg[0] = CTL_MSG_GOTO; 
  msg[1] = sP->steps1;
  msg[2] = sP->steps2;
  xQueueSend(controlQueue, &msg, 0);

  waitSlewing();

  DecayModeGoto();
  return ERRGOTO_NONE;
}

