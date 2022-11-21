#pragma once
#include <TeenAstroStepper.h>
#define default_tracking_rate 1
//geometry Axis
// backlash control
struct backlash
{
  int               inSeconds;
  volatile int      inSteps;
  volatile bool     correcting;
  volatile int      movedSteps;
  volatile double   interval_Step;
};

class StatusAxis
{
public:
  bool                enable = false;
  bool                fault = false;
  volatile double     acc = 0;                         // acceleration in steps per second square
  volatile long       pos;                             // angle position in steps
  volatile long       start;                           // angle of goto start position in steps
  volatile double     target;                          // angle of goto end   position in steps
  volatile long       deltaTarget;
  volatile long       deltaStart;
  volatile bool       dir;                             // stepping direction + or -
  double              fstep;                           // amount of steps for Tracking
  volatile double     interval_Step_Sid;                  // based on the siderealClockSpeed, this is the time between steps for sidereal tracking 
  volatile double     takeupInterval;                      // this is the takeup rate for synchronizing the target and actual positions when needed
  volatile double     interval_Step_Cur = 0;              // this is the time between steps for the current rotation speed
  volatile double     CurrentTrackingRate = default_tracking_rate; //effective rate tracking in Hour arc-seconds/second
  double              RequestedTrackingRate = default_tracking_rate; //computed  rate tracking in Hour arc-seconds/second
  long                minstepdist;
  double              ClockSpeed;
  void updateDeltaTarget()
  {
    cli();
    deltaTarget = (long)target - pos;
    sei();
  };
  void updateDeltaStart()
  {
    cli();
    deltaStart = pos - start;
    sei();
  };
  bool atTarget(bool update)
  {
    if (update)
    {
      updateDeltaTarget();
    }
    return abs(deltaTarget) < max(minstepdist * RequestedTrackingRate, 1);
  };
  void resetToSidereal()
  {
    interval_Step_Cur = interval_Step_Sid;
  };
  void setSidereal(double siderealClockSpeed, double stepsPerSecond, double cs)
  {
    ClockSpeed = cs;
    interval_Step_Sid = siderealClockSpeed / stepsPerSecond;
    minstepdist = 0.25 * stepsPerSecond;
    takeupInterval = interval_Step_Sid / 8L;
    resetToSidereal();
  };
  //double interval2speedfromTime(const double& time)
  //{
  //  
  //}
  //double GetTimeToBreak()
  //{
  //  //to do timerRate is not signed!!
  //  return abs(interval2speed(timerRate) - interval2speed(siderealRate / RequestedTrackingRate)) / (2 * acc);
  //}
  //double speedfromDist2(volatile long& distDestAxis1)
  //{
  //  //first compute the time we need to get that position
  //  // solve a*t^2 + b*t + c = 0 => a0 * t^2 + v0 * t - deltaP = 0
  //  // with a = acc, b = v0 (current speed) 
  //  //compute determinant b2 ? 4ac
  //  double v0 = interval2speed(timerRate);
  //  double deter = pow(v0, 2) + 4 * acc * distDestAxis1;
  //  double t = (sqrt(deter) - v0) / (2 * acc);
  //  //then compute speed from the time V = 2*acc*t+V0
  //  return 2 * acc * t + v0;
  //};
  //long breakDist2()
  //{
  //  double t = GetTimeToBreak();
  //  return acc* pow(t, 2) + interval2speed(timerRate) * t;
  //};
  double speedfromTarget()
  {
    volatile unsigned long delta = abs(deltaTarget);
    return speed2interval(speedfromDist(delta));
  }
  double speedfromDist(const volatile unsigned long& distDestAxis1)
  {
    return sqrt(distDestAxis1 * 4. * acc);
  };
  long breakDist()
  {
    return (long)pow(interval2speed(interval_Step_Cur), 2.) / (4. * acc);
  };
  void breakMove()
  {
    updateDeltaTarget();
    long a = breakDist();
    if (abs(deltaTarget) > a)
    {
      if (0 > deltaTarget) // overshoot
        a = -a;
      cli();
      target = pos + a;
      sei();
    }
  }
private:
  double interval2speed(double interval)
  {
    return ClockSpeed / interval;
  }
  double speed2interval(double V)
  {
    return ClockSpeed / V;
  }
};

class GeoAxis
{
public:
  long   poleDef;   //in steps
  long   homeDef;   //in steps
  long   stepsPerRot; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  double stepsPerDegree;
  double stepsPerArcSecond;
  double stepsPerSecond;
  double stepsPerCentiSecond;
  long   halfRot;   //in steps
  long   quaterRot; //in steps
  long   minAxis;   //in steps
  long   maxAxis;   //in steps
private:
  long   m_breakDist; //in steps
public:
  void setstepsPerRot(long val)
  {
    stepsPerRot = val;
    stepsPerDegree = stepsPerRot / 360.0;
    stepsPerSecond = stepsPerRot / 86400.0;
    stepsPerArcSecond = stepsPerDegree / 3600.0;
    stepsPerCentiSecond = stepsPerRot / 8640000.0;
    halfRot = stepsPerRot / 2L;
    quaterRot = stepsPerRot / 4L;
    return;
  }
  bool withinLimit(const long& axis)
  {
    return !(axis < minAxis || axis > maxAxis);
  }
};

class GuideAxis
{
public:
  volatile byte   dir;
  long            duration;
  unsigned long   durationLast;
  double          amount;
  volatile double atRate;
  double          absRate;
private:
  double* m_stepsPerCentiSecond;
public:
  void init(double* stepsPerCentiSecond, double rate)
  {
    dir = 0;
    duration = 0;
    m_stepsPerCentiSecond = stepsPerCentiSecond;
    enableAtRate(rate);
  }
  void enableAtRate(double rate)
  {
    if (absRate != rate)
    {
      absRate = rate;
      cli();
      amount = absRate * *m_stepsPerCentiSecond;
      sei();
    }
  }
};

class MotorAxis
{
public:
  unsigned int gear;
  unsigned int stepRot;
  byte micro;
  bool reverse;
  bool silent;
  unsigned int highCurr; //in mA
  unsigned int lowCurr; //in mA
  Driver driver;
  void initMotor(Driver::MOTORDRIVER useddriver, int EnPin, int CSPin, int DirPin, int StepPin)
  {
    driver.initMotor(useddriver, stepRot, EnPin, CSPin, DirPin, StepPin, lowCurr, micro, silent);
  }
};