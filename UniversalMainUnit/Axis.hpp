#pragma once
// Geometry Axis
// Backlash Control


class GeoAxis
{
public:
  long   homeDef;   //in steps
  long   westDef;   // infinity on the west side
  long   eastDef;   // infinity on the east side
  long   stepsPerRot; // calculated as    :  stepper_steps * micro_steps * gear_reduction1 * (gear_reduction2/360)
  double stepsPerDegree;
  double stepsPerSecond;
  long   halfRot;   //in steps
  long   quarterRot; //in steps
  long   minAxis;   //in degrees
  long   maxAxis;   //in degrees
private:
  long   m_brakeDist; //in steps
 public:
  bool atTarget(const volatile long &deltaTarget, double factor)
  {
    return abs(deltaTarget) < max (m_brakeDist * factor, (double) m_brakeDist);
  }
  void setstepsPerRot(long val)
  {
    stepsPerRot = val;
    stepsPerDegree = stepsPerRot / 360.0;
    stepsPerSecond = stepsPerRot / 86400.0;
    halfRot = stepsPerRot / 2L;
    quarterRot = stepsPerRot / 4L;
    m_brakeDist = max(2.0, stepsPerDegree / 3600.0 * 0.2);
    return;
  }
  bool withinLimits(long axis)
  {
    return (axis >= minAxis && axis <= maxAxis);
  }
};

class GuideAxis
{
public:
  volatile byte   dir;
  long            duration;
  unsigned long   durationLast;
  double          amount;
  volatile double timerRate;
};

