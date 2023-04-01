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
  if (DecayModeTrack) return;
  DecayModeTrack = true;
  motorA1.setCurrent((unsigned int)motorA1.lowCurr); 
  motorA2.setCurrent((unsigned int)motorA2.lowCurr);
}

void DecayModeGoto()
{
  if (!DecayModeTrack) return;
  DecayModeTrack = false;
  motorA1.setCurrent((unsigned int)motorA1.highCurr); 
  motorA2.setCurrent((unsigned int)motorA2.highCurr);
}




void controlTask(UNUSED(void *arg))
{
  TickType_t xLastWakeTime;
  const TickType_t xPeriod =  pdMS_TO_TICKS(CTRL_TASK_PERIOD);  // in mS
  CTL_MODE currentMode = CTL_MODE_IDLE;
  CTL_MODE previousMode;
  long axis1Target, axis2Target;
  bool dangerZone=false;
  long count = 0;

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
        // check tracking speed every 100mS
        if (count % 10 == 0)
        {
          Speeds speeds;
          mount.mP->getTrackingSpeeds(&speeds);
          if (speeds.speed1 != 0)
          {
            motorA1.setVmax(fabs(speeds.speed1));
            if (speeds.speed1 > 0)
              motorA1.setTargetPos(geoA1.stepsPerRot);
            else
              motorA1.setTargetPos(-geoA1.stepsPerRot);            
          }
          if (speeds.speed2 != 0)
          {
            motorA2.setVmax(fabs(speeds.speed2));
            if (speeds.speed2 > 0)
              motorA2.setTargetPos(geoA2.stepsPerRot);
            else
              motorA2.setTargetPos(-geoA2.stepsPerRot);
          }
        }
        count++;
        break;

      case CTL_MODE_GOTO:
        // check if the mount has reached target
        if (motorA1.positionReached() &&  motorA2.positionReached())
        {
          mount.mP->updateRaDec();
          currentMode = CTL_MODE_IDLE;
          resetEvents(EV_SLEWING);
          if (getEvent(EV_GOING_HOME))
          {
            resetEvents(EV_GOING_HOME);
            setEvents(EV_AT_HOME);
          }
        }          
        // Weird algorithm to prevent GEM mounts from pointing below horizon while slewing 
        if (!isAltAz())
        {
          int decDir = mount.mP->decDirection();

          if (decDir == 0)
            break;

          xSemaphoreTake(swMutex, portMAX_DELAY); 
          deltaAlt = currentAzAlt.alt - prevAzAlt.alt;
          xSemaphoreGive(swMutex);

          if ((currentAzAlt.alt - limits.minAlt) < 8)  // arbitrary value that works
          {
            dangerZone = true;
            if (decDir < 0)  // decreasing declination?
            {
              motorA1.adjustSpeed(1);
              motorA2.adjustSpeed(-1);  
            }
            else
            {
              motorA1.adjustSpeed(-1);
              motorA2.adjustSpeed(1);  
            }
          }
          else
          {
            if (dangerZone)
            {
              // well above altitude limit: reset speed to max
              dangerZone = false;
              motorA1.adjustSpeed(0);
              motorA2.adjustSpeed(0);  
            }
          }
        }
        break;

      case CTL_MODE_STOPPING:
        // wait until motors stopped, reset mode to idle
        if (! (motorA1.isMoving() ||  motorA2.isMoving()))
        {
          mount.mP->updateRaDec();
          resetAbort();
          resetEvents(EV_SLEWING);
          resetEvents(EV_TRACKING);
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
          if (getEvent(EV_ERROR))
            break;
          currentMode = CTL_MODE_GOTO;
          axis1Target = msgBuffer[1];
          axis2Target = msgBuffer[2];
          motorA1.setVmax(slewingSpeeds.speed1);
          motorA1.setTargetPos(axis1Target);
          motorA2.setVmax(slewingSpeeds.speed2);
          motorA2.setTargetPos(axis2Target);
          setEvents(EV_SLEWING);
          resetEvents(EV_AT_HOME);
          break;

        case CTL_MSG_GOTO_HOME:
          if (getEvent(EV_ERROR))
            break;
          currentMode = CTL_MODE_GOTO;
          motorA1.setVmax(slewingSpeeds.speed1);
          motorA1.setTargetPos(geoA1.homeDef);
          motorA2.setVmax(slewingSpeeds.speed2);
          motorA2.setTargetPos(geoA2.homeDef);
          setEvents(EV_GOING_HOME);
          setEvents(EV_SLEWING);
          resetEvents(EV_AT_HOME);
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
          previousMode = currentMode;
          currentMode = CTL_MODE_GOTO;
          motorA1.setVmax(fabs(speed)*geoA1.stepsPerSecond/SIDEREAL_SECOND);
          motorA1.setTargetPos(target);  
          setEvents(EV_SLEWING);
          resetEvents(EV_AT_HOME);
          break;

        case CTL_MSG_MOVE_AXIS2:
          target = msgBuffer[1];
          memcpy(&speed, &msgBuffer[2], sizeof(double));
          resetEvents(EV_AT_HOME);
          previousMode = currentMode;
          currentMode = CTL_MODE_GOTO;
          motorA2.setVmax(fabs(speed)*geoA2.stepsPerSecond/SIDEREAL_SECOND);
          motorA2.setTargetPos(target);  
          setEvents(EV_SLEWING);
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
          break;

        case CTL_MSG_STOP_AXIS2:
          motorA2.abort();
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

  target = mount.mP->poleDir(dir);

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
  enableGuideRate(RXX);
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_GOTO; 
  msg[1] = sP->steps1;
  msg[2] = sP->steps2;
  xQueueSend(controlQueue, &msg, 0);

  waitSlewing();

  DecayModeGoto();
  return ERRGOTO_NONE;
}
