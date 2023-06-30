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

// Axis speeds
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
  virtual void stepsToAxes(Steps*, Axes*);
  virtual bool syncEqu(double HA, double Dec, PierSide Side, const double *cosLat, const double *sinLat);
  virtual bool syncAzAlt(double Azm, double Alt, PierSide Side);
  virtual byte Flip();
  virtual bool checkMeridian(Axes*, CheckMode, PierSide);
  virtual PierSide GetPierSide();
  virtual void setTrackingSpeed(double speed);
  virtual void getTrackingSpeeds(Speeds *);
  virtual void initHome(Steps *);
  virtual bool getTargetPierSide(EqCoords *eP, PierSide *psOutP);
  virtual int axis1Direction(char);
  virtual int axis2Direction(char);
  virtual int decDirection(void);
  virtual void updateRaDec(void);
  virtual bool checkPole(double axis1, CheckMode mode);
  virtual void initTransformation(bool reset);
  bool hasStarAlignment(void)
  {
    return isAligned;
  }
  void hasStarAlignment(bool b)
  {
    isAligned = b;
  }
  CoordConv alignment;
protected:
  double  trackingSpeed;            // multiple of sidereal speed 
  Speeds  trackingSpeeds;           // actual tracking speeds including guiding and spiral           
  double  currentRA, currentDec;
  bool    isAligned;
};






class EqMount : public Mnt 
{
public:
  byte goToEqu(EqCoords*);
  byte goToHor(HorCoords *);
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
  bool checkPole(double, CheckMode);
  bool checkMeridian(Axes*, CheckMode, PierSide);
  void setTrackingSpeed(double speed);
  void getTrackingSpeeds(Speeds *);
  void initHome(Steps *);
  bool getTargetPierSide(EqCoords *eP, PierSide *psOutP);
  int axis1Direction(char);
  int axis2Direction(char);
  int decDirection(void);
  void updateRaDec(void);
  void initTransformation(bool reset);
  // constructor
  EqMount(MountType t)
  {
    type = t;
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
  bool syncEqu(double HA, double Dec, PierSide Side, const double *cosLat, const double *sinLat);
  bool syncAzAlt(double Azm, double Alt, PierSide Side);
  byte Flip(void);
  bool checkMeridian(Axes*, CheckMode, PierSide);
  PierSide GetPierSide(void);
  PierSide otherSide(PierSide ps);
  void setTrackingSpeed(double speed);
  void getTrackingSpeeds(Speeds *);
  void initHome(Steps *);
  bool getTargetPierSide(EqCoords *eP, PierSide *psOutP);
  int axis1Direction(char);
  int axis2Direction(char);
  int decDirection(void);
  void updateRaDec(void);
  bool checkPole(double axis1, CheckMode mode);
  void initTransformation(bool reset);
  // constructor
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
  long    clk5160;                  // clock frequency of TMC5160 SPI mode (assume both motors use the same clock frequency)
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

