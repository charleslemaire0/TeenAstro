# 
# -*- coding: UTF-8 -*-#
# TeenAstro Auto Test
#
# François Desvallées
#

import math, time, sys, argparse, csv, sys
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
from pointingPlot import pointingPlot
from driftPlot import driftPlot
from slewPlot import slewPlot
from teenastro import TeenAstro, deg2dms

sampleFreq = 2
earth = 399 # NEAF code for the earth center of mass

sgCommTypeSerial = [sg.Radio('Serial', "RADIO1", size=(8, 1), enable_events=True, key='-Serial-'),
          sg.Text('Device:', size=(10, 1)), sg.Combo('ComPorts', [],  key='-ComPorts-', size=(20, 1), disabled=True),
          sg.Text('Baud Rate:', size=(10, 1)), sg.Combo(['9600','19200','57600','115200'], default_value='115200',key='-BaudRate-', size=(20, 1)),]

sgCommTypeTCP = [sg.Radio('TCP', "RADIO1", default = True, size=(8, 1), enable_events=True, key='-tcp-'),
          sg.Text('IP Address:', size=(10, 1)),
          sg.Input('192.168.0.21', key='-IPADDR-', size=(20, 1))]

commFrame = sg.Frame('Comm Port',[sgCommTypeSerial,sgCommTypeTCP,[sg.Button('Connect', key='connect')]])

topRow = [commFrame, sg.B('Exit')]

coordFrame = sg.Frame('Coordinates',[[sg.T(' ', size=10), sg.T('Displayed', size=10),sg.T('Computed', size=10)],
                                     [sg.T('Azimuth', size=10), sg.T('0', size=10, key='az_disp'),sg.T('0', size=10, key='az_comp')],
                                     [sg.T('Altitude', size=10), sg.T('0', size=10, key='alt_disp'),sg.T('0', size=10, key='alt_comp')]])

driftFrame = sg.Frame('Drift Rates (arc-sec/S)', [[sg.T('RA:'), sg.T('0', key='ra_rate')],[sg.T('Dec:'), sg.T('0', key='dec_rate')]]) 

axisFrame = sg.Frame('Axis Positions (º)', [[sg.T('Axis1:'), sg.T('0', key='axis1_degrees')],[sg.T('Axis2:'), sg.T('0', key='axis2_degrees')]]) 

stepsFrame = sg.Frame('Axis Positions (steps)', [[sg.T('Axis1:'), sg.T('0', key='axis1_steps')],[sg.T('Axis2:'), sg.T('0', key='axis2_steps')]]) 

speedFrame = sg.Frame('Axis Speeds (x Sidereal)', [[sg.T('Axis1:'), sg.T('0', key='axis1_speed')],[sg.T('Axis2:'), sg.T('0', key='axis2_speed')]]) 

slewFrame = sg.Frame('Slew Rates (arc-sec/S)', [[sg.T('RA:'), sg.T('0', key='ra_rate')],[sg.T('Dec:'), sg.T('0', key='dec_rate')]]) 

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
                    [sg.B(button_text = 'Start Tracking', key='startStopTrack'),sg.B(button_text = 'Clear', key='clearDrift'),
                     sg.B(button_text = 'Save', key='saveDrift'), sg.B(button_text = '+',key='zoomInD'),sg.B(button_text = '-',key='zoomOutD'),
                     sg.B(button_text = 'GuideN', key='GuideN'), sg.B(button_text = 'GuideS',key='GuideS'),sg.B(button_text = 'GuideE',key='GuideE'),sg.B(button_text = 'GuideW',key='GuideW')],
                    [sg.Canvas(key='drift_cv', size=(640, 400))]]), sg.Column([[driftFrame], [axisFrame], [stepsFrame], [speedFrame]])]
                ]

slewTestTab = [[sg.Column([
                    [sg.B(button_text = 'Home', key='slewHome'),sg.B(button_text = 'North', key='slewNorth'),sg.B(button_text = 'West', key='slewWest'),
                     sg.B(button_text = 'East', key='slewEast'),sg.B(button_text = 'South', key='slewSouth'),sg.B(button_text = 'Zenith', key='slewZenith'),
                     sg.B(button_text = 'Clear', key='clearSlew'),
                     sg.B(button_text = 'Save', key='saveSlew')],
                    [sg.Canvas(key='slew_cv', size=(640, 400))]])]
                ]

alignmentTab = [[sg.Column([
                    [sg.B(button_text = 'Start', key='startStopAlignment'),
                      sg.B(button_text = '+',key='zoomInA'),sg.B(button_text = '-',key='zoomOutA'),
                      sg.DropDown([], key='alignmentTarget', size=25),
                      sg.B(button_text='Goto', key='alignmentGoto')],
                    [sg.Canvas(key='alignment_cv', size=(640, 400))]]), sg.Column([[homeErrorFrame], [poleErrorFrame]])]
                ]


#bottomRow = sg.Output(key='Log',  size=(80, 12))
bottomRow = []

layout = [ [topRow],
          [sg.TabGroup([[
            sg.Tab('Pointing Test', pointTestTab, key='ptest'),
            sg.Tab('Drift Test', driftTestTab, key='dtest'),
            sg.Tab('Slew Test', slewTestTab, key='stest'),
            sg.Tab('Alignment Test', alignmentTab, key='atest')]],
            key='tgroup')],
          [bottomRow]
         ]




# Utility functions

def eqAxesToHaDec(axis1, axis2, pierSide):
    if (pierSide == 'E'):
        ha = axis1 / 15
        dec = axis2
    else:
        ha = (axis1+180) / 15
        dec = 180-axis2
    return ha, dec






class alignmentPlot():
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
        self.fig, self.ax = plt.subplots(figsize=(6, 6), dpi=self.dpi)
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
            self.ta.gotoRaDec(ra,dec)

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
        ha, dec = eqAxesToHaDec(axis1, axis2, pierSide)
        ra = lst - ha                      # in hours
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
        self.mountType = self.ta.readMountType()
        if not self.mountType in ['E', 'K']:
            self.log('not yet implemented for Alt Az mounts')
            return
        self.lat = self.ta.getLatitude()
        self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
        self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
        ra, dec, lst, ha = self.getAxisCoords()
        self.project(ra, dec)
        self.scatter = self.ax.scatter(self.stars['x'], self.stars['y'],s=self.marker_size, color='k')
        self.state = 'RUN'


# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-t', '--portType', help='TeenAstro connection type (tcp or serial)')
  parser.add_argument('-p', '--portName', help='TeenAstro IP address or serial port')
  parser.add_argument('-b', '--baudRate', help='TeenAstro baud rate')
  opts = parser.parse_args(args)
  return opts


# Main program
class Application:

    def __init__(self, options):
        self.ts = load.timescale()

        self.window = sg.Window('TeenAstro AutoTest', layout, finalize=True, size=(1024,720))
        if options.portType == 'tcp':
            self.window['-tcp-'].update(value = True)
            self.window['-IPADDR-'].update(disabled = False)
            self.window['-IPADDR-'].update(options.portName)
            self.window['-IPADDR-'].SetFocus()
            self.window['-ComPorts-'].update(disabled = True)
        elif options.portType == 'serial':
            self.window['-Serial-'].update(value = True)
            self.window['-ComPorts-'].update(disabled = False)
            self.window['-ComPorts-'].SetFocus()
            self.window['-ComPorts-'].update(options.portName)
            self.window['-BaudRate-'].update(options.baudRate)
            self.window['-IPADDR-'].update(disabled = True)

        self.ta = TeenAstro(portType=options.portType, portName=options.portName, baudRate=options.baudRate)

        self.dp = driftPlot(self.window, self.ts, self.ta)
        self.pp = pointingPlot(self.window, self.ts, self.ta)
        self.ap = alignmentPlot(self.window, self.ts, self.ta)
        self.sp = slewPlot(self.window, self.ts, self.ta)
        self.run()

    def run(self):
        while True:
            event, values = self.window.read(1000 / sampleFreq)

            if event in (sg.WIN_CLOSED, 'Exit'): 
                break
             
            if event == '-Serial-':
              print('Serial selected')
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
              print('TCP selected')
              self.window['-ComPorts-'].update(disabled = True)
              self.window['-IPADDR-'].SetFocus()
              self.window['-IPADDR-'].update(disabled = False)
              self.ta.portType = 'tcp'
  
            if event == 'connect':
                if self.ta.isConnected():
                    self.ta.close()
                    print ('Disconnected')
                    self.window['connect'].update('Connect')
                else:
                    if self.ta.portType == 'tcp':
                        self.ta = TeenAstro(portType='tcp', portName=values['-IPADDR-'], baudRate=values['-BaudRate-'])
                    else:
                        self.ta = TeenAstro(portType='serial', portName=values['-ComPorts-'], baudRate=values['-BaudRate-'])
                    p = self.ta.open()
                    if (p != None):
                        print ('connected')
                        self.window['connect'].update('Disconnect')
                        self.dp.ta = self.ta
                        self.pp.ta = self.ta
                        self.ap.ta = self.ta
                        self.sp.ta = self.ta
                    else:
                        print ('Error opening port')

            t = self.window['tgroup'].get()
            if t == 'dtest':
                self.dp.handleEvent(event, values, self.window)
            elif t == 'ptest':
                self.pp.handleEvent(event, values, self.window)
            elif t == 'atest':
                self.ap.handleEvent(event, values, self.window)
            elif t == 'stest':
                self.sp.handleEvent(event, values, self.window)
                
        self.window.close()


    def log(self, message):
        print (message)



if __name__ == '__main__':
    options = readOptions()
    Application(options)



