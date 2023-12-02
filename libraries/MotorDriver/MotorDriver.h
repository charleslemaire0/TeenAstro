#include "MotionControl.h"

enum DriverType { DRV_UNDEFINED, DRV_STEPDIR, DRV_SPI};

class MotorDriver
{
public:
  TMC5160Stepper *drvP = NULL;
  MotionControl *mcP = NULL;
  unsigned long gear;         // 1000x the gear to allow 3 digits after the comma
  unsigned int stepRot;
  byte micro = 4;
  bool reverse = false;
  bool silent = true;
  unsigned int highCurr=100; //in mA
  unsigned int lowCurr=100; //in mA
  SemaphoreHandle_t mutex = 0;
  int CSPin, EnPin;
  DriverType drvType = DRV_UNDEFINED;

  // generic init - call this one first
  void init(int cspin, int enpin, SemaphoreHandle_t mtx)
  {
    mutex = mtx;

    CSPin = cspin;
    EnPin = enpin;
    if (EnPin > 0)
    {
      pinMode(EnPin, OUTPUT);
      digitalWrite(EnPin, HIGH);  // deactivate
    }    
    drvP = new TMC5160Stepper(CSPin);
    // Initialize 5160 through SPI
    drvP->begin();  
    drvP->reset();
    drvP->push();
    drvP->TPOWERDOWN(255);		// leave at default for faster powerdown
    drvP->tbl(2);
    drvP->toff(3);
    drvP->hstrt(0);
    drvP->hend(0);
    drvP->irun(18);
    drvP->ihold(5);
    drvP->pwm_ofs(91);
    drvP->pwm_grad(30);
    drvP->GLOBAL_SCALER(104);
    drvP->en_pwm_mode(silent);
    drvP->pwm_autoscale(silent);
    drvP->TPWMTHRS(1024);
    drvP->intpol(1);
    drvP->hold_multiplier(0.1);
    setCurrent(lowCurr);
    setMicrostep(micro);
    if (EnPin > 0)
    {
      digitalWrite(EnPin, LOW);  // activate
    }    
  }

  // StepDir constructor
  void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId)
  {
    pinMode(DirPin, OUTPUT);
    digitalWrite(DirPin, LOW);

    pinMode(StepPin, OUTPUT);
    digitalWrite(StepPin, LOW);

    pinMode(CSPin, OUTPUT);
    digitalWrite(CSPin, HIGH);

    drvP->dedge(true);		// enable double edge step impulse 

    // StepDir driver
    mcP = new StepDir();
    mcP->initStepDir(DirPin, StepPin, isrP, timerId);
    drvType = DRV_STEPDIR;
  }

  // 5160 MotionController constructor (SPI only)
  void initMc5160(SemaphoreHandle_t mtx, long clkFreq)
  {
    // MotionControl driver
    mcP = new Mc5160();
    mcP->initMc5160(drvP, mtx, clkFreq);
    drvType = DRV_SPI;
  }

  void setCurrent(unsigned int val)
  {
    if (mutex)
      xSemaphoreTake(mutex, portMAX_DELAY);
    drvP->rms_current(val / sqrt(2));
    if (mutex)
      xSemaphoreGive(mutex);
  }
  
  void setMicrostep(int mu)
  {
    int val = (int)pow(2., mu);
    if (mutex)
      xSemaphoreTake(mutex, portMAX_DELAY);
    drvP->microsteps(val);   
    if (mutex)
      xSemaphoreGive(mutex);
  }

  
  
  void setCurrentPos(long p)
  {
    mcP->setCurrentPos(reverse ? -p:p);
  }
  void syncPos(long p)
  {
    mcP->syncPos(reverse ? -p:p);
  }
  void setTargetPos(long p)
  {
    mcP->setTargetPos(reverse ? -p:p);
  }
  void setVmax(double v)
  {
    mcP->setVmax(v);
  }
  void setAmax(long a)
  {
    mcP->setAmax(a);
  }
  long getCurrentPos()
  {
    long p = mcP->getCurrentPos();
    return (reverse ? -p:p); 
  }
  long getTargetPos()
  {
    long p = mcP->getTargetPos(); 
    return (reverse ? -p:p); 
  }
  double getSpeed()
  {
    double v = mcP->getSpeed(); 
    return (reverse ? -v:v); 
  }
  bool positionReached()
  {
    bool p = mcP->positionReached(); 
    return p;
  }
  bool isMoving()
  {
    bool p = mcP->isMoving(); 
    return p;
  }
  void abort()
  {
    mcP->abort(); 
  }
  void resetAbort()
  {
    mcP->resetAbort(); 
  }
  void setRatios(long fkHz)
  {
    mcP->setRatios(fkHz); 
  }
  void enable(void)
  {
    mcP->enable();     
  }
  void disable(void)
  {
    mcP->disable();     
  }

};
