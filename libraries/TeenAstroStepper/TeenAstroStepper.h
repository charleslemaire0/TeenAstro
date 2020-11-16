#pragma once
#include <TMC26XStepper.h>
#include <TMCStepper.h>

class Driver
{
public:
  enum MOTORDRIVER
  {
    NODRIVER, TMC26X, TMC2130, TMC5160
  };
private:
  MOTORDRIVER m_driver = NODRIVER;
  TMC26XStepper *m_tmc26x = NULL;
  TMC2130Stepper *m_tmc2130 = NULL;
  TMC5160Stepper *m_tmc5160 = NULL;
public:

  unsigned int getMode()
  {
    switch (m_driver)
    {
    case TMC26X:
      return m_tmc26x->isCoolStepEnabled();
    case TMC2130:
      return  m_tmc2130->en_pwm_mode();
    case TMC5160:
      return  m_tmc5160->en_pwm_mode();
    case NODRIVER:
      break;
    }
    return 0;
  }
  //get current in mA
  unsigned int getCurrent()
  {
    switch (m_driver)
    {
    case TMC26X:
      return m_tmc26x->getCurrentCurrent();
    case TMC2130:
      break;
    case TMC5160:
      break;
    case NODRIVER:
      break;
    }
    return 0;
  };

  unsigned int getSG()
  {
    switch (m_driver)
    {
    case TMC26X:
      return m_tmc26x->getCurrentStallGuardReading();
    case TMC2130:
      break;
    case TMC5160:
      break;
    case NODRIVER:
      break;
    }
    return 0;
  };

  void setSG(int i)
  {
    switch (m_driver)
    {
    case TMC26X:
      return m_tmc26x->setStallGuardThreshold((char)i, 0);
    case TMC2130:
      return;
    case TMC5160:
      break;
    case NODRIVER:
      break;
    }
    return;
  };

  void setmode(bool i)
  {
    switch (m_driver)
    {
    case TMC26X:
      m_tmc26x->setCoolStepEnabled(i);
      return;
    case TMC2130:
      m_tmc2130->en_pwm_mode(i);
      return;
    case TMC5160:
      m_tmc5160->en_pwm_mode(i);
      break;
    case NODRIVER:
      break;
    }
    return;
  };
  //set current in mA
  void setCurrent(unsigned int val)
  {
    switch (m_driver)
    {
    case TMC26X:
      m_tmc26x->setCurrent(val);
      break;
    case TMC2130:
      m_tmc2130->rms_current(val / sqrt(2), 0.25);
      break;
    case TMC5160:
      m_tmc5160->rms_current(val / sqrt(2), 0.25);
      break;
    case NODRIVER:
      break;
    };
  };
  // set microstep as power of 2
  void setMicrostep(int mu)
  {
    int val = (int)pow(2., mu);
    switch (m_driver)
    {
    case TMC26X:
      m_tmc26x->setMicrosteps(val);
      break;
    case TMC2130:
      m_tmc2130->microsteps(val);
      break;
    case TMC5160:
      m_tmc5160->microsteps(val);
      break;
    case NODRIVER:
      break;
    }
    return;
  };


  void initMotor(MOTORDRIVER useddriver, int StepRot, int EnPin, int CSPin, int DirPin, int StepPin, unsigned int Curr, int Micros, bool silent)
  {
    m_driver = useddriver;
    switch (m_driver)
    {
    case TMC26X:
      if (EnPin > 0)
        digitalWrite(EnPin, LOW);
      m_tmc26x = new TMC26XStepper(StepRot, CSPin, DirPin, StepPin, Curr);
      m_tmc26x->setSpreadCycleChopper(2, 24, 8, 6, 0);
      m_tmc26x->setRandomOffTime(0);
      setMicrostep(Micros);
      m_tmc26x->setStallGuardThreshold(63, -1);  //turn off SG
      m_tmc26x->setCoolStepConfiguration(480, 480, 1, 3, COOL_STEP_HALF_CS_LIMIT);
      m_tmc26x->setCoolStepEnabled(false);
      m_tmc26x->start();
      break;
    case TMC2130:
      m_tmc2130 = new TMC2130Stepper(CSPin);
      if (EnPin > 0)
      {
        pinMode(EnPin, OUTPUT);
        digitalWrite(EnPin, HIGH); //deactivate driver (LOW active)
      }
      pinMode(DirPin, OUTPUT);
      pinMode(StepPin, OUTPUT);
      pinMode(CSPin, OUTPUT);
      digitalWrite(DirPin, LOW); //LOW or HIGH
      digitalWrite(StepPin, LOW);
      digitalWrite(CSPin, HIGH);
      SPI.begin();
      pinMode(MISO, INPUT_PULLUP);
      m_tmc2130->push();
      m_tmc2130->reset();
      m_tmc2130->tbl(1);
      m_tmc2130->TPOWERDOWN(255);
      m_tmc2130->toff(5);
      m_tmc2130->hstrt(0);
      m_tmc2130->hend(2);
      m_tmc2130->en_pwm_mode(silent);
      m_tmc2130->pwm_autoscale(silent);
      //m_tmc2130->pwm_freq(150);
      //m_tmc2130->pwm_grad(15);
      m_tmc2130->TPWMTHRS(64);
      setCurrent(Curr); // mA
      m_tmc2130->intpol(1);
      setMicrostep(Micros);
      if (EnPin > 0)
        digitalWrite(EnPin, LOW);
      break;
    case TMC5160:
      m_tmc5160 = new TMC5160Stepper(CSPin);
      if (EnPin > 0)
      {
        pinMode(EnPin, OUTPUT);
        digitalWrite(EnPin, HIGH); //deactivate driver (LOW active)
      }
      pinMode(DirPin, OUTPUT);
      pinMode(StepPin, OUTPUT);
      pinMode(CSPin, OUTPUT);
      digitalWrite(DirPin, LOW);
      digitalWrite(StepPin, LOW);
      digitalWrite(CSPin, HIGH);
      SPI.begin();
      pinMode(MISO, INPUT_PULLUP);
      m_tmc5160->reset();
      m_tmc5160->push();
      m_tmc5160->tbl(1);
      m_tmc5160->TPOWERDOWN(255);
      m_tmc5160->toff(5);
      m_tmc5160->hstrt(0);
      m_tmc5160->hend(2);
      m_tmc5160->en_pwm_mode(silent);
      m_tmc5160->pwm_autoscale(silent);
      //m_tmc5160->pwm_freq(150);
      //m_tmc5160->pwm_grad(15);
      m_tmc5160->TPWMTHRS(64);
      m_tmc5160->intpol(1);
      setCurrent(Curr); // mA
      setMicrostep(Micros);
      if (EnPin > 0)
        digitalWrite(EnPin, LOW);
      break;
    case NODRIVER:
      break;
    }
  };
};
