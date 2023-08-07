#pragma once
#include <TeenAstroStepper.h>
#define default_tracking_rate 1
//geometry Axis
// backlash control


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
  volatile double     interval_Step_Sid;               // based on the siderealClockSpeed, this is the time between steps for sidereal tracking 
  volatile double     takeupRate;                      // this is the takeup rate for synchronizing the target and actual positions when needed
  volatile double     takeupInterval;
  volatile double     interval_Step_Cur = 0;                         // this is the time between steps for the current rotation speed
  volatile double     CurrentTrackingRate = default_tracking_rate;   //effective rate tracking in Hour arc-seconds/second
  double              RequestedTrackingRate = default_tracking_rate; //computed  rate tracking in Hour arc-seconds/second
  long                minstepdist;
  double              ClockSpeed;
  volatile bool       backlash_correcting;
  volatile int        backlash_movedSteps;
  volatile double     backlash_interval_Step;
  volatile int        backlash_inSteps;
public:
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
  void setSidereal(double siderealClockSpeed, double stepsPerSecond, double cs, int bl_rate)
  {
    ClockSpeed = cs;
    interval_Step_Sid = siderealClockSpeed / stepsPerSecond;
    minstepdist = 0.25 * stepsPerSecond;
    takeupRate = 8L;
    takeupInterval = interval_Step_Sid / takeupRate;
    SetBacklash_interval_Step(bl_rate);
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
  //double speedfromTarget()
  //{
  //  volatile unsigned long delta = abs(deltaTarget);
  //  return speed2interval(speedfromDist(delta));
  //}

  long breakDist()
  {
    return (long)(pow(interval2speed(interval_Step_Cur), 2.) / (2. * acc));
  };

  void breakMoveLowRate()
  {
    updateDeltaTarget();
    long a = breakDist();
    if (abs(deltaTarget) > a)
    {
      if (0 > deltaTarget)
        a = -a;
      cli();
      target = pos + a;
      deltaTarget = a;
      sei();
    }
  };
  void breakMoveHighRate()
  {
    updateDeltaTarget();
    long a = breakDist();
    if (0 > deltaTarget) 
      a = -a;
    cli();
    target = pos + a;
    deltaTarget = a;
    sei();
  };
  void setIntervalfromDist(const volatile unsigned long& d, double minInterval, double maxInterval)
  {
    interval_Step_Cur = max(speed2interval(speedfromDist(d), maxInterval), minInterval);
  };
  void setIntervalfromRate(double rate, double minInterval, double maxInterval)
  {
    if (rate == 0)
    {
      interval_Step_Cur = maxInterval;
    }
    else
    {
      interval_Step_Cur = max(min(interval_Step_Sid / rate, maxInterval), minInterval);
    }
  };
  void move()
  {
    if (dir)
    {
      if (backlash_movedSteps < backlash_inSteps)
      {
        backlash_movedSteps++;
        backlash_correcting = true;
      }
      else
      {
        backlash_correcting = false;
        pos++;
      }
    }
    else
    {
      if (backlash_movedSteps > 0)
      {
        backlash_movedSteps--;
        backlash_correcting = true;
      }
      else
      {
        backlash_correcting = false;
        pos--;
      }
    }
  }
  void setBacklash_inSteps(int amount, double stepsPerArcSecond)
  {
    backlash_inSteps = amount * stepsPerArcSecond;
    backlash_movedSteps = 0;
    backlash_correcting = false;
  }
  void SetBacklash_interval_Step(int bl_rate)
  {
    backlash_interval_Step = interval_Step_Sid / bl_rate;
  }
private:
  double speedfromDist(const volatile unsigned long& d)
  {
    return sqrt((long double)d * 2. * acc);
  };
  long double interval2speed(double interval)
  {
    return ClockSpeed / interval;
  }
  double speed2interval(double V, double maxInterval)
  {
    if (V == 0)
    {
      return maxInterval;
    }
    return min(ClockSpeed / V, maxInterval);
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
  float  LimMinAxis; //in deg
  float  LimMaxAxis; //in deg

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
  long            duration;
  unsigned long   durationLast;
  double          absRate;
private:
  enum moveStatus { MBW = -2, BBW = -1, Idle = 0, BFW = 1, MFW = 2 };
  volatile moveStatus m_mst;
  volatile double  m_amount;
  double* m_stepsPerCentiSecond;
public:
  bool isMFW()
  {
    return m_mst == MFW;
  }
  bool isMBW()
  {
    return m_mst == MBW;
  }
  bool isDirFW()
  {
    return m_mst >= BFW;
  }
  bool isDirBW()
  {
    return m_mst <= BBW;
  }
  bool isBusy()
  {
    return m_mst != Idle;
  }
  bool isBraking()
  {
    return  m_mst == BBW || m_mst == BFW;
  }
  bool isMoving()
  {
    return m_mst == MFW || m_mst == MBW;
  }
  double getRate()
  {
    if (isDirFW())
    {
      return absRate;
    }
    else if (isDirBW())
    {
      return -absRate;
    }
    return 0;
  }
  double getAmount()
  {
    if (isDirFW())
    {
      return m_amount;
    }
    else if (isDirBW())
    {
      return -m_amount;
    }
    return 0;
  }
  void setIdle()
  {
    m_mst = Idle;
  }
  void moveFW()
  {
    m_mst = MFW;
  }
  void moveBW()
  {
    m_mst = MBW;
  }
  void brake()
  {
    if (isMFW())
    {
      m_mst = BFW;
    }
    if (isMBW())
    {
      m_mst = BBW;
    }
  }

  void init(double* stepsPerCentiSecond, double rate)
  {
    m_mst = Idle;
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
      m_amount = absRate * *m_stepsPerCentiSecond;
      sei();
    }
  }
};

class MotorAxis
{
public:
  unsigned long gear; //1000 time the gear in order to 3 digits after the comma
  bool isGearFix;
  unsigned int stepRot;
  bool isStepRotFix;
  byte micro;
  bool isMicroFix;
  bool reverse;
  bool isReverseFix;
  bool silent;
  bool isSilentFix;
  unsigned int highCurr; //in mA
  bool isHighCurrfix;
  unsigned int lowCurr; //in mA
  bool isLowCurrfix;
  int backlashAmount;
  int backlashRate;
  Driver driver;
  void initMotor(Driver::MOTORDRIVER useddriver, int EnPin, int CSPin, int DirPin, int StepPin)
  {
    driver.initMotor(useddriver, stepRot, EnPin, CSPin, DirPin, StepPin, 200, micro, silent);
  }
};