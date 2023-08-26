# 
# -*- coding: UTF-8 -*-#
# TeenAstro Auto Test
#
# François Desvallées
#

import math, time, sys, argparse, csv, sys
import PySimpleGUI as sg
import numpy as np  
from skyfield.api import wgs84, load, position_of_radec, utc, Star

import serial.tools.list_ports

sys.path.insert(0, '../mountSim')
from slewPlot import slewPlot
from trackingPlot import trackingPlot
from alignmentPlot import alignmentPlot
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

horCoordFrame = sg.Frame('Hor. Coordinates',[[sg.T(' ', size=10), sg.T('Displayed', size=10),sg.T('Computed', size=10)],
                                     [sg.T('Azimuth', size=10), sg.T('0', size=10, key='az_disp'),sg.T('0', size=10, key='az_comp')],
                                     [sg.T('Altitude', size=10), sg.T('0', size=10, key='alt_disp'),sg.T('0', size=10, key='alt_comp')]])

eqCoordFrame = sg.Frame('Eq. Coordinates',[[sg.T(' ', size=10), sg.T('Displayed', size=10),sg.T('Computed', size=10)],
                                     [sg.T('RA', size=10), sg.T('0', size=10, key='ra_disp'),sg.T('0', size=10, key='ra_comp')],
                                     [sg.T('Dec', size=10), sg.T('0', size=10, key='dec_disp'),sg.T('0', size=10, key='dec_comp')]])

errorFrame = sg.Frame('Error',  [[sg.Text('', key='errorCode',size=(30,1))]])

statusFrame = sg.Frame('Status',  [[sg.Text('', key='statusCode',size=(30,1))]])

driftFrame = sg.Frame('Drift Rates (arc-sec/S)', [[sg.T('RA:'), sg.T('0', key='ra_rate')],[sg.T('Dec:'), sg.T('0', key='dec_rate')]]) 

axisFrame = sg.Frame('Axis Positions (º)', [[sg.T('Axis1:'), sg.T('0', key='axis1_degrees')],[sg.T('Axis2:'), sg.T('0', key='axis2_degrees')]]) 

stepsFrame = sg.Frame('Axis Positions (steps)', [[sg.T('Axis1:'), sg.T('0', key='axis1_steps')],[sg.T('Axis2:'), sg.T('0', key='axis2_steps')]]) 

speedFrame = sg.Frame('Axis Speeds ', [[sg.T('Axis1:'), sg.T('0', key='axis1_speed')],[sg.T('Axis2:'), sg.T('0', key='axis2_speed')]]) 

slewFrame = sg.Frame('Slew Rates (arc-sec/S)', [[sg.T('RA:'), sg.T('0', key='ra_rate')],[sg.T('Dec:'), sg.T('0', key='dec_rate')]]) 

def sgSpin(label):
    return sg.Spin(values = [i for i in range(-100,100,10)], initial_value=0, key=label, background_color='white', size=6)

homeErrorFrame = sg.Frame('Home Errors - arc-minutes',[[sg.T('Axis1', size=6), sgSpin('home_error_axis1')],
                                                       [sg.T('Axis2', size=6), sgSpin('home_error_axis2')]])

poleErrorFrame = sg.Frame('Pole Errors - arc-minutes',[[sg.T('Azimuth', size=6),  sgSpin('pole_error_az')],
                                                       [sg.T('Altitude', size=6), sgSpin('pole_error_alt')]])

centeringFrame = sg.Frame('Centering',[[sg.B(button_text='N', key='alignN'), sg.B(button_text='S', key='alignS'),
                                                        sg.B(button_text='E', key='alignE'),sg.B(button_text='W', key='alignW'), 
                                                        sg.Text('Speed:', size=(6, 1)),
                      sg.Spin(values = ['0', '1', '2', '3', '4'], initial_value=2, key='CenteringSpeed', background_color='white', enable_events=True, size=6)]])


alignmentFrame = sg.Frame('Sync/Alignment',[[sg.B(button_text='Sync', key='alignmentSync'),sg.B(button_text='Sync+Slew', key='syncSlew'),
                                        sg.B(button_text='StartAlign', key='startAlign'),sg.B(button_text='ClearAlign', key='clearAlign'),
                                        sg.B(button_text='Align2', key='align2')]])

slewingCanvasGroup = sg.TabGroup([[sg.Tab('T',  [[sg.Canvas(key='slew_cv_t', size=(640, 400))]])],
                                   [sg.Tab('Polar', [[sg.Canvas(key='slew_cv_polar', size=(640, 400))]])]
                                 ], key='s1group')

slewTestTab = [[sg.Column([
                    [sg.B(button_text = 'Home', key='slewHome'),sg.B(button_text = 'North', key='slewNorth'),sg.B(button_text = 'West', key='slewWest'),
                     sg.B(button_text = 'East', key='slewEast'),sg.B(button_text = 'South', key='slewSouth'),sg.B(button_text = 'Zenith', key='slewZenith'),
                     sg.B(button_text = 'AutoSlew', key='autoSlew'),sg.B(button_text = 'Stop', key='stopSlew'),
                     sg.B(button_text = 'Clear', key='clearSlew'), sg.B(button_text = 'Flip', key='flipMount'), 
                     sg.B(button_text = 'Park', key='park'), sg.B(button_text = 'Unpark', key='unpark')],
                    [slewingCanvasGroup, horCoordFrame]
                ])]]
                    

trackingCanvasGroup = sg.TabGroup([[sg.Tab('T',  [[sg.Canvas(key='tracking_cv_t', size=(640, 400))]])],
                                   [sg.Tab('XY', [[sg.Canvas(key='tracking_cv_xy', size=(640, 400))]])]
                                 ], key='t1group')

trackingTab = [[sg.Column([
                    [sg.B(button_text = 'Start Tracking', key='startStopTrack'),
                      sg.B(button_text = '+',key='zoomInT'),sg.B(button_text = '-',key='zoomOutT'),
                      sg.B(button_text = 'GuideN', key='GuideN'), sg.B(button_text = 'GuideS',key='GuideS'),
                      sg.B(button_text = 'GuideE',key='GuideE'),sg.B(button_text = 'GuideW',key='GuideW'),
                      sg.B(button_text = 'Nudge',key='Nudge'),
                      sg.B(button_text='Spiral', key='spiral'),sg.B(button_text = 'Clear', key='clearTrack')],
                    [trackingCanvasGroup, sg.Column([[driftFrame], [axisFrame], [stepsFrame], [speedFrame]])]
                ])]]

alignmentTab = [[sg.Column([
                    [sg.B(button_text = '+',key='zoomInA'),sg.B(button_text = '-',key='zoomOutA'),
                      sg.DropDown([], key='alignmentTarget', size=25),
                      sg.B(button_text='Goto', key='alignmentGoto'), 
                      centeringFrame, alignmentFrame],
                    [sg.Canvas(key='alignment_cv', size=(640, 400)), 
                     sg.Column([[eqCoordFrame],[homeErrorFrame], [poleErrorFrame]])]
                ])]]


topRow = [commFrame, sg.B('Exit'), sg.Column([[statusFrame], [errorFrame]])]
#bottomRow = sg.Output(key='Log',  size=(80, 12))
bottomRow = []

layout = [ [topRow],
          [sg.TabGroup([[
            sg.Tab('Slewing', slewTestTab, key='stest'),
            sg.Tab('Tracking', trackingTab, key='ttest'),
            sg.Tab('Alignment', alignmentTab, key='atest')  
            ]],
            key='tgroup')],
          [bottomRow]
         ]











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

        self.window = sg.Window('TeenAstro AutoTest', layout, finalize=True, size=(1200,720))
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

        if sys.platform == "darwin":
            dpi = 36
        elif sys.platform == "linux":
            dpi = 64
        else:
            dpi = 64

        self.sp = slewPlot(self.window, self.ts, self.ta, dpi)
        self.tp = trackingPlot(self.window, self.ts, self.ta, dpi)
        self.ap = alignmentPlot(self.window, self.ts, self.ta, dpi)
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
                        self.ap.connect(self.ta) 
                        self.sp.connect(self.ta) 
                        self.tp.connect(self.ta) 
                    else:
                        print ('Error opening port')

            t = self.window['tgroup'].get()
            if t == 'ptest':
                self.pp.handleEvent(event, values, self.window)
            elif t == 'atest':
                self.ap.handleEvent(event, values, self.window)
            elif t == 'stest':
                self.sp.handleEvent(event, values, self.window)
            elif t == 'ttest':
                self.tp.handleEvent(event, values, self.window)

            # update status and errors  
            if (self.ta.isConnected()):      
                status = ''
                if (self.ta.isAtHome()):
                    status = 'HOME '
                elif self.ta.isTracking():
                    status = 'TRACKING '
                elif self.ta.isSlewing():
                    status = 'SLEWING '
                elif self.ta.isParking():
                    status = 'PARKING '
                elif self.ta.isParked():
                    status = 'PARKED '
                self.window['statusCode'].update(status + self.ta.guideStatus())
                self.window['errorCode'].update(self.ta.getErrorCode())
        
        self.window.close()


    def log(self, message):
        print (message)



if __name__ == '__main__':
    options = readOptions()
    Application(options)



