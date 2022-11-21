# Testing TeenAstro with AutoTest 



autoTest is a test program that compares the axis positions against values computed with the [skyfield](https://rhodesmill.org/skyfield/) astronomy library. It is only implemented for equatorial mounts at this time.

It has 3 test options:

- Pointing Accuracy Test

The mount points to a list of positions defined as alt-az pairs. At each position, the program compares the RA / Dec reported by TeenAstro against values computed from the axis positions using Skyfield. The differences are printed out in a text file.

- Drift Test 

The mount is set to track any point in the sky. At regular intervals (typically 2 to 10 seconds), the program reads the axis positions and computes the differences in RA and Dec since the test started. This can be used to verify slow movements like guiding and custom tracking

- Align Test
The current TeenAstro position is plotted on top of a simple star map. A set of controls sets "home position" and "pole alignment" errors. This allows running the alignment routine in the hand controller, to view its effect on pointing accuracy.

More documentation is available at [this address](https://fdesvallees.github.io/teenastro_linux/auto_test/)