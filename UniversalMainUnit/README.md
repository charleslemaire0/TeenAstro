UniversalMainUnit
======  
### Introduction

This is an early release of the TeenAstro redesign documented [here](https://fdesvallees.github.io/teenastro_v3/teenastro_v3/)

It dynamically supports both step/dir and position/velocity interface. It is possible to run each motor with a different interface.

It already has: 

- Basic command handling through 2 serial ports 
- Mount movements in 4 directions
- Goto
- Tracking 
- Guiding 
- Parking, custom Home position 
- HAL is partly done but there are still some #ifdefs in the code that need to be removed
- EEPROM support: small change to support the RAM/Flash implementation of ESP32, that also requires a task to commit the changes to flash when needed. 
- RealTime clock: ESP32 does not have a clock, so it reports a fake time.



What is does **not** have:

- Alignment
- Backlash
- GPS, Focuser
- Encoder support


### Hardware support

The software is tested on an ESP32 development board, a Board 250 (Teensy4) and a modified board 240 (removed the ST4 5V pull-up resistors). It is built with PlatformIO, either with the command line tools or through VSCode. On the ESP32, it is possible to use JTAG with its excellent breakpoint and watch capabilities. 

### Design choices

#### FreeRTOS

The most important change is the introduction of FreeRTOS. This allows the creation of separate tasks. There are much fewer global variables. Instead, we use the following features of the OS:

- Queues for exchanging messages between tasks
- Semaphores for protecting variables and hardware accesses (including SPI)
- Event Groups that replace most of the global variables in the original code
- Software timers for various operations

Only the MainUnit code is changed. SHC, Focuser are not touched. TeenAstroStepper is replaced by the new MotorDriver class.

All .ino files are replaced by .cpp files with corresponding headers. Timer.ino is gone. The StatusAxis class is no longer needed, but GeoAxis is still there.

#### MotorDriver 

The single MotorDriver class encapsulates:

- the native TMCStepper class which works very well
- a Mc5160 class that exposes a simplified version of the TMC5160 Motion Controller. We use only the positioning interface, with the ability to set max. speed and acceleration. These register access functions are directly provided by TMCStepper.
- a StepDir class that provides the same API as the 5160. This requires one FreeRTOS task for each motor.

#### Separate models for Eq and AltAz mounts

I chose this for the reasons indicated [here](https://fdesvallees.github.io/teenastro_v3/teenastro_v3/#alignment-equatorial-vs-altaz-mounts). I think it is better to keep the alignment matrix only for alignment and mount errors, plus the sky model (refractions). We need more testing and discussion on both models to decide which works best.

#### New commands
A new command has been added (SXK,vvvv#) to indicate the clock speed of the TMC5160. The freqency (in kHz) is stored in the EEPROM. This allows using the TMC5160 in SPI mode with or without an external clock.


### Test and Configuration

**TAConfig.py**  updates the mount parameters. Testing is based on **debug5160** (stand-alone executable that runs commands from a terminal), **mountSim.py** (useful for visualizing) and **autoTest.py** (more thorough testing). Both test programs rely on the axis coordinates and steps reported by TeenAstro. See the respective README for more info on both programs.

### Test Status - 1 June 2023

#### Tested features
Basic Goto, tracking and guiding work, SHC connects and runs normally. Tests are done first in simulation (both Equatorial and AltAz), then with a real equatorial mount (AP600). No testing under the sky yet, no astrophoto. 
Pulse and ST4 guiding
Spiral

I did a little testing with EKOS through the INDI driver. 

### Not yet implemented or tested
No ASCOM testing whatsoever
Alignment is not working (possible reentrancy problems in CoordConv)

#### Known bugs
In some cases a goto never stops slewing but stays at very low speed. Difficult to reproduce.


