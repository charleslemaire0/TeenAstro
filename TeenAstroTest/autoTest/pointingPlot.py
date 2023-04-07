import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from teenastro import TeenAstro, deg2dms
import numpy as np  
import sys

# Pointing test case
testCase =  [
                {'az':0,'alt':10}, {'az':20,'alt':10},{'az':40,'alt':10},{'az':60,'alt':10},{'az':180,'alt':10},
                {'az':200,'alt':10},{'az':240,'alt':10}, {'az':300,'alt':10}, {'az':320,'alt':10},{'az':360,'alt':10}
            ]



class pointingPlot():
    def __init__(self, window, ts, ta):
        self.window = window
        self.ts = ts
        self.ta = ta
        self.az = []
        self.alt = []
        if sys.platform == "darwin":
            self.dpi = 36
        elif sys.platform == "linux":
            self.dpi = 64
        elif sys.platform == "win32":
            self.dpi = 64
        self.fig, self.ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(10,6), dpi=self.dpi)
        self.line, = self.ax.plot(self.az, self.alt)
        self.ax.set_rmin(90)
        self.ax.set_rmax(-15)
        self.ax.set_rticks([0,15,30,45,60,75,90],fontsize=20)  # radial ticks
        self.ax.set_rlabel_position(-22.5)  
        self.ax.set_theta_zero_location("N")
        self.ax.grid(True)
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=window['point_cv'].TKCanvas)
        self.planets = load('de421.bsp')
        self.testStep = 0
        self.state = 'STOP'

    def handleEvent(self, ev, v, window):
        if ev == 'clearPoint':
            self.az = []
            self.alt =  []
            self.line.set_data(self.az, self.alt)
            self.render()

        if ev == 'startPointTest':
            if not self.ta.isConnected():
                self.log('Not connected')
                return
            window['startPointTest'].update(disabled = True)
            self.state = 'RUNNING'
            return

        if (ev == 'savePoint'):
            self.saveCsv()
            return

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return

            self.update()
            if self.state == 'RUNNING':
                if self.ta.isSlewing():
                    return
    
                altaz = testCase[self.testStep]
                self.log('goto az:{0} alt:{1}'.format(altaz['az'], altaz['alt']))
                self.ta.gotoAzAlt(altaz['az'], altaz['alt'])

                self.testStep = self.testStep + 1
                if (self.testStep == len(testCase)):
                    self.log('Test done')
                    self.state = 'STOP'
                    window['startPointTest'].update(disabled = False)
                    self.testStep = 0



    def saveCsv(self):
        self.log ('Saving to pointTest.csv')
        data = np.stack((self.az, self.alt), axis=1)
        np.savetxt("pointTest.csv", data, delimiter=",", fmt="%.2f",
                       header="az, alt", comments="")

    def log(self, message):
        print (message)

    def az2string(self, dms):
        return "{0:03d}º{1:02d}'{2:02d}''".format(dms[0], dms[1], dms[2])

    def alt2string(self, dms):
        return "{0:+02d}º{1:02d}'{2:02d}''".format(dms[0], dms[1], dms[2])

    def update(self):
        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
        alt = self.ta.getAltitude()
        az = self.ta.getAzimuth()
        ra = self.ta.getRA()
        dec = self.ta.getDeclination()
        t1 = self.ta.readDateTime()         # python datetime from TeenAstro
        t2 = self.ts.from_datetime(t1)      # converted to skyfield format
        pos = self.site.at(t2).observe(Star(ra_hours=ra, dec_degrees=dec))
        alt_comp, az_comp, dist = pos.apparent().altaz()
        self.window['az_comp'].Update(self.az2string(deg2dms(az_comp.degrees)))
        self.window['alt_comp'].Update(self.az2string(deg2dms(alt_comp.degrees)))

        self.az = np.append(self.az, az)
        self.alt = np.append(self.alt, alt)
        self.window['az_disp'].Update(self.az2string(deg2dms(az)))
        self.window['alt_disp'].Update(self.alt2string(deg2dms(alt)))
        self.line.set_data(np.deg2rad(self.az), self.alt)
        self.render()

    def render(self):
        self.figure_canvas_agg.draw()
        self.figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)

    def start(self):
        self.axis1 = self.axis2 = 0
        self.state = 'RUN'
        self.testStep = 0
        self.log ('starting Point Test')
        res = self.ta.gotoAzAlt(testCase[self.testStep]['az'], testCase[self.testStep]['alt'])


    def run(self):
        if self.state == 'STOP':
            return

        if self.state == 'RECORD':
            alt = self.ta.getAltitude()
            az = self.ta.getAzimuth()
            self.update(az, alt)
            self.render()
            return

        # Finished slewing
        t1 = self.ta.readDateTime()         # python datetime from TeenAstro
        t2 = self.ts.from_datetime(t1)      # converted to skyfield format
        lst = t2.gmst + self.lon / 15   

#        print (refraction(0.0, temperature_C=15.0, pressure_mbar=1030.0))
        axis1 = self.ta.getAxis1() 
        axis2 = self.ta.getAxis2()
        alt = self.ta.getAltitude()
        az = self.ta.getAzimuth()
        pierSide = self.ta.getPierSide()

        # report test result, execute next step
        self.update(az, alt)
        self.testStep += 1
        if self.testStep == len(testCase):
            self.log('Finished test - go Home')
            self.ta.goHome()
            self.busy = False
            self.state = 'STOP'
            return
        res = self.ta.gotoAzAlt(testCase[self.testStep]['az'], testCase[self.testStep]['alt'])


