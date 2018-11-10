/*
 * Generic Stepper Motor Driver Driver
 * Indexer mode only.
 *
 * Copyright (C)2015 Laurentiu Badea
 *
 * This file may be redistributed under the terms of the MIT license.
 * A copy of this license has been included with this distribution in the file LICENSE.
 */
#ifndef STEPPER_DRIVER_BASE_H
#define STEPPER_DRIVER_BASE_H
#include <Arduino.h>
#include <TMC2130Stepper.h>

 // used internally by the library to mark unconnected pins
#define PIN_UNCONNECTED -1
#define IS_CONNECTED(pin) (pin != PIN_UNCONNECTED)

inline void microWaitUntil(unsigned long target_micros) {
  yield();
  while (micros() < target_micros);
}
#define DELAY_MICROS(us) microWaitUntil(micros() + us)

/*
 * Basic Stepper Driver class.
 * Microstepping level should be externally controlled or hardwired.
 */
class BasicStepperDriver {
protected:
  TMC2130Stepper *driver;
  bool reverse;
  int dir_pin;
  int step_pin;
  int cs_pin;
  int enable_pin = PIN_UNCONNECTED;

  // step pulse duration (microseconds), depends on rp10m and microstep level
  unsigned long step_pulse;

  void setDirection(int direction);
  void init(void);


  // tWH(STEP) pulse duration, STEP high, min value (us)
  static const int step_high_min = 1;
  // tWL(STEP) pulse duration, STEP low, min value (us)
  static const int step_low_min = 1;
  // tWAKE wakeup time, nSLEEP inactive to STEP (us)
  static const int wakeup_time = 0;


public:
  void setReverse(bool rev);
  void setSpeed(unsigned int step_speed);
  unsigned int getSpeed();
  /*
   * Basic connection: DIR, STEP are connected.
   */
  BasicStepperDriver(int enable_pin, int dir_pin, int step_pin, int cs_pin);
  /*
   * Move the motor a given number of steps.
   * positive to move forward, negative to reverse
   */
  void move(long steps);
  /*
   * Turn off/on motor to allow the motor to be moved by hand/hold the position in place
   */
  void enable(void);
  void disable(void);
};
#endif // STEPPER_DRIVER_BASE_H
