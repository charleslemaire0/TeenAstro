TeenAstroNt
======  
### Introduction

This is the first release of the TeenAstro redesign documented [here](https://fdesvallees.github.io/teenastro_v3/teenastro_v3/)

It supports both step/dir and position/velocity interface without requiring a rebuild. It should even be possible to run each motor with a different interface.

It already has: 

- Basic command handling through 2 serial ports 
- Mount movements in 4 directions
- Goto
- Tracking (not fully working for AltAz mounts)
- Guiding is there but not tested
- Parking, custom Home position are not implemented
- HAL is partly done but there are still some #ifdefs in the code that need to be removed
- EEPROM support: small change to support the RAM/Flash implementation of ESP32, that also requires a task to commit the changes to flash when needed. 
- RealTime clock: my boards do not have a clock, so I report a fake time.



What is does **not** have:

- Alignment
- Backlash
- GPS, Focuser
- Encoder support

The MainUnit code was forked from the 1.3 release, so it needs work to upgrade to the 1.4 code, including new :GXI command fields and EEPROM addresses.

### Hardware support

The software is tested on an ESP32 development board, and on a Teensy4. It is built with PlatformIO, either with the command line tools or through VSCode. On the ESP32, it is possible to use JTAG with its excellent breakpoint and watch capabilities. The low-level accesses to the motor drivers through SPI (TMC5160) have been only briefly tested.

### Design choices

#### FreeRTOS

The most important change is the introduction of FreeRTOS. This allows the creation of separate tasks. There are much fewer global variables. Instead, we use the following features of the OS:

- Queues for exchanging messages between tasks
- Semaphores for protecting variables and hardware accesses (including SPI)

Only the MainUnit code is changed. SHC, Focuser are not touched. TeenAstroStepper is replaced by the new MotorDriver class.

All .ino files are replaced by .cpp files with corresponding headers. Timer.ino is gone. The StatusAxis class is no longer needed, but GeoAxis is still there.

#### MotorDriver 

The single MotorDriver class encapsulates:

- the native TMCStepper class which works very well
- a Mc5160 class that exposes a simplified version of the TMC5160 Motion Controller. We use only the positioning interface, with the ability to set max. speed and acceleration. These register access functions are directly provided by TMCStepper.
- a StepDir class that provides the same API as the 5160. This requires one FreeRTOS task for each motor.

#### Separate models for Eq and AltAz mounts

I chose this for the reasons indicated [here](https://fdesvallees.github.io/teenastro_v3/teenastro_v3/#alignment-equatorial-vs-altaz-mounts). I think it is better to keep the alignment matrix only for alignment and mount errors, plus the sky model (refractions). We need more testing and discussion on both models to decide which works best.



### Test and Configuration

I updated TAConfig.py for updating the mount parameters. Testing is based on mountSim.py (useful for visualizing) and autoTest.py (more thorough testing). Both test programs rely on the axis coordinates and steps reported by TeenAstro. See the respective README for more info on both programs.

I did a little testing with EKOS through the INDI driver. Basic Goto and tracking work, but it needs much more testing. No ASCOM testing whatsoever, also I have not yet tried to connect the SHC.



