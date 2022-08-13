# Testing TeenAstro with AutoTest 



autoTest is a test program that compares the axis positions against values computed with the [skyfield](https://rhodesmill.org/skyfield/) astronomy library. It is only implemented for equatorial mounts at this time.

It has 2 test options:

- Pointing Accuracy Test (not yet completed)

The mount points to a list of positions defined as alt-az pairs. At each position, the program compares the RA / Dec reported by TeenAstro against values computed from the axis positions using Skyfield. The differences are printed out in a text file.

 

- Drift Test 

The mount is set to track any point in the sky. At regular intervals (typically 2 to 10 seconds), the program reads the axis positions and computes the differences in RA and Dec since the test started. This can be used to verify slow movements like guiding and custom tracking
