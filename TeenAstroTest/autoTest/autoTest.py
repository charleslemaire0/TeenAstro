import math, time, sys, argparse, csv
import numpy as np  
from skyfield.api import wgs84, load, position_of_radec
from skyfield.positionlib import Apparent, Barycentric, Astrometric, Distance
from skyfield.earthlib import refraction
from skyfield.api import utc
import glooey
import pyglet
from pyglet import clock

sys.path.insert(0, '../mountSim')
from teenastro import TeenAstro, deg2dms

# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-p', '--ip', help='TeenAstro IP address')
  opts = parser.parse_args(args)
  if opts.ip == None:
    opts.ip = '192.168.0.21'
  return opts


'''
testCase =  [
                {'az':180,'alt':20}, {'az':90,'alt':20},{'az':0,'alt':20},{'az':270,'alt':20},
                {'az':180,'alt':40}, {'az':90,'alt':40},{'az':0,'alt':40},{'az':270,'alt':40},
                {'az':180,'alt':40}, {'az':90,'alt':40},{'az':0,'alt':40},{'az':270,'alt':40},
                {'az':180,'alt':40}, {'az':90,'alt':40},{'az':0,'alt':40},{'az':270,'alt':40},
            ]
'''
testCase =  [
                {'az':0,'alt':40}, {'az':20,'alt':40},{'az':40,'alt':40},{'az':60,'alt':40},{'az':80,'alt':40},
                {'az':100,'alt':40}, {'az':120,'alt':40},{'az':140,'alt':40},{'az':160,'alt':40},{'az':180,'alt':40},
                {'az':200,'alt':40}, {'az':220,'alt':40},{'az':240,'alt':40},{'az':260,'alt':40},{'az':280,'alt':40},
                {'az':300,'alt':40}, {'az':320,'alt':40},{'az':340,'alt':40}
            ]


# Main program
class Application:

    def __init__(self, options):

        self.width, self.height = 200, 200
        window = self._create_window(width=self.width, height=self.height)

        gui = glooey.Gui(window)
        hbox = glooey.HBox()

        try:
            self.ta = TeenAstro('tcp', options.ip)
            p = self.ta.open()
        except:
            self.log ("Error connecting to TeenAstro")
            sys.exit(1)

        self.ts = load.timescale()
        self.planets = load('de421.bsp')
        earth = self.planets['earth']
        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = earth + wgs84.latlon(self.lat, self.lon)

        vbox = glooey.VBox()
        hbox.add(vbox)
        button = glooey.Button("Pointing Accuracy Test")
        button.push_handlers(on_click=self.startPointingTest)
        vbox.add(button)

        button = glooey.Button("Drift Test")
        button.push_handlers(on_click=self.startDriftTest)
        vbox.add(button)

        gui.add(hbox)
        pyglet.clock.schedule_interval(self.update, 1. / 5) 
        self.busy = False
        pyglet.app.run()

    def convertAxis1(self, axis1, pierSide):
        if (pierSide == 'E'):
            return axis1
        else:
            return axis1+180

    def convertAxis2(self, axis2, pierSide):
        if (pierSide == 'E'):
            return axis2 
        else:
            return 180-axis2


    def startPointingTest(self,arg):
        if self.busy:
            self.log ('Test already running')
            return
        self.axis1 = self.axis2 = 0
        self.busy = True
        self.testStep = 0
        self.log ('az, alt, deltaRA, deltaDec')
        res = self.ta.gotoAzAlt(testCase[self.testStep]['az'], testCase[self.testStep]['alt'])
        pyglet.clock.schedule_interval(self.runPointingTest, 0.5) 

    def runPointingTest(self, context):
        if self.ta.isSlewing():
            return

        t1 = self.ta.readDateTime()         # python datetime from TeenAstro
        t2 = self.ts.from_datetime(t1)      # converted to skyfield format
        lst = t2.gmst + self.lon / 15   

#        print (refraction(0.0, temperature_C=15.0, pressure_mbar=1030.0))
        axis1 = self.ta.getAxis1() 
        axis2 = self.ta.getAxis2()
        alt = self.ta.getAltitude()
        az = self.ta.getAzimuth()
        pierSide = self.ta.getPierSide()

        apparent = self.site.at(t2).from_altaz(alt_degrees=alt, az_degrees=az, distance=Distance(au=100))
        ra,dec, dist = apparent.radec()

        deltaRA = (15 * (lst - ra.hours) - self.convertAxis1(axis1, pierSide)) % 360
        if (deltaRA > 180):
            deltaRA -= 360
        deltaDec = (dec.degrees - self.convertAxis2(axis2, pierSide)) % 90
        if (deltaDec > 45):
            deltaDec -= 90

        self.log ('{0},{1},{2},{3}'.format(testCase[self.testStep]['az'],testCase[self.testStep]['alt'],deltaRA, deltaDec)) 
        self.testStep += 1
        if self.testStep == len(testCase):
            self.ta.goHome()
            pyglet.clock.unschedule(self.runPointingTest) 
            self.busy = False
            return
        res = self.ta.gotoAzAlt(testCase[self.testStep]['az'], testCase[self.testStep]['alt'])

    def startDriftTest(self, context):
        if (self.busy):
            self.log ('Test already running')
            return
        self.log ('axis1Drift, axis2Drift')
        self.testStep = 0
        self.busy = True

        t1 = self.ts.now()
        lst = t1.gmst + self.lon / 15       # in hours   

        axis1 = self.ta.getAxis1() 
        axis2 = self.ta.getAxis2()
        pierSide = self.ta.getPierSide()
        self.initialRA = 15 * lst - self.convertAxis1(axis1, pierSide)  # in degrees
        self.initialDec = self.convertAxis1(axis2, pierSide)
        pyglet.clock.schedule_interval(self.runDriftTest, 2) 

    def runDriftTest(self, dt):
        if self.testStep == 100:
            self.busy = False
            pyglet.clock.unschedule(self.runDriftTest) 
            return

        t1 = self.ts.now()
        lst = t1.gmst + self.lon / 15       # in hours   

        axis1 = self.ta.getAxis1() 
        axis2 = self.ta.getAxis2()
        pierSide = self.ta.getPierSide()
        ra = 15 * lst - self.convertAxis1(axis1, pierSide)  # in degrees
        dec = self.convertAxis1(axis2, pierSide)

        self.log ('{0},{1}'.format(ra-self.initialRA, dec-self.initialDec)) 


    # May override this with a graphical window in the future
    def log(self, message):
        print (message)

    def update(self, dt):
        pass

    def _create_window(self, width, height):
        try:
            config = pyglet.gl.Config(sample_buffers=1,
                                      samples=4,
                                      depth_size=24,
                                      double_buffer=True)
            window = pyglet.window.Window(config=config,
                                          width=width,
                                          height=height)
        except pyglet.window.NoSuchConfigException:
            config = pyglet.gl.Config(double_buffer=True)
            window = pyglet.window.Window(config=config,
                                          width=width,
                                          height=height)

        @window.event
        def on_key_press(symbol, modifiers):
            if modifiers == 0:
                if symbol == pyglet.window.key.Q:
                    window.close()
        return window



if __name__ == '__main__':
    options = readOptions()
    Application(options)
