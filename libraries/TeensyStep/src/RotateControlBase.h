#pragma once

#include "MotorControlBase.h"
#include <algorithm>
#include "core_pins.h"

template <typename Accelerator, typename TimerField>
class RotateControlBase : public MotorControlBase<TimerField>
{
  public:
    RotateControlBase(unsigned pulseWidth = 5, unsigned accUpdatePeriod = 5000);    

    // Non-blocking movements ----------------
    template <typename... Steppers>
    void rotateAsync(Steppers &... steppers);
   
    template <size_t N>
    void rotateAsync(Stepper *(&motors)[N]);

    void stopAsync();

     void emergencyStop() {
         accelerator.eStop();
         this->timerField.stepTimerStop();
     }

     // Blocking movements --------------------
     void stop();

     void overrideSpeed(float speedFac);
     void overrideAcceleration(float accFac);

 protected:
     void doRotate(int N, float speedFactor = 1.0);
     void accTimerISR();

     Accelerator accelerator;

     RotateControlBase(const RotateControlBase &) = delete;
     RotateControlBase &operator=(const RotateControlBase &) = delete;
};

// Implementation *************************************************************************************************

template <typename a, typename t>
RotateControlBase<a, t>::RotateControlBase(unsigned pulseWidth, unsigned accUpdatePeriod)
    : MotorControlBase<t>(pulseWidth, accUpdatePeriod)
{
    this->mode = MotorControlBase<t>::Mode::notarget;
}

template <typename a, typename t>
void RotateControlBase<a, t>::doRotate(int N, float speedFactor)
{
     //Calculate Bresenham parameters ----------------------------------------------------------------
    std::sort(this->motorList, this->motorList + N, Stepper::cmpVmax);
    this->leadMotor = this->motorList[0];

    if (this->leadMotor->vMax == 0)
        return;

    this->leadMotor->currentSpeed = 0; 

    this->leadMotor->A = std::abs(this->leadMotor->vMax);
    for (int i = 1; i < N; i++)
    {
        this->motorList[i]->A = std::abs(this->motorList[i]->vMax);
        this->motorList[i]->B = 2 * this->motorList[i]->A - this->leadMotor->A;
    }
    uint32_t acceleration = (*std::min_element(this->motorList, this->motorList + N, Stepper::cmpAcc))->a; // use the lowest acceleration for the move
    
    // Start moving---------------------------------------------------------------------------------------  
    accelerator.prepareRotation(this->leadMotor->current, this->leadMotor->vMax, acceleration, this->accUpdatePeriod, speedFactor);
    this->timerField.setStepFrequency(0);    
    this->timerField.accTimerStart();    
}

// ISR -----------------------------------------------------------------------------------------------------------

template <typename a, typename t>
void RotateControlBase<a, t>::accTimerISR()
{   
    int32_t newSpeed = accelerator.updateSpeed(this->leadMotor->current); // get new speed for the leading motor
     
    //Serial.printf("rc,curSpeed: %i newspd:%i\n",this->leadMotor->currentSpeed,  newSpeed);

    if (this->leadMotor->currentSpeed == newSpeed)
    {         
        return; // nothing changed, just keep running
    }

    int dir = newSpeed >= 0 ? 1 : -1; // direction changed? -> toggle direction of all motors
    if (dir != this->leadMotor->dir)
    {
        Stepper **motor = this->motorList;
        while ((*motor) != nullptr)
        {
            (*motor++)->toggleDir();
        }
        delayMicroseconds(this->pulseWidth);
    }
    
    
    this->timerField.setStepFrequency(std::abs(newSpeed)); // speed changed, update timer    
    this->leadMotor->currentSpeed = newSpeed;   
}

// ROTATE Commands -------------------------------------------------------------------------------

template <typename a, typename t>
template <typename... Steppers>
void RotateControlBase<a, t>::rotateAsync(Steppers &... steppers)
{
    this->attachStepper(steppers...);
    doRotate(sizeof...(steppers));
}

template <typename a, typename t>
template <size_t N>
void RotateControlBase<a, t>::rotateAsync(Stepper *(&steppers)[N]) 
{
    this->attachStepper(steppers);
    doRotate(N);
}

template <typename a, typename t>
void RotateControlBase<a, t>::overrideSpeed(float factor)
{
    accelerator.overrideSpeed(factor);
}

template <typename a, typename t>
void RotateControlBase<a, t>::overrideAcceleration(float factor)
{
    accelerator.overrideAcceleration(factor);
}

template <typename a, typename t>
void RotateControlBase<a, t>::stopAsync()
{
    accelerator.initiateStopping(this->leadMotor->current);
}

template <typename a, typename t>
void RotateControlBase<a, t>::stop()
{
    stopAsync();
    while (this->isRunning())
    {
        delay(1);
    }
}