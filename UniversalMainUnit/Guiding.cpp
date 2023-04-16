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
        if (milliseconds != 0)
        {
          xTimerChangePeriod(axis1Timer, milliseconds, 0);
          xTimerStart(axis1Timer, 0);
        }
        setEvents(EV_GUIDING_AXIS1 | EV_WEST | EV_SPEED_CHANGE);
      }
      break;
    case 'e':
      if (! (getEvent(EV_GUIDING_AXIS1)))
      {
        if (milliseconds != 0)
        {
          xTimerChangePeriod(axis1Timer, milliseconds, 0);
          xTimerStart(axis1Timer, 0);
        }
        setEvents(EV_GUIDING_AXIS1 | EV_EAST | EV_SPEED_CHANGE);
      }
      break;
    case 'n':
      if (! (getEvent(EV_GUIDING_AXIS2)))
      {
        if (milliseconds != 0)
        {
          xTimerChangePeriod(axis2Timer, milliseconds, 0);
          xTimerStart(axis2Timer, 0);
        }
        setEvents(EV_GUIDING_AXIS2 | EV_NORTH | EV_SPEED_CHANGE);
      }
      break;
    case 's':
      if (! (getEvent(EV_GUIDING_AXIS2)))
      {
        if (milliseconds != 0)
        {
          xTimerChangePeriod(axis2Timer, milliseconds, 0);
          xTimerStart(axis2Timer, 0);
        }
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

#ifdef HASST4
void setupST4()
{
  pinMode(ST4RAw, INPUT);
  pinMode(ST4RAe, INPUT);
  pinMode(ST4DEn, INPUT);
  pinMode(ST4DEs, INPUT);
}

void checkST4()
{
  // Simulated ST4 with inactive signals
  byte w1 = HIGH, w2 = HIGH, e1 = HIGH, e2 = HIGH, n1 = HIGH, n2 = HIGH, s1 = HIGH, s2 = HIGH;
  static char ST4RA_state = 0;
  static char ST4RA_last = 0;
  static char ST4DE_state = 0;
  static char ST4DE_last = 0;

  // ST4 port is active only if there is no mount Error
  if (lastError() != ERRT_NONE)
  {
//    if (GuidingState == GuidingST4)
//    {
//      StopAxis1();
//      StopAxis2();
//    }
    return;
  }

  w1 = digitalRead(ST4RAw);
  e1 = digitalRead(ST4RAe);
  n1 = digitalRead(ST4DEn);
  s1 = digitalRead(ST4DEs);
  delayMicroseconds(5);
  w2 = digitalRead(ST4RAw);
  e2 = digitalRead(ST4RAe);
  n2 = digitalRead(ST4DEn);
  s2 = digitalRead(ST4DEs);

  if ((w1 == w2) && (e1 == e2))
  {
    ST4RA_state = 0;
    if (w1 != e1)
    {
      if (w1 == LOW)
        ST4RA_state = 'w';
      if (e1 == LOW)
        ST4RA_state = 'e';
    }
  }
  if ((n1 == n2) && (s1 == s2))
  {
    ST4DE_state = 0;
    if (n1 != s1)
    {
      if (n1 == LOW)
        ST4DE_state = 'n';
      if (s1 == LOW)
        ST4DE_state = 's';
    }
  }

  // RA changed?
  if (ST4RA_last != ST4RA_state)
  {
    ST4RA_last = ST4RA_state;
    if (ST4RA_state)
    {
      startGuiding(ST4RA_state, 0);
    }
    else
    {
      stopGuidingAxis1(nullptr);
    }
  }

  // Dec changed?
  if (ST4DE_last != ST4DE_state)
  {
    ST4DE_last = ST4DE_state;
    if (ST4DE_state)
    {
      startGuiding(ST4RA_state, 0);
    }
    else
    {
      stopGuidingAxis2(nullptr);
    }
  }
}
#endif