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
BasicStepperDriver::BasicStepperDriver(int enable_pin, int dir_pin, int step_pin, int cs_pin)
  : enable_pin(enable_pin),  dir_pin(dir_pin), step_pin(step_pin), cs_pin(cs_pin)
{
  init();
}

void BasicStepperDriver::init(void) {

  driver = new TMC2130Stepper(enable_pin, dir_pin, step_pin, cs_pin);
  SPI.begin();
  pinMode(MISO, INPUT_PULLUP);
  driver->begin(); 			// Initiate pins and registeries
  driver->rms_current(600); 	// Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  driver->stealthChop(1); 	// Enable extremely quiet stepping

  pinMode(dir_pin, OUTPUT);
  digitalWrite(dir_pin, HIGH);

  pinMode(step_pin, OUTPUT);
  digitalWrite(step_pin, LOW);

  if IS_CONNECTED(enable_pin) {
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, HIGH); // disable
  }
  enable();
}

void BasicStepperDriver::setReverse(bool rev) {
  reverse = rev;
}

void BasicStepperDriver::setSpeed(unsigned int setStepSpeed) {
  step_pulse = 10000L / setStepSpeed;
}

unsigned int BasicStepperDriver::getSpeed() {
  return 10000L / step_pulse;
}


/*
 * DIR: forward HIGH, reverse LOW
 */
void BasicStepperDriver::setDirection(int direction) {
  direction = reverse ? -direction : direction;
  digitalWrite(dir_pin, (direction < 0) ? LOW : HIGH);
}

/*
 * Move the motor a given number of steps.
 * positive to move forward, negative to reverse
 */
void BasicStepperDriver::move(long steps) {
  if (steps >= 0) {
    setDirection(1);
  }
  else {
    setDirection(-1);
    steps = -steps;
  }
  /*
   * We currently try to do a 50% duty cycle so it's easy to see.
   * Other option is step_high_min, pulse_duration-step_high_min.
   */
  unsigned long pulse_duration = step_pulse / 2;
  while (steps--) {
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
void BasicStepperDriver::enable(void) {
  if IS_CONNECTED(enable_pin) {
    digitalWrite(enable_pin, LOW);
  }
}

void BasicStepperDriver::disable(void) {
  if IS_CONNECTED(enable_pin) {
    digitalWrite(enable_pin, HIGH);
  }
}

