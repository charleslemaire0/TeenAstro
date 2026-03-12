#pragma once

struct Backlash
{
  int             inSeconds;
  volatile int    inSteps;
  volatile bool   correcting;
  volatile int    movedSteps;
  volatile double timerRate;
};

struct EqCoords
{
  double ha;
  double dec;
};

struct HorCoords
{
  double az;
  double alt;
};

struct Axes
{
  double axis1;
  double axis2;
};

struct Steps
{
  long steps1;
  long steps2;
};

// Axis speeds
struct Speeds
{
  double speed1;
  double speed2;
};
