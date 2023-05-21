# Testing TeenAstro 

We now have 3 Python programs to test TeenAstro:

- MountSim is a mount simulator that displays any of the 4 models of mounts supported by TeenAstro, and moves according to the axis positions reported
- autoTest is a test program that compares the axis positions against values computed with the [skyfield](https://rhodesmill.org/skyfield/) astronomy library
- debug5160.cpp is a small monitor that allows initializing and sending commands to a TMC5160 motor driver.

The previous test program that used Mel Bartel's Scope to Sky calculator is no longer supported. 



