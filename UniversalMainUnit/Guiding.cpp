#include "Global.h"

TimerHandle_t axis1Timer, axis2Timer;

void initGuiding(void)
{
  axis1Timer = xTimerCreate
               ( "axis1 Guiding Timer",
                 1000,            
                 pdFALSE,
                 NULL,
                 stopGuidingAxis1);  
  axis2Timer = xTimerCreate
               ( "axis2 Guiding Timer",
                 1000,            
                 pdFALSE,
                 NULL,
                 stopGuidingAxis2);    
}


/*
 * startGuiding
 * add guiding speed to tracking speed
 */
void startGuiding(char dir, int milliseconds)
{
  if (!isTracking())
    return;
  switch(dir)
  {
    case 'w':
      if (! (getEvent(EV_GUIDING_AXIS1)))
      {
        xTimerChangePeriod(axis1Timer, milliseconds, 0);
        xTimerStart(axis1Timer, 0);
        setEvents(EV_GUIDING_AXIS1 | EV_WEST | EV_SPEED_CHANGE);
      }
      break;
    case 'e':
      if (! (getEvent(EV_GUIDING_AXIS1)))
      {
        xTimerChangePeriod(axis1Timer, milliseconds, 0);
        xTimerStart(axis1Timer, 0);
        setEvents(EV_GUIDING_AXIS1 | EV_EAST | EV_SPEED_CHANGE);
      }
      break;
    case 'n':
      if (! (getEvent(EV_GUIDING_AXIS2)))
      {
        xTimerChangePeriod(axis2Timer, milliseconds, 0);
        xTimerStart(axis2Timer, 0);
        setEvents(EV_GUIDING_AXIS2 | EV_NORTH | EV_SPEED_CHANGE);
      }
      break;
    case 's':
      if (! (getEvent(EV_GUIDING_AXIS2)))
      {
        xTimerChangePeriod(axis2Timer, milliseconds, 0);
        xTimerStart(axis2Timer, 0);
        setEvents(EV_GUIDING_AXIS2 | EV_SOUTH | EV_SPEED_CHANGE);
      }
      break;
  }
}

// Callbacks for guiding timers - reset tracking speeds to default
void stopGuidingAxis1(UNUSED(TimerHandle_t xTimer))
{
  resetEvents(EV_GUIDING_AXIS1 | EV_EAST | EV_WEST);
  setEvents(EV_SPEED_CHANGE);
}

void stopGuidingAxis2(UNUSED(TimerHandle_t xTimer))
{
  resetEvents(EV_GUIDING_AXIS2 | EV_NORTH | EV_SOUTH);
  setEvents(EV_SPEED_CHANGE);
}
