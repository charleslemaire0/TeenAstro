# Motor & stepper drivers

Stepper abstraction: **MotionControl** (StepDir or Mc5160), **MotorDriver**, **Driver** (TMC wrappers).

**Source:** `libraries/TeenAstroStepper/`, `libraries/MotorDriver/`

---

## Class hierarchy

- **MotionControl** (abstract) → **StepDir** (step/dir + timer ISR), **Mc5160** (TMC5160 SPI motion controller).
- **MotorDriver** — holds MotionControl* and TMC5160Stepper*; `initStepDir()` or `initMc5160()`.
- **Driver** — MOTORDRIVER: NODRIVER, STEPDIR, TMC26X, TMC2130, TMC5160, TMC2660; methods for current, microstep, silent mode.

---

## StepDir

Step and direction pins; ISR toggles step pin. `programSpeed(double V)` sets direction and timer period. Used with TMC5160 in step/dir mode (double-edge).

---

## Step rate math

- interval2speed(interval) = ClockSpeed / interval (steps/s).
- speed2interval(V, maxInterval) = min(ClockSpeed/V, maxInterval).
- **StepsMinInterval:** 3 µs (Teensy 4.x) or 12 µs; **StepsMaxInterval:** 100000 µs.

---

## PositionState (internal)

PS_IDLE, PS_ACCEL, PS_CRUISE, PS_DECEL, PS_DECEL_TARGET, PS_STOPPING.

---

**See also:** [MainUnit](mainunit.md)
