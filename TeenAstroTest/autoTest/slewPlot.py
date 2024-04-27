import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from teenastro import TeenAstro, deg2dms
import numpy as np  
import sys

numSamples = 100

# Pointing test case
testCase =  [
                {'az':0,'alt':10}, {'az':20,'alt':10},{'az':40,'alt':10},{'az':60,'alt':10},{'az':180,'alt':10},
                {'az':200,'alt':10},{'az':240,'alt':10}, {'az':300,'alt':10}, {'az':320,'alt':10},{'az':360,'alt':10}
            ]



class slewPlotPolar():
    def __init__(self, window, ts, ta, dpi):
        self.window = window
        self.ts = ts
        self.ta = ta
        self.az = []
        self.alt = []
        self.fig, self.ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(10,6), dpi=dpi)
        self.line, = self.ax.plot(self.az, self.alt)
        self.ax.set_rmin(90)
        self.ax.set_rmax(-15)
#        self.ax.set_rticks([0,15,30,45,60,75,90],fontsize=20)  # radial ticks
        self.ax.set_rticks([0,15,30,45,60,75,90])  # radial ticks
        self.ax.set_rlabel_position(-22.5)  
        self.ax.set_theta_zero_location("N")
        self.ax.grid(True)
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=window['slew_cv_polar'].TKCanvas)
        self.planets = load('de421.bsp')
        self.testStep = 0
        self.state = 'STOP'

    def connect(self, ta):
        self.ta = ta
        self.subName = self.ta.getSubName()

    def clear(self):
        self.az = []
        self.alt = []

    def log(self, message):
        print (message)

    def update(self, az, alt):
        self.az = np.append(self.az, az)
        self.alt = np.append(self.alt, alt)
        self.line.set_data(np.deg2rad(self.az), self.alt)
        self.render()

    def render(self):
        self.figure_canvas_agg.draw()
        self.figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)



class slewPlotT():
    def __init__(self, window, ts, ta, dpi):
        self.window = window
        self.ts = ts
        self.ta = ta
        self.x = np.linspace(0, numSamples-1, numSamples)
        self.axis1 = np.zeros(numSamples)
        self.axis2 = np.zeros(numSamples)
        self.slewing = np.zeros(numSamples)
        self.fig, self.axes = plt.subplots(3, sharex=True, figsize=(10,6), dpi=dpi)
        self.axes[0].set_xlabel('time (seconds)')
        self.axes[0].set_ylabel('Axis1 (degrees)')
        self.axes[0].set_ylim(-100, 300)
        self.axes[1].set_ylabel('Axis2 (degrees)')
        self.axes[1].set_ylim(-100, 300)
        self.axes[2].set_ylabel('slewing')
        self.axes[2].set_ylim(-1, 2)
        self.line1, = self.axes[0].plot(self.x,self.axis1,'green')
        self.line2, = self.axes[1].plot(self.x,self.axis2,'red')
        self.line3, = self.axes[2].plot(self.x,self.slewing,'blue')
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=window['slew_cv_t'].TKCanvas)

    def connect(self, ta):
        self.ta = ta

    def clear(self):
        self.axis1 = np.zeros(numSamples)
        self.axis2 = np.zeros(numSamples)
        self.slewing = np.zeros(numSamples)

    def render(self):
        self.figure_canvas_agg.draw()
        self.figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)

    def update(self, axis1, axis2, slewing):
        self.axis1 = np.append(self.axis1, axis1)
        self.line1.set_ydata(self.axis1[-100:])   # degrees
        self.axis2 = np.append(self.axis2, axis2)
        self.slewing = np.append(self.slewing, slewing)
        self.line2.set_ydata(self.axis2[-100:])
        self.line3.set_ydata(self.slewing[-100:])
        self.render()


    def saveCsv(self):
        self.log ('Saving to driftTest.csv')
        data = np.stack((self.axis1, self.axis2, self.slewing), axis=1)
        np.savetxt("driftTest.csv", data, delimiter=",", fmt="%3.8f",
                       header="axis1, axis2, slewing", comments="")

class slewPlot():
    def __init__(self, window, ts, ta, dpi):
        self.window = window
        self.ts = ts
        self.ta = ta
        self.state = 'IDLE'
        self.tPlot = slewPlotT(self.window, self.ts, self.ta, dpi)
        self.pPlot = slewPlotPolar(self.window, self.ts, self.ta, dpi)
        self.planets = load('de421.bsp')

    def connect(self, ta):
        self.ta = ta
        self.tPlot.connect(ta)
        self.pPlot.connect(ta)
        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.mountType = self.ta.readMountType()
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)

    def az2string(self, dms):
        return "{0:03d}º{1:02d}'{2:02d}''".format(dms[0], dms[1], dms[2])

    def alt2string(self, dms):
        return "{0:+02d}º{1:02d}'{2:02d}''".format(dms[0], dms[1], dms[2])

    def log(self, message):
        print (message)

    def slewHome(self):
        self.log('goto Home')
        self.ta.goHome()

    def slewDir(self, az, alt):
        self.log('goto az:{0} alt:{1}'.format(az, alt))
        self.ta.gotoAzAlt(az, alt)

    def run(self):
        slew = self.ta.isSlewing()
        axis1 = self.ta.getAxis1() 
        axis2 = self.ta.getAxis2()
        self.tPlot.update(axis1, axis2, slew)

        az = self.ta.getAzimuth()
        alt = self.ta.getAltitude()        
        self.pPlot.update(az, alt)

        # update parameters in text box
        ra = self.ta.getRA()
        dec = self.ta.getDeclination()
        t1 = self.ta.readDateTime()         # python datetime from TeenAstro
        t2 = self.ts.from_datetime(t1)      # converted to skyfield format
        pos = self.site.at(t2).observe(Star(ra_hours=ra, dec_degrees=dec))
        alt_comp, az_comp, dist = pos.apparent().altaz()
        self.window['az_comp'].Update(self.az2string(deg2dms(az_comp.degrees)))
        self.window['alt_comp'].Update(self.az2string(deg2dms(alt_comp.degrees)))

        self.window['az_disp'].Update(self.az2string(deg2dms(az)))
        self.window['alt_disp'].Update(self.alt2string(deg2dms(alt)))



    def handleEvent(self, ev, v, w):
        if (ev == 'slewHome'):
            self.slewHome()

        if (ev == 'slewNorth'):
            self.slewDir(0, 0)

        if (ev == 'slewWest'):
            self.slewDir(270, 0)

        if (ev == 'slewEast'):
            self.slewDir(90, 0)

        if (ev == 'slewSouth'):
            self.slewDir(180, 0)

        if (ev == 'slewZenith'):
            self.slewDir(180, 90)

        if (ev == 'clearSlew'):
            self.tPlot.clear()
            self.pPlot.clear()

        if (ev == 'stopSlew'):
            self.ta.abort()
            self.state = 'IDLE'
            self.window['autoSlew'].update(disabled = False)

        if ev == 'autoSlew':
            if not self.ta.isConnected():
                self.log('Not connected')
                return
            self.window['autoSlew'].update(disabled = True)
            self.state = 'AUTO'
            self.testStep = 0
            return

        if (ev == 'flipMount'):
            self.log('Requesting Flip')
            self.ta.flipMount()

        if (ev == 'park'):
            self.log('Park')
            self.ta.park()

        if (ev == 'unpark'):
            self.log('Unpark')
            self.ta.unpark()

        if (ev == 'setPark'):
            self.log('Set Park')
            self.ta.setPark()

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return

            if (self.state == 'AUTO'):
                if not (self.ta.isSlewing()):
                    altaz = testCase[self.testStep]
                    self.log('goto az:{0} alt:{1}'.format(altaz['az'], altaz['alt']))
                    self.ta.gotoAzAlt(altaz['az'], altaz['alt'])

                    self.testStep = self.testStep + 1
                    if (self.testStep == len(testCase)):
                        self.log('Test done - go Home')
                        self.state = 'IDLE'
                        self.window['autoSlew'].update(disabled = False)
                        self.testStep = 0
                        self.ta.goHome()
            self.run()
