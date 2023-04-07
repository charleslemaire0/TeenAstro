import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from teenastro import TeenAstro, deg2dms
import numpy as np  
import sys

numSamples = 100

def altazAxesToAltAz(axis1, axis2):
    az = 180 + axis1
    alt = axis2
    return az, alt


class driftPlot():
    def __init__(self, window, ts, ta):
        self.window = window
        self.ts = ts
        self.ta = ta
        if sys.platform == "darwin":
            self.dpi = 36
        elif sys.platform == "linux":
            self.dpi = 64
        else:
            self.dpi = 64
        self.x = np.linspace(0, numSamples-1, numSamples)
        self.ra = np.zeros(numSamples)
        self.dec = np.zeros(numSamples)
        self.speed1 = np.zeros(numSamples)
        self.speed2 = np.zeros(numSamples)
        self.fig, self.axes = plt.subplots(4, sharex=True, sharey=True, figsize=(10,6), dpi=self.dpi)
        self.yScale = 10
        self.axes[0].set_ylim(-self.yScale,self.yScale)
        self.axes[0].set_xlabel('time (seconds)')
        self.axes[0].set_ylabel('RA (arc-sec)')
        self.axes[1].set_ylabel('Dec (arc-sec)')
        self.axes[2].set_ylabel('Axis1 Speed')
        self.axes[3].set_ylabel('Axis2 Speed')
        self.line1, = self.axes[0].plot(self.x,self.ra,'green')
        self.line2, = self.axes[1].plot(self.x,self.dec,'red')
        self.line3, = self.axes[2].plot(self.x,self.speed1,'blue')
        self.line4, = self.axes[3].plot(self.x,self.speed2,'black')
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=window['drift_cv'].TKCanvas)
        self.planets = load('de421.bsp')
        self.state='STOP'

    def handleEvent(self, ev, v, w):
        if (ev == 'startStopTrack'):
            if not self.ta.isConnected():
                self.log('Not connected')
                return

            if self.state == 'IDLE':
                self.ta.enableTracking()
                self.log ('tracking enabled')
                self.reset()
                self.window['startStopTrack'].update('Stop Tracking')
                self.state = 'RECORD'
            else:
                self.ta.disableTracking()
                self.log ('tracking disabled')
                self.window['startStopTrack'].update('Start Tracking')
                self.state = 'IDLE'

        if (ev == 'clearDrift'):
            self.ra = np.zeros(numSamples)
            self.dec =  np.zeros(numSamples)
            self.reset()
            self.render()

        if (ev == 'saveDrift'):
            self.saveCsv()

        if (ev == 'GuideN'):
            self.ta.guideCmd('n',100)
            self.render()

        if (ev == 'GuideS'):
            self.ta.guideCmd('s',100)
            self.render()

        if (ev == 'GuideE'):
            self.ta.guideCmd('e',100)
            self.render()

        if (ev == 'GuideW'):
            self.ta.guideCmd('w',100)
            self.render()

        if (ev == 'zoomInD'):
            self.yScale = self.yScale / 2
            self.axes[0].set_ylim(-self.yScale,self.yScale)
            self.render()

        if (ev == 'zoomOutD'):
            self.yScale = self.yScale * 2
            self.axes[0].set_ylim(-self.yScale,self.yScale)
            self.render()

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return
            self.run()


    def saveCsv(self):
        self.log ('Saving to driftTest.csv')
        data = np.stack((self.ra, self.dec), axis=1)
        np.savetxt("driftTest.csv", data, delimiter=",", fmt="%3.8f",
                       header="axis1Drift, axis2Drift", comments="")

    def log(self, message):
        print (message)

    def render(self):
        self.figure_canvas_agg.draw()
        self.figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)
        try:
            ra_rate = 3600 * (self.ra[-1] - self.ra[-10]) / 10
        except:
            ra_rate = 0
        self.window['ra_rate'].Update('{0:2.2f}'.format(ra_rate)) 
        try:
            dec_rate = 3600 * (self.dec[-1] - self.dec[-10]) / 10
        except:
            dec_rate = 0
        self.window['dec_rate'].Update('{0:2.2f}'.format(dec_rate)) 

    def update(self, ra, dec, speed1, speed2):
        self.ra = np.append(self.ra, ra)
        self.line1.set_ydata(3600*self.ra[-100:])   # arc-seconds
        self.dec = np.append(self.dec, dec)
        self.line2.set_ydata(3600*self.dec[-100:])
        self.speed1 = np.append(self.speed1, speed1)
        self.line3.set_ydata(self.speed1[-100:])
        self.speed2 = np.append(self.speed2, speed2)
        self.line4.set_ydata(self.speed2[-100:])

    def reset(self):
        t1 = self.ts.now()
        lst = t1.gmst + self.lon / 15       # in hours   
        if (self.mountType in ['E','K']):
            pierSide = self.ta.getPierSide()
            axis1 = self.ta.getAxis1()
            axis2 = self.ta.getAxis2()
            self.initialRA = 15.0 * lst - axis1  # in degrees
            self.initialDec = axis2
        elif (self.mountType in ['A','k']):     
            axis1 = self.ta.getAxis1()
            axis2 = self.ta.getAxis2()
            az, alt = altazAxesToAltAz(axis1, axis2)
            t = self.ts.now()                    # skyfield datetime from computer 
            direction = self.site.at(t).from_altaz(az_degrees=az, alt_degrees=alt)
            ra_, dec_, distance_ = direction.radec()
            self.initialRA = ra_._degrees    # transform from skyfield units to float
            self.initialDec = dec_.degrees

    def start(self):
        if not self.ta.isConnected():
            self.log('Not connected')
            return
        self.mountType = self.ta.readMountType()
        if (self.mountType == 'E'):
            self.log  ('German Equatorial')
        elif (self.mountType == 'K'):
            self.log  ('Equatorial Fork')
        elif (self.mountType == 'A'):
            self.log  ('Alt Az Tee')
        elif (self.mountType == 'k'):
            self.log  ('Alt Az Fork')
        self.log ('start Drift Test')
        self.testStep = 0

        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)

        self.reset()
        self.state = 'IDLE'

    def run(self):
        if not self.ta.isConnected():
            return
        if self.state == 'STOP':
            self.start()
            return
        t1 = self.ts.now()
        lst = 15.0 * t1.gmst + self.lon         # in degrees   
        self.sp1 = self.ta.getAxis1Speed()
        self.sp2 = self.ta.getAxis2Speed()

        if (self.mountType in ['E','K']):
            pierSide = self.ta.getPierSide()
            axis1 = self.ta.getAxis1()          # get axis positions and compute RA/Dec
            axis2 = self.ta.getAxis2()
            ra = lst - axis1  # in degrees
            dec = axis2
        elif (self.mountType in ['A','k']):     
            axis1 = self.ta.getAxis1()
            axis2 = self.ta.getAxis2()
            az, alt = altazAxesToAltAz(axis1, axis2)
            t = self.ts.now()
            direction = self.site.at(t).from_altaz(az_degrees=az, alt_degrees=alt)
            ra_, dec_, distance_ = direction.radec()
            ra = ra_._degrees
            dec = dec_.degrees
        self.update(ra-self.initialRA, dec-self.initialDec, self.sp1, self.sp2)
        self.render()
        self.window['axis1_degrees'].Update('{0:4.2f}'.format(axis1)) 
        self.window['axis2_degrees'].Update('{0:4.2f}'.format(axis2)) 
        self.window['axis1_steps'].Update(self.ta.getAxis1Steps()) 
        self.window['axis2_steps'].Update(self.ta.getAxis2Steps()) 
        self.window['axis1_speed'].Update('{0:4.2f}'.format(self.sp1)) 
        self.window['axis2_speed'].Update('{0:4.2f}'.format(self.sp2)) 

