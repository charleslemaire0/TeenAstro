#pragma once
#include <TMC26XStepper.h>
#include <TMCStepper.h>

class Motor
{
public:
  enum Motor_Driver { NODRIVER, TMC26X, TMC2130, TMC5160 };
private:
  Motor_Driver m_driver = NODRIVER;
  TMC26XStepper *m_tmc26x = NULL;
  TMC2130Stepper *m_tmc2130 = NULL;
  TMC5160Stepper *m_tmc5160 = NULL;
public:
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
  //set current in mA
  void setCurrent(unsigned int val)
  {
    switch (m_driver)
    {
    case TMC26X:
      m_tmc26x->setCurrent(val);
      break;
    case TMC2130:
      m_tmc2130->rms_current(val / sqrt(2),0.25);
      break;
    case TMC5160:
      m_tmc5160->rms_current(val / sqrt(2),0.25);
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

  void initMotor(Motor_Driver useddriver, int StepRot, int EnPin,  int CSPin, int DirPin, int StepPin, unsigned int Curr, int Micros)
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
      m_tmc2130 = new TMC2130Stepper(CSPin,0.11-0.02);
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
      m_tmc2130->tbl(1);
      m_tmc2130->TPOWERDOWN(255);
      m_tmc2130->toff(4);

      // Effective hysteresis = 0
      m_tmc2130->hstrt(0);
      m_tmc2130->hend(2);

      m_tmc2130->en_pwm_mode(true);
      m_tmc2130->pwm_freq(1);
      m_tmc2130->pwm_autoscale(true);
      m_tmc2130->pwm_ampl(180);
      m_tmc2130->pwm_grad(1);

      setCurrent(Curr); // mA
      setMicrostep(Micros);
      if (EnPin > 0)
        digitalWrite(EnPin, LOW);
      break;
    case TMC5160:
      m_tmc5160 = new TMC5160Stepper(CSPin, 0.075-0.02);
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
      m_tmc5160->push();
      m_tmc5160->tbl(2);
      m_tmc5160->toff(3);
      m_tmc5160->hstrt(4);
      m_tmc5160->hend(1);   
      m_tmc5160->en_pwm_mode(true);
      m_tmc5160->TPWMTHRS(1000);
      setCurrent(Curr);
      setMicrostep(Micros);
      if (EnPin > 0)
        digitalWrite(EnPin, LOW);
      break;
    case NODRIVER:
      break;
    default:
      break;
    }
  };
};