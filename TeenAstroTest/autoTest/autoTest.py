# 
# -*- coding: UTF-8 -*-#
# TeenAstro Auto Test
#
# François Desvallées
#

import math, time, sys, argparse, csv
import PySimpleGUI as sg
import numpy as np  
from pandas import read_csv
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from skyfield.positionlib import Apparent, Barycentric, Astrometric, Distance
from skyfield.earthlib import refraction
from skyfield.projections import build_stereographic_projection

import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial.tools.list_ports

sys.path.insert(0, '../mountSim')
from teenastro import TeenAstro, deg2dms

numSamples = 100
sampleFreq = 1
earth = 399 # NEAF code for the earth center of mass

sgCommTypeSerial = [sg.Radio('Serial', "RADIO1", size=(8, 1), enable_events=True, key='-serial-'),
          sg.Text('Device:', size=(10, 1)),
          sg.Combo('ComPorts', [], key='-ComPorts-', size=(20, 1), disabled=True)]

sgCommTypeTCP = [sg.Radio('TCP', "RADIO1", default = True, size=(8, 1), enable_events=True, key='-tcp-'),
          sg.Text('IP Address:', size=(10, 1)),
          sg.Input('192.168.0.21', key='-IPADDR-', size=(20, 1))]

commFrame = sg.Frame('Comm Port',[sgCommTypeSerial,sgCommTypeTCP])

topRow = [commFrame, sg.B('Connect', key='connect'),sg.B('Exit')]

coordFrame = sg.Frame('Coordinates',[[sg.T(' ', size=10), sg.T('Displayed', size=10),sg.T('Computed', size=10)],
                                     [sg.T('Azimuth', size=10), sg.T('0', size=10, key='az_disp'),sg.T('0', size=10, key='az_comp')],
                                     [sg.T('Altitude', size=10), sg.T('0', size=10, key='alt_disp'),sg.T('0', size=10, key='alt_comp')]])

driftFrame = sg.Frame('Drift Rates', [[sg.T('RA (arc-sec/sec):'), sg.T('0', key='ra_rate')],[sg.T('Dec (arc-sec/sec):'), sg.T('0', key='dec_rate')]]) 

def sgSpin(label):
    return sg.Spin(values = [i for i in range(-100,100,10)], initial_value=0, key=label, readonly=True, background_color='white', size=6)

homeErrorFrame = sg.Frame('Home Errors - arc-minutes',[[sg.T('Axis1', size=6), sgSpin('home_error_axis1')],
                                                       [sg.T('Axis2', size=6), sgSpin('home_error_axis2')]])

poleErrorFrame = sg.Frame('Pole Errors - arc-minutes',[[sg.T('Azimuth', size=6),  sgSpin('pole_error_az')],
                                                       [sg.T('Altitude', size=6), sgSpin('pole_error_alt')]])

pointTestTab = [[sg.Column([
                    [sg.B(button_text = 'Start', key='startPointTest'),sg.B(button_text = 'Clear', key='clearPoint'),
                     sg.B(button_text = 'Save', key='savePoint'), ],
                    [sg.Canvas(key='point_cv', size=(640, 400))]]
                    ), coordFrame]
               ]

driftTestTab = [[sg.Column([
                    [sg.B(button_text = 'Start', key='startStopDrift'),sg.B(button_text = 'Clear', key='clearDrift'),
                     sg.B(button_text = 'Save', key='saveDrift'), sg.B(button_text = '+',key='zoomIn'),sg.B(button_text = '-',key='zoomOut')],
                    [sg.Canvas(key='drift_cv', size=(640, 400))]]), driftFrame]
                ]

alignmentTab = [[sg.Column([
                    [sg.B(button_text = 'Start', key='startStopAlignment'),
                      sg.B(button_text = '+',key='zoomInA'),sg.B(button_text = '-',key='zoomOutA'),
                      sg.DropDown([], key='alignmentTarget', size=25),
                      sg.B(button_text='Goto', key='alignmentGoto')],
                    [sg.Canvas(key='alignment_cv', size=(640, 400))]]), sg.Column([[homeErrorFrame], [poleErrorFrame]])]
                ]


bottomRow = sg.Output(key='Log',  size=(80, 12))
#bottomRow = []

layout = [ [topRow],
          [sg.TabGroup([[
            sg.Tab('Pointing Test', pointTestTab, key='ptest'),
            sg.Tab('Drift Test', driftTestTab, key='dtest'),
            sg.Tab('Alignment Test', alignmentTab, key='atest')]],
            key='tgroup')],
          [bottomRow]
         ]

# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-p', '--ip', help='TeenAstro IP address')
  opts = parser.parse_args(args)
  if opts.ip == None:
    opts.ip = '192.168.0.21'
  return opts



# Utility functions

def convertAxis1(axis1, pierSide):
    if (pierSide == 'E'):
        return axis1
    else:
        return axis1+180

def convertAxis2(axis2, pierSide):
    if (pierSide == 'E'):
        return axis2 
    else:
        return 180-axis2

# Pointing test case
testCase =  [
                {'az':0,'alt':40}, {'az':20,'alt':40},{'az':40,'alt':40},{'az':60,'alt':40},{'az':80,'alt':40},
                {'az':100,'alt':40}, {'az':120,'alt':40},{'az':140,'alt':40},{'az':160,'alt':40},{'az':180,'alt':40},
                {'az':200,'alt':40}, {'az':220,'alt':40},{'az':240,'alt':40},{'az':260,'alt':40},{'az':280,'alt':40},
                {'az':300,'alt':40}, {'az':320,'alt':40},{'az':340,'alt':40}
            ]

class pointingPlot():
    def __init__(self, window, ta, ts):
        self.window = window
        self.ta = ta
        self.ts = ts
        self.az = []
        self.alt = []
        self.fig, self.ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(10,6), dpi=36)
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




class driftPlot():
    def __init__(self, window, ta, ts):
        self.window = window
        self.ta = ta
        self.ts = ts
        self.x = np.linspace(0, numSamples-1, numSamples)
        self.ra = np.zeros(numSamples)
        self.dec = np.zeros(numSamples)
        self.fig, self.axes = plt.subplots(2, sharex=True, sharey=True, figsize=(10,6), dpi=36)
        self.yScale = 10
        self.axes[0].set_ylim(-self.yScale,self.yScale)
        self.axes[0].set_xlabel('time (seconds)')
        self.axes[0].set_ylabel('Right Ascension (arc-sec)')
        self.axes[1].set_ylabel('Declination (arc-sec)')
        self.line1, = self.axes[0].plot(self.x,self.ra,'green')
        self.line2, = self.axes[1].plot(self.x,self.dec,'red')
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=window['drift_cv'].TKCanvas)
        self.planets = load('de421.bsp')
        self.state = 'STOP'

    def handleEvent(self, ev, v, w):
        if (ev == 'startStopDrift'):
            if not self.ta.isConnected():
                self.log('Not connected')
                return

            if self.state == 'STOP':
                self.log ('running')
                self.window['startStopDrift'].update('Stop')
                self.start()
            else:
                self.log ('stopped')
                self.window['startStopDrift'].update('Start')
                self.state = 'STOP'

        if (ev == 'clearDrift'):
            self.ra = np.zeros(numSamples)
            self.dec =  np.zeros(numSamples)
            self.update(0, 0)
            self.render()

        if (ev == 'saveDrift'):
            self.saveCsv()

        if (ev == 'zoomIn'):
            self.yScale = self.yScale / 2
            self.axes[0].set_ylim(-self.yScale,self.yScale)
            self.render()

        if (ev == 'zoomOut'):
            self.yScale = self.yScale * 2
            self.axes[0].set_ylim(-self.yScale,self.yScale)
            self.render()

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return
            if (self.state == 'RECORD'):
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

    def update(self, ra, dec):
        self.ra = np.append(self.ra, ra)
        self.line1.set_ydata(3600*self.ra[-100:])   # arc-seconds
        self.dec = np.append(self.dec, dec)
        self.line2.set_ydata(3600*self.dec[-100:])

    def start(self):
        if not self.ta.isConnected():
            self.log('Not connected')
            return
        self.log ('start Drift Test')
        self.testStep = 0

        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative

        t1 = self.ts.now()
        lst = t1.gmst + self.lon / 15       # in hours   

        pierSide = self.ta.getPierSide()
        axis1 = self.ta.getAxis1()
        axis2 = self.ta.getAxis2()
        self.initialRA = 15.0 * lst - axis1  # in degrees
        self.initialDec = axis2
        self.state = 'RECORD'

    def run(self):
        t1 = self.ts.now()
        lst = 15.0 * t1.gmst + self.lon         # in degrees   

        pierSide = self.ta.getPierSide()
        axis1 = self.ta.getAxis1()
        axis2 = self.ta.getAxis2()
        ra = lst - axis1  # in degrees
        dec = axis2
        self.update(ra-self.initialRA, dec-self.initialDec)
        self.render()


class alignmentPlot():
    def __init__(self, window, ta, ts):
        self.window = window
        self.ta = ta
        self.ts = ts
        self.planets = load('de421.bsp')
        # Load star catalog and compute Jnow from J2000 coordinates
        self.stars = read_csv('stars.csv')
        t = ts.now()
        astrometric = self.planets['earth'].at(t).observe(Star(ra_hours=self.stars.ra_hours, dec_degrees=self.stars.dec_degrees))
        ra, dec, dist = astrometric.radec(epoch=t)
        self.stars['ra_hours'] = ra.hours
        self.stars['dec_degrees'] = dec.degrees
        self.namedStarsIndex = self.stars['name'].notnull()
        self.namedStars = self.stars[self.namedStarsIndex].copy()
        self.namedStars['starDesignation'] =  self.namedStars['bayer'] + ' ' + self.namedStars['const'] + '-' + self.namedStars['name']    
        self.namedStars.sort_values(by=['const','bayer'], inplace=True)
        self.starList = self.namedStars['starDesignation'].tolist()
        self.window['alignmentTarget'].update(values=self.starList)
        self.window['alignmentTarget'].update(self.starList[0])
        self.field_of_view_degrees = 45.0
        self.limiting_magnitude = 5.0
        self.marker_size = ((self.limiting_magnitude - self.stars.magnitude) ** 2.0) #/ (self.field_of_view_degrees / 10)
        self.fig, self.ax = plt.subplots(figsize=(6, 6), dpi=40)
        plt.subplots_adjust(left=0, bottom=0, right=1, top=1, wspace=0, hspace=0)
        self.fig.add_artist(lines.Line2D([0.45, 0.55], [0.5, 0.5], linewidth=1, color='black')) # cursor H
        self.fig.add_artist(lines.Line2D([0.5, 0.5], [0.45, 0.55], linewidth=1, color='black')) # cursor V
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=self.window['alignment_cv'].TKCanvas)
        self.home_error_axis1 = 0.0
        self.home_error_axis2 = 0.0
        self.state = 'STOP'

    def handleEvent(self, ev, v, w):
        if (ev == 'startStopAlignment'):
            if not self.ta.isConnected():
                self.log('Not connected')
                return

            if self.state == 'STOP':
                self.log ('running')
                self.window['startStopAlignment'].update('Stop')
                self.start()
            else:
                self.log ('stopped')
                self.window['startStopAlignment'].update('Start')
                self.state = 'STOP'

        if (ev == 'zoomInA'):
            self.field_of_view_degrees = self.field_of_view_degrees / 1.5
            self.marker_size = ((self.limiting_magnitude - self.stars.magnitude) ** 2.0) #/ (self.field_of_view_degrees / 10)
            angle = np.pi - self.field_of_view_degrees / 360.0 * np.pi
            limit = np.sin(angle) / (1.0 - np.cos(angle))
            self.ax.set_xlim(-limit, limit)
            self.ax.set_ylim(-limit, limit)
            self.render()

        if (ev == 'zoomOutA'):
            self.field_of_view_degrees = self.field_of_view_degrees * 1.5
            self.marker_size = ((self.limiting_magnitude - self.stars.magnitude) ** 2.0) #/ (self.field_of_view_degrees / 10)
            angle = np.pi - self.field_of_view_degrees / 360.0 * np.pi
            limit = np.sin(angle) / (1.0 - np.cos(angle))
            self.ax.set_xlim(-limit, limit)
            self.ax.set_ylim(-limit, limit)
            self.render()

        if (ev == 'alignmentGoto'):
            bayer, name = self.window['alignmentTarget'].get().split('-')
            star = self.namedStars[self.namedStars.name == name]
            ra = star.ra_hours.values[0]
            dec = star.dec_degrees.values[0]
            self.log('Goto {0}'.format (self.window['alignmentTarget'].get()))
            self.log(self.ta.gotoRaDec(ra,dec))

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return
            if (self.state == 'RUN'):
                self.home_error_axis1 = float(v['home_error_axis1']) / 60        # in degrees
                self.home_error_axis2 = float(v['home_error_axis2']) / 60 
                self.pole_error_az = float(v['pole_error_az'])  / 60
                self.pole_error_alt = float(v['pole_error_alt']) / 60
                self.update()

    def log(self, message):
        print (message)

    def render(self):
        self.ax.scatter(self.stars['x'], self.stars['y'],s=self.marker_size, color='k')
        self.figure_canvas_agg.draw()
        self.figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)

    def getAxisCoords(self):
        pierSide = self.ta.getPierSide()
        axis1 = self.ta.getAxis1()
        axis2 = self.ta.getAxis2()        
        lst = self.ta.getLST()            # in hours
        ha = convertAxis1(axis1, pierSide) / 15 
        ra = lst - ha                      # in hours
        dec = convertAxis2(axis2, pierSide)
        return (ra,dec,lst,ha)   

    def computePolarError(self, Δa, Δe, dec, lst, ha):    # from Wikipedia
        Φ = np.radians(self.lat)
        δ = np.radians(dec)
        h = np.radians(ha*15) 
        Δα = np.radians(Δe) * np.tan(δ) * np.sin(h) + np.radians(Δa) * (np.sin(Φ) - np.cos(Φ) * np.tan(δ) * np.cos(h))
        Δδ = np.radians(Δe) * np.cos(h) + np.radians(Δa) * np.cos(Φ) * np.sin(h)
        return np.degrees(Δα) / 15, np.degrees(Δδ)

    def project(self, ra, dec):
        t = self.ts.from_datetime(self.ta.readDateTime())      # converted to skyfield format
        center = self.site.at(t).observe(Star(ra_hours=ra, dec_degrees=dec))
        projection = build_stereographic_projection(center)
        star_positions = self.site.at(t).observe(Star(ra_hours=self.stars.ra_hours, dec_degrees=self.stars.dec_degrees)) 
        self.stars['x'], self.stars['y'] = projection(star_positions)

    def update(self):
        ra, dec, lst, ha = self.getAxisCoords()
        ra = ra + self.home_error_axis1 /  15
        dec = dec + self.home_error_axis2 
        Δα, Δδ = self.computePolarError(self.pole_error_az, self.pole_error_alt, dec, lst, ha)
        ra = ra + Δα
        dec = dec + Δδ
        self.project(ra, dec)
        angle = np.pi - self.field_of_view_degrees / 360.0 * np.pi
        limit = np.sin(angle) / (1.0 - np.cos(angle))
        self.ax.clear()
        self.ax.set_xlim(-limit, limit)
        self.ax.set_ylim(-limit, limit)
        self.ax.xaxis.set_visible(False)
        self.ax.yaxis.set_visible(False)
        self.ax.set_aspect(1.0)        
        self.render()

    def start(self):
        if not self.ta.isConnected():
            self.log('Not connected')
            return

        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
        ra, dec, lst, ha = self.getAxisCoords()
        self.project(ra, dec)
        self.scatter = self.ax.scatter(self.stars['x'], self.stars['y'],s=self.marker_size, color='k')
        self.state = 'RUN'




# Main program
class Application:

    def __init__(self, options):

        self.ts = load.timescale()

        self.window = sg.Window('TeenAstro AutoTest', layout, finalize=True, size=(1024,720))
        self.window['-IPADDR-'].update(options.ip)
        self.ta = TeenAstro('tcp', options.ip)

        self.dp = driftPlot(self.window, self.ta, self.ts)
        self.pp = pointingPlot(self.window, self.ta, self.ts)
        self.ap = alignmentPlot(self.window, self.ta, self.ts)
        self.run()

    def run(self):
        while True:
            event, values = self.window.read(1000 / sampleFreq)

            if event in (sg.WIN_CLOSED, 'Exit'): 
                break

            if event == '-serial-':
                self.log('Serial selected')
                self.window['-ComPorts-'].SetFocus()
                self.window['-ComPorts-'].update(disabled = False)
                self.window['-IPADDR-'].update(disabled = True)
                # collect available serial ports on the system
                serPortList = serial.tools.list_ports.comports()
                serPortsDetected = []
                for port in serPortList:
                    serPortsDetected.append(port.device)
                self.window['-ComPorts-'].update(values = sorted(serPortsDetected))            
                self.ta.portType = 'serial'
             
            if event == '-tcp-':
                self.log('TCP selected')
                self.portType = 'tcp'

            if event == 'connect':
                if self.ta.isConnected():
                    self.ta.close()
                    self.window['connect'].update('Connect')
                else:
                    if self.ta.portType == 'tcp':
                        self.ta.portName = values['-IPADDR-']
                    else:
                        self.ta.portName = values['-ComPorts-']
                    self.connect() 
                    self.window['connect'].update('Disconnect')

            t = self.window['tgroup'].get()
            if t == 'dtest':
                self.dp.handleEvent(event, values, self.window)
            elif t == 'ptest':
                self.pp.handleEvent(event, values, self.window)
            elif t == 'atest':
                self.ap.handleEvent(event, values, self.window)
                
        self.window.close()


    def connect(self):
        try:
            p = self.ta.open()
        except:
            self.log ("Error connecting to TeenAstro")
            return
        if p == None:
            self.log ("Error connecting to TeenAstro")
            return

    def log(self, message):
        print (message)



if __name__ == '__main__':
    options = readOptions()
    Application(options)



