import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from teenastro import TeenAstro, deg2dms
import numpy as np  

numSamples = 100

class slewPlot():
    def __init__(self, window, ts, ta):
        self.window = window
        self.ts = ts
        self.ta = ta
        self.x = np.linspace(0, numSamples-1, numSamples)
        self.axis1 = np.zeros(numSamples)
        self.axis2 = np.zeros(numSamples)
        self.slewing = np.zeros(numSamples)
        self.fig, self.axes = plt.subplots(3, sharex=True, figsize=(10,6), dpi=36)
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
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=window['slew_cv'].TKCanvas)
        self.planets = load('de421.bsp')
        self.state = 'RECORD'

    def handleEvent(self, ev, v, w):
        if (ev == 'slewHome'):
            if not self.ta.isConnected():
                self.log('Not connected')
                return
            self.slewHome()

        if (ev == 'slewNorth'):
            self.slewDir(0, 45)

        if (ev == 'slewWest'):
            self.slewDir(270, 45)

        if (ev == 'slewEast'):
            self.slewDir(90, 45)

        if (ev == 'slewSouth'):
            self.slewDir(180, 45)

        if (ev == 'slewZenith'):
            self.slewDir(184, 45)

        if (ev == 'clearSlew'):
            self.ra = np.zeros(numSamples)
            self.dec =  np.zeros(numSamples)
            self.slewing =  np.zeros(numSamples)

            self.update(0, 0, 0)
            self.render()

        if (ev == 'saveSlew'):
            self.saveCsv()

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return
            if (self.state == 'RECORD'):
                self.run()


    def saveCsv(self):
        self.log ('Saving to driftTest.csv')
        data = np.stack((self.axis1, self.axis2, self.slewing), axis=1)
        np.savetxt("driftTest.csv", data, delimiter=",", fmt="%3.8f",
                       header="axis1, axis2, slewing", comments="")

    def log(self, message):
        print (message)

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


    def slewHome(self):
        if not self.ta.isConnected():
            self.log('Not connected')
            return
        self.state = 'RECORD'
        az = 0 
        alt = 45 
        self.log('goto az:{0} alt:{1}'.format(az, alt))
        self.ta.gotoAzAlt(az, alt)

    def slewDir(self, az, alt):
        if not self.ta.isConnected():
            self.log('Not connected')
            return
        self.log('goto az:{0} alt:{1}'.format(az, alt))
        self.ta.gotoAzAlt(az, alt)

    def run(self):
        slew = self.ta.isSlewing()
        axis1 = self.ta.getAxis1()          # get axis positions and compute RA/Dec
        axis2 = self.ta.getAxis2()
        self.update(axis1, axis2, slew)
        self.render()


