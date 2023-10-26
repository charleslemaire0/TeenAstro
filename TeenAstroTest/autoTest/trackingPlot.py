import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from teenastro import TeenAstro, deg2dms
import numpy as np  
import sys, random
from datetime import datetime

numSamples = 100


def sign(val):
    res = np.sign(val)
    if (res == 0):
        res = 1
    return res


def eqAxesToEqu(axis1, axis2, latitude, lst):
    hemisphere = sign(latitude)
    flipSign = sign(axis2)
    ha  = hemisphere * (axis1 + flipSign * 90) / 15;
    ra = lst - ha       # in hours
    dec = hemisphere * (90 - (flipSign * axis2));    
    return (ha, ra, dec)


def altazAxesToAltAz(axis1, axis2, latitude):
    if (latitude >= 0):
      az = 180 + axis1
      if (az > 360):
        az = az - 360
    else:
      az = axis1
    alt = axis2
    return az, alt


class driftPlot():
    def __init__(self, window, dpi, x, ra, dec, sp1, sp2):
        self.window = window
        self.fig, self.axes = plt.subplots(4, sharex=True, sharey=True, figsize=(10,6), dpi=dpi)
        self.yScale = 10
        self.axes[0].set_ylim(-self.yScale,self.yScale)
        self.axes[0].set_xlabel('time (seconds)')
        self.axes[0].set_ylabel('RA (arc-sec)')
        self.axes[1].set_ylabel('Dec (arc-sec)')
        self.axes[2].set_ylabel('Axis1 Speed')
        self.axes[3].set_ylabel('Axis2 Speed')
        self.line1, = self.axes[0].plot(x,ra,'green')
        self.line2, = self.axes[1].plot(x,dec,'red')
        self.line3, = self.axes[2].plot(x,sp1,'blue')
        self.line4, = self.axes[3].plot(x,sp2,'black')
        self.figure_canvas_t = FigureCanvasTkAgg(self.fig, master=window['tracking_cv_t'].TKCanvas)

    def zoomIn(self):
        self.yScale = self.yScale / 2
        self.axes[0].set_ylim(-self.yScale,self.yScale)
        self.render()

    def zoomOut(self):
        self.yScale = self.yScale * 2
        self.axes[0].set_ylim(-self.yScale,self.yScale)
        self.render()

    def render(self):
        self.figure_canvas_t.draw()
        self.figure_canvas_t.get_tk_widget().pack(side='right', fill='both', expand=1)

    def update(self, ra, dec, sp1, sp2):
        self.line1.set_ydata(ra[-100:])   # arc-seconds
        self.line2.set_ydata(dec[-100:])
        self.line3.set_ydata(sp1[-100:])
        self.line4.set_ydata(sp2[-100:])
        self.render()



class trackingPlotXY():
    def __init__(self, window, dpi, ra, dec):
        self.window = window
        self.scale = 10
        self.fig, self.ax = plt.subplots(figsize=(6,6), dpi=dpi)
        self.line, = self.ax.plot(ra, dec, 'blue')
        self.ax.set(xlim=(-self.scale, self.scale), ylim=(-self.scale, self.scale))
        self.ax.grid(True)
        self.ax.set_xlabel('RA (arc-sec)')
        self.ax.set_ylabel('Dec (arc-sec)')
        self.figure_canvas_xy = FigureCanvasTkAgg(self.fig, master=window['tracking_cv_xy'].TKCanvas)

    def update(self, ra, dec):
        self.line.set_data(ra, dec)
        self.render()

    def zoomIn(self):
        self.scale = self.scale / 2
        self.ax.set(xlim=(-self.scale, self.scale), ylim=(-self.scale, self.scale))
        self.render()

    def zoomOut(self):
        self.scale = self.scale * 2
        self.ax.set(xlim=(-self.scale, self.scale), ylim=(-self.scale, self.scale))
        self.render()

    def render(self):
        self.figure_canvas_xy.draw()
        self.figure_canvas_xy.get_tk_widget().pack(side='right', fill='both', expand=1)


class trackingPlot():
    def __init__(self, window, ts, ta, dpi):
        self.window = window
        self.ts = ts
        self.ta = ta
        self.planets = load('de421.bsp')

        self.x = np.linspace(0, numSamples-1, numSamples)
        self.clear()

        self.state='STOP'
        self.driftPlot = driftPlot(self.window, dpi, self.x, self.ra, self.dec, self.sp1, self.sp2)
        self.xyPlot = trackingPlotXY(self.window, dpi, self.ra, self.dec)

    def clear(self):
        self.t = np.zeros(numSamples)
        self.ra = np.zeros(numSamples)
        self.dec = np.zeros(numSamples)
        self.sp1 = np.zeros(numSamples)
        self.sp2 = np.zeros(numSamples)


    def connect(self, ta):
        self.ta = ta
        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
        self.mountType = self.ta.readMountType()
        self.reset()

    def run(self):
        if not self.ta.isConnected():
            return
        if self.state == 'STOP':
            self.start()
            return

        self.updateTracking()
        # get parameters 
        axis1 = self.ta.getAxis1()
        axis2 = self.ta.getAxis2()
        sp1 = self.ta.getAxis1Speed()
        sp2 = self.ta.getAxis2Speed()

        # compute values
        t = self.ts.now()
        lst = t.gmst + self.lon / 15.0        # in degrees   

        if (self.mountType in ['E','K']):
            pierSide = self.ta.getPierSide()
            ha, ra, dec = eqAxesToEqu(axis1, axis2, self.lat, lst)
        elif (self.mountType in ['A','k']):     
            az, alt = altazAxesToAltAz(axis1, axis2, self.lat)
            direction = self.site.at(t).from_altaz(az_degrees=az, alt_degrees=alt)
            ra_, dec_, distance_ = direction.radec()
            ra = ra_.hours
            dec = dec_.degrees

        self.t = np.append(self.t, datetime.now().timestamp())
        self.ra = np.append(self.ra, 3600*(15*ra-self.initialRA)) # convert ra to arc-seconds
        self.dec = np.append(self.dec, 3600*(dec-self.initialDec))
        self.sp1 = np.append(self.sp1, sp1)
        self.sp2 = np.append(self.sp2, sp2)

        # update text boxes
        self.window['axis1_degrees'].Update('{0:4.4f}'.format(axis1)) 
        self.window['axis2_degrees'].Update('{0:4.4f}'.format(axis2)) 
        self.window['axis1_steps'].Update(self.ta.getAxis1Steps()) 
        self.window['axis2_steps'].Update(self.ta.getAxis2Steps()) 
        self.window['axis1_speed'].Update('{0:4.4f}'.format(sp1)) 
        self.window['axis2_speed'].Update('{0:4.4f}'.format(sp2)) 

        try:
            ra_rate =  (self.ra[-1] - self.ra[-11]) / (self.t[-1] - self.t[-11]) 
        except:
            ra_rate = 0
        self.window['ra_rate'].Update('{0:2.2f}'.format(ra_rate)) 
        try:
            dec_rate = (self.dec[-1] - self.dec[-11]) / (self.t[-1] - self.t[-11])
        except:
            dec_rate = 0
        self.window['dec_rate'].Update('{0:2.2f}'.format(dec_rate)) 

        # update both plots
        self.driftPlot.update(self.ra, self.dec, self.sp1, self.sp2)
        self.xyPlot.update(self.ra, self.dec)

    def reset(self):
        t1 = self.ts.now()
        lst = t1.gmst + self.lon / 15.0       # in hours   
        if (self.mountType in ['E','K']):
            pierSide = self.ta.getPierSide()
            axis1 = self.ta.getAxis1()
            axis2 = self.ta.getAxis2()
            ha, ra, dec = eqAxesToEqu(axis1, axis2, self.lat, lst)
            self.initialRA = 15 * ra  # convert to degrees
            self.initialDec = dec
        elif (self.mountType in ['A','k']):     
            axis1 = self.ta.getAxis1()
            axis2 = self.ta.getAxis2()
            az, alt = altazAxesToAltAz(axis1, axis2, self.lat)
            t = self.ts.now()                    # skyfield datetime from computer 
            direction = self.site.at(t).from_altaz(az_degrees=az, alt_degrees=alt)
            ra_, dec_, distance_ = direction.radec()
            self.initialRA = ra_._degrees    # transform from skyfield units to float
            self.initialDec = dec_.degrees

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
                self.state = 'TRACKING'
            else:
                self.ta.disableTracking()
                self.log ('tracking disabled')
                self.window['startStopTrack'].update('Start Tracking')
                self.state = 'IDLE'

        if (ev == 'GuideN'):
            self.ta.guideCmd('n',50)

        if (ev == 'GuideS'):
            self.ta.guideCmd('s',50)

        if (ev == 'GuideE'):
            self.ta.guideCmd('e',50)

        if (ev == 'GuideW'):
            self.ta.guideCmd('w',50)

        if (ev == 'Nudge'):
            ra = self.ta.getRA() + (random.random() - 0.5) / 15.0    # add or subtract up to a half degree
            dec = self.ta.getDeclination() + (random.random() - 0.5)
            self.log('goto ra:{0:2.2f} dec:{1:2.2f}'.format(ra, dec))
            self.ta.gotoRaDec(ra, dec)


        if (ev == 'spiral'):
            self.ta.spiral(10)

        if (ev == 'zoomInT'):
            self.xyPlot.zoomIn()
            self.driftPlot.zoomIn()

        if (ev == 'zoomOutT'):
            self.xyPlot.zoomOut()
            self.driftPlot.zoomOut()

        if (ev == 'clearTrack'):
            self.clear()

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return
            self.run()


    def log(self, message):
        print (message)


    def updateTracking(self):
        if (self.ta.isTracking()):
            self.state = 'TRACKING'
            self.window['startStopTrack'].update('Stop Tracking')
        else:
            self.state = 'IDLE'
            self.window['startStopTrack'].update('Start Tracking')


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
        self.updateTracking()



