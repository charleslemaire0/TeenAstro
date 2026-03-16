# MainUnit firmware

Core mount firmware on Teensy 3/4. Real-time control loop: sidereal clock, step ISRs, tracking, goto, alignment, EEPROM.

**Source:** `TeenAstroMainUnit/`

---

## Application class

**setup()** — Startup: blink, EEPROM/mount init, time sync, reticule, mount pins/sidereal, park/home/guide rates, GNSS probe, focuser probe, command serial.

**loop()** — Each cycle: `loopCommandsAndStatus` (process LX200 commands), `loopSt4AndGuiding`, `loopEncoderSync`, `loopSiderealAndSafety` (sidereal tick, tracking, safety).

---

## Axis classes

**StatusAxis** — enable, fault, acc, pos, start, target, deltaTarget, deltaStart, dir, fstep, interval_Step_Sid, takeupRate, takeupInterval, interval_Step_Cur, CurrentTrackingRate, RequestedTrackingRate, ClockSpeed, backlash state. Methods: `updateDeltaTarget()`, `atTarget()`, `resetToSidereal()`, `setSidereal()`, `breakDist()`, `setIntervalfromDist()`, `setIntervalfromRate()`, `move()`.

**Formulas:**  
- speedfromDist(d) = sqrt(2·acc·|d|) (signed)  
- ratefromSpeed(V) = V·interval_Step_Sid/ClockSpeed  
- interval2speed(interval) = ClockSpeed/interval  
- breakDist = interval2speed(interval)²/(2·acc)

**GeoAxis** — poleDef, homeDef, stepsPerRot, stepsPerDegree, stepsPerArcSecond, minAxis, maxAxis, limits.

**GuideAxis** — duration, absRate, speedMultiplier; methods for pulse guide (FW/BW, brake, etc.).

**MotorAxis** — gear, stepRot, micro, reverse, silent, highCurr, lowCurr, backlashAmount, backlashRate, driver.

---

## Sidereal clock & timers

- **masterClockSpeed** = 1,000,000; **siderealClockSpeed** ≈ 997269.5625.
- **Timer1:** sidereal tick; **TIMER3/TIMER4:** axis step ISRs (half-step toggle, update delta, move).
- Main loop calls `onSiderealTick(phase, forceTracking, elapsed)` when elapsed > 0; tracking: target += fstep·elapsed per axis.

---

## Tracking & goto

- **Tracking:** interval_Step_Cur = interval_Step_Sid/rate (clamped). Backlash uses backlash_interval_Step.
- **Goto:** start=pos, target=axisTarget; `moveTo()` uses setIntervalfromDist(d, tracking, min, max); on arrival: settling, resetToSidereal.
- **Acceleration:** acc = Vmax²/(4·degreesForAccel·stepsPerDegree).

---

## Command dispatcher

**T_Serial:** frame `:cmd#` or ACK `\x06`. `update()` builds command; `getCmdPar()` returns it; `reply()` sends.  
**processCommands():** switch on command[0] → Command_A/B/C/D/E/F/G/g/h/M/Q/R/S/T/U/W; pad reply to expected length; reply on all ports.

---

## EEPROM

**Layout:** EE_autoInitKey, EE_currentMount, EE_sites (27×3), EE_Mounts (200 bytes per mount). Per mount: mountType, positions, TC, park, site, refraction, rates, limits, drift, meridian, home, motors, encoders, alignment T (EE_T11…EE_T33, EE_Tvalid). **Autoinit:** if key ≠ 152682, clear EEPROM and write defaults.

---

## Encoder (EncoderAxis)

**Members:** pulsePerDegree, reverse, has_Ref, deg_T_Ref, pulse_E_Ref.  
**Methods:** `readencoder()`, `r_deg()`, `w_deg()`, `setRef()`, `calibrate()`.  
**autoSyncWithEncoder:** tolerances ES_60=1°, ES_30=0.5°, ES_15=0.25°, etc.; sync stepper to encoder when not slewing.

---

**See also:** [Tracking (detailed)](tracking.md) · [Alignment](../math/alignment.md) · [SHC](shc.md) · [Protocol](../protocol.md)
