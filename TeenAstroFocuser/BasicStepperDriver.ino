/*
 * Generic Stepper Motor Driver Driver
 * Indexer mode only.

 * Copyright (C)2015 Laurentiu Badea
 *
 * This file may be redistributed under the terms of the MIT license.
 * A copy of this license has been included with this distribution in the file LICENSE.
 */
#include "BasicStepperDriver.h"

/*
 * Basic connection: only DIR, STEP are connected.
 * Microstepping controls should be hardwired.
 */
BasicStepperDriver::BasicStepperDriver(int dir_pin, int step_pin)
: dir_pin(dir_pin), step_pin(step_pin)
{
    init();
}

BasicStepperDriver::BasicStepperDriver(int dir_pin, int step_pin, int enable_pin)
: dir_pin(dir_pin), step_pin(step_pin), enable_pin(enable_pin)
{
    init();
}

void BasicStepperDriver::init(void){
    pinMode(dir_pin, OUTPUT);
    digitalWrite(dir_pin, HIGH);

    pinMode(step_pin, OUTPUT);
    digitalWrite(step_pin, LOW);

    if IS_CONNECTED(enable_pin){
        pinMode(enable_pin, OUTPUT);
        digitalWrite(enable_pin, HIGH); // disable
    }
    enable();
}

void BasicStepperDriver::setReverse(bool rev) {
     reverse = rev;
}

void BasicStepperDriver::setSpeed(unsigned int setStepSpeed){
    step_pulse = 100000L / setStepSpeed;
}

/*
 * DIR: forward HIGH, reverse LOW
 */
void BasicStepperDriver::setDirection(int direction){
  direction = reverse ? -direction : direction;
  digitalWrite(dir_pin, (direction<0) ? LOW : HIGH);
}

/*
 * Move the motor a given number of steps.
 * positive to move forward, negative to reverse
 */
void BasicStepperDriver::move(long steps){
    if (steps >= 0){
        setDirection(1);
    } else {
        setDirection(-1);
        steps = -steps;
    }
    /*
     * We currently try to do a 50% duty cycle so it's easy to see.
     * Other option is step_high_min, pulse_duration-step_high_min.
     */
    unsigned long pulse_duration = step_pulse/2;
    while (steps--){
        digitalWrite(step_pin, HIGH);
        unsigned long next_edge = micros() + pulse_duration;
        microWaitUntil(next_edge);
        digitalWrite(step_pin, LOW);
        microWaitUntil(next_edge + pulse_duration);
    }
}


/*
 * Enable/Disable the motor by setting a digital flag
 */
void BasicStepperDriver::enable(void){
    if IS_CONNECTED(enable_pin){
        digitalWrite(enable_pin, LOW);
    }
}

void BasicStepperDriver::disable(void){
    if IS_CONNECTED(enable_pin){
        digitalWrite(enable_pin, HIGH);
    }
}

