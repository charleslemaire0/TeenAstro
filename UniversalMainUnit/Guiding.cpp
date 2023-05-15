#include "Global.h"

TimerHandle_t axis1Timer, axis2Timer, spiralTimer;
#define SPIRAL_PERIOD     200         // mS
#define MAX_SPIRAL_TIME   600000      // mS
static long spiralCount;
static double SpiralFOV;    // field of view in degrees
Speeds spiralSpeed;

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
  spiralTimer = xTimerCreate
               ( "Spiral Timer",
                 SPIRAL_PERIOD,           
                 pdTRUE,
                 NULL,
                 spiralTask);    
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

/*
 * startSpiral
 *
 * start a timer that periodically updates the tracking speed on both axes 
 */
void startSpiral(double fov)
{
  if ((parkStatus() != PRK_UNPARKED) || isSlewing() || !(isTracking()))
    return;

  spiralCount = 0;
  SpiralFOV = fov;
  xTimerStart(spiralTimer, 0);
  setEvents(EV_SPIRAL);
}

/*
 * equation of movement: R = θ = √t
 * 
 *  x = √t * cos (√t)
 *  y = √t * sin (√t)
 *
 * equation of speed
 *
 * dx/dt = (cos(√t) - √t * sin(√t)) / 2 * √t
 * dy/dt = (sin(√t) + √t * cos(√t)) / 2 * √t
 *
 */
void spiralTask(UNUSED(TimerHandle_t xTimer))
{
  int k = 180;    // magic number to get requested distance between spiral paths 
  spiralCount++;

  if (spiralCount >= (MAX_SPIRAL_TIME / SPIRAL_PERIOD))
  {
    stopSpiral();
  }

  double T = sqrtf(spiralCount);
  double sinT = sin(T);
  double cosT = cos(T);

  xSemaphoreTake(swMutex, portMAX_DELAY); 
  spiralSpeed.speed1 = k * SpiralFOV * (cosT - T * sinT) / (2 * T);
  spiralSpeed.speed2 = k * SpiralFOV * (sinT + T * cosT) / (2 * T);
  xSemaphoreGive(swMutex);

  setEvents(EV_SPEED_CHANGE);
}

void stopSpiral(void)
{
  xTimerStop(spiralTimer, 0);
  spiralSpeed.speed1 = 0;
  spiralSpeed.speed2 = 0;
  resetEvents(EV_SPIRAL);  
}

// speeds are in multiple of sidereal
void getSpiralSpeeds(Speeds *vP)
{
  xSemaphoreTake(swMutex, portMAX_DELAY); 
  vP->speed1 = spiralSpeed.speed1;
  vP->speed2 = spiralSpeed.speed2;
  xSemaphoreGive(swMutex);
}
