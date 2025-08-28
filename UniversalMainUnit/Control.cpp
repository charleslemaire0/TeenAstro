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
  motorA1.setCurrent((unsigned int)motorA1.lowCurr); 
  motorA2.setCurrent((unsigned int)motorA2.lowCurr);
}

void DecayModeGoto()
{
  motorA1.setCurrent((unsigned int)motorA1.highCurr); 
  motorA2.setCurrent((unsigned int)motorA2.highCurr);
}

// Weird algorithm to prevent GEM mounts from pointing below horizon while slewing 
void adjustSpeeds(void)
{
  static int n = 0;                       // counter to only run once every 100mS
  enum adjAxis {NONE, AXIS1, AXIS2};
  static adjAxis ajx = NONE;
  int decDir = mount.mP->decDirection();
  double k;

  n++;
  if ((n % 100) != 0)
    return;

  if (decDir == 0)
    return;

  float aboveLimit = currentAzAlt.alt - limits.minAlt;

  if (aboveLimit < mount.DegreesForAcceleration)  
  {
    k = fmax(0.3, (aboveLimit < 0 ? 0 : aboveLimit / mount.DegreesForAcceleration));
    if (decDir > 0)
    {
      ajx = AXIS1;
      motorA1.setVmax(slewingSpeeds.speed1 * k);      
    }
    else
    {
      ajx = AXIS2;
      motorA2.setVmax(slewingSpeeds.speed2 * k);      
    }
  }
  else // above limit: reset speed to max
  {
    switch (ajx)
    {
      case AXIS1: motorA1.setVmax(slewingSpeeds.speed1); ajx = NONE; break;    
      case AXIS2: motorA2.setVmax(slewingSpeeds.speed2); ajx = NONE; break;
      case NONE: break;    
    }
  }  
}



int fsign(double n)
{
  if (n > 0.0)
    return 1;
  if (n < 0.0)
    return -1;
  return 0;
}


void controlTask(UNUSED(void *arg))
{
  TickType_t xLastWakeTime;
  const TickType_t xPeriod =  pdMS_TO_TICKS(CTRL_TASK_PERIOD);  // in mS
  CTL_MODE currentMode = CTL_MODE_IDLE;
  long axis1Target, axis2Target;
  long tickCount = 0;
  CTL_MODE prevMode;
  int dir, previousDirAxis2;

  while (1)
  { 
    xLastWakeTime = xTaskGetTickCount();  
    tickCount++;
    // Perform action according to current mode
    switch (currentMode)
    {
      case CTL_MODE_IDLE:
        if (getEvent(EV_START_TRACKING))
        {
          currentMode = CTL_MODE_TRACKING;
          DecayModeTracking();
          resetEvents(EV_AT_HOME);
          resetEvents(EV_START_TRACKING);
          setEvents(EV_TRACKING | EV_SPEED_CHANGE);
          previousDirAxis2 = 0;
        }
        break;

      case CTL_MODE_TRACKING:
        {
          Speeds speeds;

          // if we are centering, don't change mode
          if (getEvent(EV_CENTERING))
            break;
#ifdef HASST4           
          checkST4();
#endif          
          // Only need to update speed in case of guiding/spiral, or for an AltAz mount
          if (getEvent(EV_SPEED_CHANGE) || (isAltAz() && (tickCount % 100 == 0)))
          {
            mount.mP->getTrackingSpeeds(&speeds);
            resetEvents(EV_SPEED_CHANGE);
            motorA1.setVmax(fabs(speeds.speed1));
            dir = fsign(speeds.speed1);
            // only set target if direction has changed (no. in some cases this fails)
//            if (dir != previousDirAxis1)
            {
              motorA1.setTargetPos(dir * geoA1.stepsPerRot);
//              previousDirAxis1 = dir;
            }
            motorA2.setVmax(fabs(speeds.speed2));
            dir = fsign(speeds.speed2);
//            if (dir != previousDirAxis2)
            {
              motorA2.setTargetPos(dir * geoA2.stepsPerRot);
//              previousDirAxis2 = dir;
            }
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
      resetEvents(EV_START_TRACKING | EV_ABORT);
      parkStatus(PRK_UNPARKED);
      currentMode = CTL_MODE_STOPPING;
    }
  
  unsigned msgBuffer[CTL_MAX_MESSAGE_SIZE];
  double speed;
  long target;
  byte dir;
  bool t;

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
          resetEvents(EV_AT_HOME | EV_TRACKING | EV_START_TRACKING | EV_SPIRAL | EV_GUIDING_AXIS1 | EV_GUIDING_AXIS2);
          axis1Target = msgBuffer[1];
          axis2Target = msgBuffer[2];
          DecayModeGoto();
          motorA1.setVmax(slewingSpeeds.speed1);
          motorA1.setTargetPos(axis1Target);
          motorA2.setVmax(slewingSpeeds.speed2);
          motorA2.setTargetPos(axis2Target);
          setEvents(EV_SLEWING);
          break;

        case CTL_MSG_GOTO_HOME:
          if (lastError() != ERRT_NONE)
            break;
          currentMode = CTL_MODE_GOTO;
          resetEvents(EV_AT_HOME | EV_TRACKING | EV_START_TRACKING | EV_SPIRAL | EV_GUIDING_AXIS1 | EV_GUIDING_AXIS2);
          DecayModeGoto();
          motorA1.setVmax(slewingSpeeds.speed1);
          motorA1.setTargetPos(geoA1.homeDef);
          motorA2.setVmax(slewingSpeeds.speed2);
          motorA2.setTargetPos(geoA2.homeDef);
          setEvents(EV_GOING_HOME);
          setEvents(EV_SLEWING);
          break;

        case CTL_MSG_SYNC:
          if (lastError() != ERRT_NONE)
            break;
          t = isTracking();
          axis1Target = msgBuffer[1];
          axis2Target = msgBuffer[2];
          motorA1.syncPos(axis1Target);
          motorA2.syncPos(axis2Target);
          // After synchronizing the motors, restart tracking if needed
          if (t)
          {
            startTracking();
          }
          break;

        case CTL_MSG_MOVE_AXIS1:
          prevMode = currentMode;
          dir = msgBuffer[1];
          target = mount.mP->axis1Direction(dir) * geoA1.stepsPerRot;
          memcpy(&speed, &msgBuffer[2], sizeof(double));
          resetEvents(EV_AT_HOME);
          DecayModeGoto();
          motorA1.setVmax(fabs(speed)*geoA1.stepsPerSecond/SIDEREAL_SECOND);
          motorA1.setTargetPos(target);  
          if (dir == 'w')
            setEvents(EV_CENTERING | EV_WEST);
          else
            setEvents(EV_CENTERING | EV_EAST);
          resetEvents(EV_AT_HOME);
          break;

        case CTL_MSG_MOVE_AXIS2:
          dir = msgBuffer[1];
          target = mount.mP->axis2Direction(dir) * geoA2.stepsPerRot;
          memcpy(&speed, &msgBuffer[2], sizeof(double));
          resetEvents(EV_AT_HOME);
          DecayModeGoto();
          motorA2.setVmax(fabs(speed)*geoA2.stepsPerSecond/SIDEREAL_SECOND);
          motorA2.setTargetPos(target);  
          if (dir == 'n')
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
          if (prevMode == CTL_MODE_TRACKING)
          {
            currentMode = CTL_MODE_STOPPING;
            setEvents(EV_START_TRACKING);
          }
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
  setEvents(EV_START_TRACKING | EV_SPEED_CHANGE);  
}

void stopTracking(void)
{
  if (getEvent(EV_SPIRAL))
    stopSpiral();
  stopMoving();
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
  msg[1] = dir;
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

  if ((parkStatus() != PRK_UNPARKED) || isSlewing())
    return;

  msg[0] = CTL_MSG_MOVE_AXIS2; 
  msg[1] = dir;
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
  unsigned msg[CTL_MAX_MESSAGE_SIZE];

  if (parkStatus() == PRK_PARKED)
    return(ERRGOTO_PARKED);

  setSlewSpeed(guideRates[RXX]);
  // initiate the goto
  msg[0] = CTL_MSG_GOTO; 
  msg[1] = sP->steps1;
  msg[2] = sP->steps2;
  xQueueSend(controlQueue, &msg, 0);

  waitSlewing();

  return ERRGOTO_NONE;
}

byte sync(Steps *sP)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];

  if (parkStatus() == PRK_PARKED)
    return(ERRGOTO_PARKED);

  msg[0] = CTL_MSG_SYNC; 
  msg[1] = sP->steps1;
  msg[2] = sP->steps2;
  xQueueSend(controlQueue, &msg, 0);

  return ERRGOTO_NONE;
}



void setSlewSpeed(double speed)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];

  // set max slew speed without updating current guide rate
  msg[0] = CTL_MSG_SET_SLEW_SPEED;
  memcpy (&msg[1], &speed, sizeof(double)); 
  xQueueSend(controlQueue, &msg, 0);  
}
