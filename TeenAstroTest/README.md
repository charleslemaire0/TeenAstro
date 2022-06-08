# Scope to Sky and TeenAstro

Mel Bartel’s [Scope to Sky calculator](https://www.bbastrodesigns.com/scopeToSky.html), written in Javascript with an HTML user interface, has several functions:  
- Given sky coordinates (either Right Ascension or Hour Angle, and Declination), compute a telescope’s axis positions.   
- Given a telescope’s axis positions, compute the corresponding sky coordinates.  
- For a telescope with encoders, display encoder values for a given telescope position or compute telescope position according to the encoder values.  

The configuration includes:  
- Setup site, time and date, time zone  
- equatorial and altazimuth mounts  
- Correction for precession, nutation and annual aberration  
- Correction for refraction  
- For equatorial mounts, set pier side (mount flip)  
- Conversion styles (trigonometry or matrix)  
- Tracking rates algorithms (method for computing the rates)  
- Encoder gears


## How to use it for testing TeenAstro Firmware
We generate a set of test cases (sky positions, site, time etc.), use ScopeToSky to compute axes positions for each, and use them as Goto targets for a TeenAstro. We read the stepper counts for both axes, normalize them back to degrees, and compare with the computed values.

See complete documentation at [http://astro.roya.org/teenastro_linux/scopetosky/](http://astro.roya.org/teenastro_linux/scopetosky/)

# Mount Simulator

mountSim is a Python program that connects to a TeenAstro through Wifi and displays a simulated mount. It reads the steps on both axes, and shows the mount's movements, including Meridian Flip (for German Equatorials only) and to debug eventual firmware problems. 


## Installation
Download the python scripts and STL files from Github. 
Install Python 3.8 or higher, and the following modules:

`pip install argparse numpy trimesh glooey pyglet threading serial serial.tools time datetime`

Launch mountSim from the command line. The single option is the IP address of your TeenAstro.

`python mountSim.py --ip 192.168.0.21`

A graphic window opens, that displays a simplified mount model, selected according to your type of mount:

You can now move the mount with the hand controller. It is also possible to control TeenAstro remotely, either with the Web interface from the SHC, or through a program running on your PC (Ekos, SkySafari etc.).   
Note that you can have only one Wifi (IP) port at a time, so the PC program should use USB, not Wifi.   
You can run mountSim in parallel with your mount, or without. In this case it is possible to speed up the maximum speed up to 2000x or more. (You may need to lower the gear reduction). 

Use these mouse movements: click-drag to change the view orientation, right-click+drag to pan around.

Full documentation is available at [this address](https://fdesvallees.github.io/teenastro_linux/mount_sim/)


