#pragma once


enum MountType { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };

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

// Axis speeds in steps per second
struct Speeds
{
  double speed1;
  double speed2;
};

// Callbacks for guiding timers
void stopGuidingAxis1(TimerHandle_t xTimer);
void stopGuidingAxis2(TimerHandle_t xTimer);


// Virtual base class for Eq and Altaz 
class Mnt
{
public:
  MountType type;
  virtual byte goToEqu(EqCoords*);
  virtual byte goToHor(HorCoords *);
  virtual bool getEqu(double *HA, double *Dec, const double *cosLat, const double *sinLat, bool returnHA);
  virtual bool getHorApp(HorCoords*);
  virtual byte goTo(Steps*);
  virtual bool syncEqu(double HA, double Dec, PierSide Side, const double *cosLat, const double *sinLat);
  virtual bool syncAzAlt(double Azm, double Alt, PierSide Side);
  virtual byte Flip();
  virtual PierSide GetPierSide();
  virtual void startTracking(void);
  virtual void stopTracking(void);
  virtual void setTrackingSpeed(double speed);
  virtual void MoveAxis1(const byte dir);
  virtual void MoveAxis1AtRate(double speed, const byte dir);
  virtual void StopAxis1(void);
  virtual void MoveAxis2(const byte dir);
  virtual void MoveAxis2AtRate(double speed, const byte dir);
  virtual void StopAxis2(void);
  virtual void getTrackingSpeeds(Speeds *);
  virtual void initHome(Steps *);
  virtual void startGuiding(char, int);
  virtual bool getTargetPierSide(EqCoords *eP, PierSide *psOutP);
  virtual long poleDir(char);
  virtual int decDirection(void);
  double  trackingSpeed, guidingSpeed;  // multiple of sidereal speed 
  Speeds  trackingSpeeds;               // actual tracking speeds including guiding and spiral           
  bool axis1Guiding=false, axis2Guiding=false;
  TimerHandle_t axis1Timer, axis2Timer;
};

class EqMount : public Mnt 
{
public:
  byte goToEqu(EqCoords*);
  byte goToHor(HorCoords *);
  byte goTo(Steps*);
  bool eqToAxes(EqCoords*, Axes*, PierSide);
  bool getEqu(double *HA, double *Dec, const double *cosLat, const double *sinLat, bool returnHA);  
  bool getHorApp(HorCoords *hP);
  void axesToSteps(Axes*, Steps*);
  void stepsToAxes(Steps*, Axes*);
  void axesToEqu(Axes*, EqCoords*);
  bool syncEqu(double HA, double Dec, PierSide Side, const double *cosLat, const double *sinLat);
  bool syncAzAlt(double Azm, double Alt, PierSide Side);
  byte Flip(void);
  PierSide GetPierSide(void);
  PierSide otherSide(PierSide ps);
  bool isFlipped(void);
  bool checkPole(long, CheckMode);
  bool checkMeridian(Axes*, CheckMode, PierSide);
  bool withinLimits(long, long);
  void startTracking(void);
  void stopTracking(void);
  void setTrackingSpeed(double speed);
  void MoveAxis1(const byte dir);
  void MoveAxis1AtRate(double speed, const byte dir);
  void StopAxis1(void);
  void MoveAxis2(const byte dir);
  void MoveAxis2AtRate(double speed, const byte dir);
  void StopAxis2(void);
  void getTrackingSpeeds(Speeds *);
  void initHome(Steps *);
  void startGuiding(char, int);
  bool getTargetPierSide(EqCoords *eP, PierSide *psOutP);
  long poleDir(char);
  int decDirection(void);
  // constructor
  EqMount(MountType t)
  {
    type = t;
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
};


class AltAzMount : public Mnt 
{
public:
  void axesToSteps(Axes *, Steps *);
  void stepsToAxes(Steps *, Axes *);
  bool horToAxes(HorCoords *, Axes *);
  void axesToHor(Axes *, HorCoords *);
  byte goToEqu(EqCoords*);
  byte goToHor(HorCoords *);
  bool getEqu(double *HA, double *Dec, const double *cosLat, const double *sinLat, bool returnHA);
  bool getHorApp(HorCoords *hP);
  byte goTo(Steps*);
  bool syncEqu(double HA, double Dec, PierSide Side, const double *cosLat, const double *sinLat);
  bool syncAzAlt(double Azm, double Alt, PierSide Side);
  bool withinLimits(long, long);
  byte Flip(void);
  PierSide GetPierSide(void);
  void startTracking(void);
  void stopTracking(void);
  void setTrackingSpeed(double speed);
  void MoveAxis1(const byte dir);
  void MoveAxis1AtRate(double speed, const byte dir);
  void StopAxis1(void);
  void MoveAxis2(const byte dir);
  void MoveAxis2AtRate(double speed, const byte dir);
  void StopAxis2(void);
  void getTrackingSpeeds(Speeds *);
  void initHome(Steps *);
  void startGuiding(char, int);
  bool getTargetPierSide(EqCoords *eP, PierSide *psOutP);
  long poleDir(char);
  int decDirection(void);
  AltAzMount(MountType t)
  {
    type = t;
  }
};



class Mount 
{
public:
  double    maxSpeed;               // steps per second
  double    DegreesForAcceleration;
  double    trackingSpeed;          // multiple of sidereal 
  Backlash  backlashA1;
  Backlash  backlashA2;
  Mnt *mP;
  void init(MountType t)
  {
    switch(t)
    {
      case MOUNT_TYPE_GEM:
      case MOUNT_TYPE_FORK:
        mP = new EqMount(t);
        break;

      case MOUNT_TYPE_ALTAZM:
      case MOUNT_TYPE_FORK_ALT:
        mP = new AltAzMount(t);
        break;
      case MOUNT_UNDEFINED:
        break;
    }
  }
};

