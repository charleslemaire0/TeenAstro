#!/usr/bin/env python3
#
# Load and save TeenAstro configuration
#
# (C) 2020, François Desvallées

import PySimpleGUI as sg
import platform, re, json, sys, math
from telnetlib import Telnet
import serial, time
import serial.tools.list_ports
from threading import Timer

# Global variables



sgCommTypeSerial = [sg.Radio('Serial', "RADIO1", size=(8, 1), enable_events=True, key='-Serial-'),
          sg.Text('Device:', size=(10, 1)),
          sg.Combo('ComPorts', [], key='-ComPorts-', size=(20, 1))]

sgCommTypeTCP = [sg.Radio('TCP', "RADIO1", default = True, size=(8, 1), enable_events=True, key='-TCPIP-'),
          sg.Text('IP Address:', size=(10, 1)),
          sg.Input('192.168.0.12', key='-IPADDR-', size=(20, 1))]
  


def openPort():
  if window['-Serial-'].Get():
    try:
      comm = serial.Serial(port=values['-ComPorts-'],
                        baudrate=57600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE,
                        timeout=None, xonxoff=False, rtscts=False, write_timeout=None, dsrdtr=False,
                        inter_byte_timeout=None)
      logText('Opening port '+ values['-ComPorts-'])
      return comm
    except:
      logText('Error opening port')
      return None
      
  else:
    try:
      comm = Telnet(values['-IPADDR-'], '9999')          # 9999 is the hard-coded IP port of TeenAstro
      logText('Opening port '+ values['-IPADDR-'] + ' 9999')
      return comm
    except:
      logText('Error opening port')
      return None


def getValue(cmdStr):
    comm.write(cmdStr.encode('utf-8'))
    # Read response byte to allow some time before the next command
    resp = (comm.read_until(b'#', 100)).decode('utf-8')
    return resp

def sendCommand(cmdStr):
    print (cmdStr)
    comm.write(cmdStr.encode('utf-8'))
    return


def logText(t):
  print (t)


def readGears():
  if (comm != None):
    print('readGears()')
    global axis1Gear, axis2Gear
    gear1 = getValue(':%GR#').strip('#')
    steps1 = getValue(':%SR#').strip('#')
    axis1Gear = int(gear1) * int(steps1)
    window['axis1Gear'].update("Gear1:%d" % axis1Gear)
    gear2 = getValue(':%GD#').strip('#')
    steps2 = getValue(':%SD#').strip('#')
    axis2Gear = int(gear2) * int(steps2)
    window['axis2Gear'].update("Gear2:%d" % axis2Gear)




def readStatus():
  if (comm != None):
    try:
      res = getValue(':GU#')
      if  (res[0] == '0'):
        window['status'].update('Status : stopped')
      elif  (res[0] == '2'):
        window['status'].update('Status : slewing')
      if (res[13]) == 'E':
        window['pierSide'].update('Pier Side : East')
      else:  
        window['pierSide'].update('Pier Side : West')
    except:
      print ("Error reading status")    


def readAxis1():
  if (comm != None):
    try:
      initSteps1 = axis1Gear * 4
      steps = float(getValue(':GXF8#').strip('#'))
      degrees =  (90.0 / 4) * (steps - initSteps1) / axis1Gear   
      window['axis1Steps'].update('Steps: %d' % steps)
      window['axis1Degrees'].update('Degrees: %3.2f' % degrees)
    except:
      print ("Error reading Axis1")

def readAxis2():
  if (comm != None):
    try:
      initSteps2 = axis2Gear * 4
      steps = float(getValue(':GXF9#').strip('#'))
      degrees =  (90.0 / 4) * (steps - initSteps2) / axis2Gear
      window['axis2Steps'].update('Steps: %d' % steps)
      window['axis2Degrees'].update('Degrees: %3.2f' % degrees)
    except:
      print ("Error reading Axis2")


def goHome():
  sendCommand(":hC#")

def flipMount():
  sendCommand(":MF#")


def gotoCoordinate(dir):
  print (dir)
  if (comm == None):
    return
  if (dir == "North"):
    az =  0
    alt = 0
  elif (dir == "South"):
    az = 180
    alt = 0
  elif (dir == "West"):
    az = 270
    alt = 0
  elif (dir == "East"):
    az = 90
    alt = 0

  sendCommand(":Sz%03u*00:00#" % az)
  sendCommand(":Sa+%02u*00:00#" % alt)
  sendCommand(":MA#")



# Main program
comm = None



sg.SetOptions(
       input_elements_background_color='#F7F3EC',
       progress_meter_color = ('green', 'blue'),
       button_color=('black','lightgray'))

axis1Frame = sg.Frame('Axis 1', [[sg.Text('Gear:' , size = (16,1), key='axis1Gear')],
                                 [sg.Text('Steps', size = (16,1), key='axis1Steps')],
                                 [sg.Text('Degrees', size = (16,1), key='axis1Degrees')]])

axis2Frame = sg.Frame('Axis 2', [[sg.Text('Gear:', size = (16,1), key='axis2Gear')],
                                 [sg.Text('Steps', size = (16,1), key='axis2Steps')],
                                 [sg.Text('Degrees', size = (16,1), key='axis2Degrees')]])

moveRow = sg.Column([[sg.Button('Goto South'), sg.Button('Goto North'),
               sg.Button('Goto East'), sg.Button('Goto West')]])

homeRow = sg.Column([[sg.Button('Home'), sg.Button('Flip')]])
axisRow = sg.Column([[axis1Frame, axis2Frame]])

topRow = sg.Column([[sg.Text('IP Address:'), sg.Input('192.168.0.12', key='IPAddr' )]])
commRow = sg.Column([[sg.Button('Open'), sg.Button('Close')]])
statusRow = sg.Column([[sg.Text('Status:', key='status', size=(20,1))], [sg.Text('Pier Side:', key='pierSide', size=(20,1))]])
bottomRow = sg.Column([[sg.Output(key='Log',  size=(80, 10))]])


layout = [sgCommTypeSerial,
          sgCommTypeTCP,
          [commRow],
          [axisRow],
          [moveRow],
          [homeRow],
          [statusRow],
          [bottomRow]
         ]

window = sg.Window('TATest 1.0', layout)


while True:
  event, values = window.Read(10)

  if event == sg.TIMEOUT_KEY:
    if (comm != None):
      readStatus()    
      readAxis1()
      readAxis2()

  elif event == 'Open':
    comm = openPort()
    if (comm != None):
      readGears()
      print('Connected')
    else:
      print('Error opening port')


  elif event == 'Close':
    comm = None
    print('Connection closed')

  elif event == 'Goto North':
    gotoCoordinate('North')

  elif event == 'Goto South':
    gotoCoordinate('South')

  elif event == 'Goto East':
    gotoCoordinate('East')

  elif event == 'Goto West':
    gotoCoordinate('West')

  elif event == 'Home':
    goHome()

  elif event == 'Flip':
    flipMount()

  elif event != None:                 # handle changes on spin boxes
    print(event)
    tag = event
    value = window[event].Get()

    if tag == '-Serial-':
      print('Serial selected')
      window['-ComPorts-'].SetFocus()
      # collect available serial ports on the system
      serPortList = serial.tools.list_ports.comports()
      serPortsDetected = []
      for port in serPortList:
        serPortsDetected.append(port.device)
      window['-ComPorts-'].update(values = sorted(serPortsDetected))

    elif tag == '-TCPIP-':
      print('TCP/IP selected')
      window['-IPADDR-'].SetFocus()

  else:
    print (event)
    break


