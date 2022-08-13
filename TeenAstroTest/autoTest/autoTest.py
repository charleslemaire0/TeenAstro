# 
# -*- coding: UTF-8 -*-#
# TeenAstro Auto Test
#
# François Desvallées
#

import math, time, sys, argparse, csv
import PySimpleGUI as sg
import numpy as np  

from skyfield.api import wgs84, load, position_of_radec
from skyfield.positionlib import Apparent, Barycentric, Astrometric, Distance
from skyfield.earthlib import refraction
from skyfield.api import utc

import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial.tools.list_ports

sys.path.insert(0, '../mountSim')
from teenastro import TeenAstro, deg2dms

numSamples = 100
sampleFreq = 1


sgCommTypeSerial = [sg.Radio('Serial', "RADIO1", size=(8, 1), enable_events=True, key='-serial-'),
          sg.Text('Device:', size=(10, 1)),
          sg.Combo('ComPorts', [], key='-ComPorts-', size=(20, 1), disabled=True)]

sgCommTypeTCP = [sg.Radio('TCP', "RADIO1", default = True, size=(8, 1), enable_events=True, key='-tcp-'),
          sg.Text('IP Address:', size=(10, 1)),
          sg.Input('192.168.0.21', key='-IPADDR-', size=(20, 1))]

commFrame = sg.Frame('Comm Port',[sgCommTypeSerial,sgCommTypeTCP])

topRow = [commFrame, sg.B('Connect', key='connect'),sg.B('Exit')]

pointTestTab = [[sg.Column([
                    [sg.B(button_text = 'Start', key='startStopPoint'),sg.B(button_text = 'Clear', key='clearPoint'),
                     sg.B(button_text = 'Save', key='savePoint')],
                    [sg.Canvas(key='point_cv', size=(640, 400))]]
                    )]
               ]

driftTestTab = [[sg.Column([
                    [sg.B(button_text = 'Start', key='startStopDrift'),sg.B(button_text = 'Clear', key='clearDrift'),
                     sg.B(button_text = 'Save', key='saveDrift'), sg.B(button_text = '+',key='zoomIn'),sg.B(button_text = '-',key='zoomOut')],
                    [sg.Canvas(key='drift_cv', size=(640, 400))]]
                )]
                ]

bottomRow = sg.Output(key='Log',  size=(80, 4))

layout = [ [topRow],
          [sg.TabGroup([[
            sg.Tab('Pointing Test', pointTestTab, key='ptest'),
            sg.Tab('Drift Test', driftTestTab, key='dtest')]],
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


class pointingPlot():
    def __init__(self, canvas, ta, ts):
        self.ta = ta
        self.ts = ts
        self.az = []
        self.alt = []
        self.deltaRA = []
        self.deltaDec = []

        self.fig, self.ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(10,6), dpi=36)
        self.line, = self.ax.plot(self.az, self.alt)
        self.ax.set_rmin(90)
        self.ax.set_rmax(-15)
        self.ax.set_rticks([0,15,30,45,60,75,90],fontsize=20)  # radial ticks
        self.ax.set_rlabel_position(-22.5)  
        self.ax.set_theta_zero_location("N")
        self.ax.grid(True)
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=canvas)
        self.state = 'STOP'

    def handleEvent(self, ev, window):
        if (ev == 'clearPoint'):
            self.az = []
            self.alt =  []
            self.line.set_data(self.az, self.alt)
            self.render()

        if (ev == 'startStopPoint'):
            if not self.ta.isConnected():
                self.log('Not connected')
                return

            if self.state == 'STOP':
                self.log ('running')
                window['startStopPoint'].update('Stop')
                self.state = 'RECORD'
            else:
                self.log ('stopped')
                window['startStopPoint'].update('Start')
                self.state = 'STOP'

        if (ev == 'savePoint'):
            self.saveCsv()

        if (ev =='__TIMEOUT__'):
            if not self.ta.isConnected():
                return

        if (self.state == 'RECORD'):
            alt = self.ta.getAltitude()
            az = self.ta.getAzimuth()
            self.update(az, alt)
            self.render()

    def saveCsv(self):
        self.log ('Saving to pointTest.csv')
        data = np.stack((self.az, self.alt), axis=1)
        np.savetxt("pointTest.csv", data, delimiter=",", fmt="%.2f",
                       header="az, alt, deltaRA, deltaDec", comments="")

    def log(self, message):
        print (message)

    def update(self, az, alt):
        self.az = np.append(self.az, az)
        self.alt = np.append(self.alt, alt)
        self.line.set_data(np.deg2rad(self.az), self.alt)

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

        apparent = self.site.at(t2).from_altaz(alt_degrees=alt, az_degrees=az, distance=Distance(au=100))
        ra,dec, dist = apparent.radec()

        deltaRA = (15 * (lst - ra.hours) - self.convertAxis1(axis1, pierSide)) % 360
        if (deltaRA > 180):
            deltaRA -= 360
        deltaDec = (dec.degrees - self.convertAxis2(axis2, pierSide)) % 90
        if (deltaDec > 45):
            deltaDec -= 90

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
    def __init__(self, canvas, ta, ts):
        self.ta = ta
        self.ts = ts
        self.x = np.linspace(0, 100, numSamples)
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
        self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=canvas)
        self.planets = load('de421.bsp')
        self.state = 'STOP'

    def handleEvent(self, ev, window):
        if (ev == 'startStopDrift'):
            if not self.ta.isConnected():
                self.log('Not connected')
                return

            if self.state == 'STOP':
                self.log ('running')
                window['startStopDrift'].update('Stop')
                self.start()
            else:
                self.log ('stopped')
                window['startStopDrift'].update('Start')
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

    def update(self, ra, dec):
        self.ra = np.append(self.ra, ra)
        self.line1.set_ydata(3600*self.ra[-100:])   # arc-seconds
        self.dec = np.append(self.dec, dec)
        self.line2.set_ydata(3600*self.dec[-100:])

    def start(self):
        if not self.ta.isConnected():
            self.log('Not connected')
            return
        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
        self.log ('start Drift Test')
        self.testStep = 0

        t1 = self.ts.now()
        lst = t1.gmst + self.lon / 15       # in hours   

        pierSide = self.ta.getPierSide()
        axis1 = convertAxis1(self.ta.getAxis1(), pierSide)
        axis2 = convertAxis2(self.ta.getAxis2(), pierSide)
        self.initialRA = 15.0 * lst - axis1  # in degrees
        self.initialDec = axis2
        self.state = 'RECORD'

    def run(self):
        t1 = self.ts.now()
        lst = 15.0 * t1.gmst + self.lon         # in degrees   

        pierSide = self.ta.getPierSide()
        axis1 = convertAxis1(self.ta.getAxis1(), pierSide)
        axis2 = convertAxis2(self.ta.getAxis2(), pierSide)
        ra = lst - axis1  # in degrees
        dec = axis2
        self.update(ra-self.initialRA, dec-self.initialDec)
        self.render()





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

        self.ts = load.timescale()
        self.ta = TeenAstro('tcp', options.ip)

        self.window = sg.Window('TeenAstro AutoTest', layout, finalize=True, size=(1024,640))
        self.dp = driftPlot(self.window['drift_cv'].TKCanvas, self.ta, self.ts)
        self.pp = pointingPlot(self.window['point_cv'].TKCanvas, self.ta, self.ts)
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
                        self.portName = values['-IPADDR-']
                    else:
                        self.ta.portName = values['-ComPorts-']
                    self.connect() 
                    self.window['connect'].update('Disconnect')

            t = self.window['tgroup'].get()
            if t == 'dtest':
                self.dp.handleEvent(event, self.window)
            elif t == 'ptest':
                self.pp.handleEvent(event, self.window)
                
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
