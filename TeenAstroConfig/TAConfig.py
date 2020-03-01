#!/usr/bin/env python3
#
# Load and save TeenAstro configuration
#
# (C) 2020, François Desvallées
# (C) 2020, Lukas Zimmermann, Basel

import PySimpleGUI as sg
import platform, re, json, sys, math
from telnetlib import Telnet
import serial
import serial.tools.list_ports


# Global variables
Filename = 'TAConfig.json'

MountDef = { 'mType':['German', 'Fork', 'Alt Az', 'Alt Az Fork'],
      'MaxR':[i for i in range(32,4000)],'GuideR':[i/100 for i in range(1,100)],'Acc':[i/10 for i in range(1,251)],
      'mrot1':['Direct','Reverse'],'mge1':[i for i in range(1,60000)],'mst1':[i for i in range(1,400)],'mmu1':[8,16,32,64,128,256],
      'mbl1':[i for i in range(0,999)],'mlc1':[i for i in range(100,2000)],'mhc1':[i for i in range(100,2000)], 
      'mrot2':['Direct','Reverse'],'mge2':[i for i in range(1,60000)],'mst2':[i for i in range(1,400)],'mmu2':[8,16,32,64,128,256],
      'mbl2':[i for i in range(0,999)],'mlc2':[i for i in range(100,2000)],'mhc2':[i for i in range(100,2000)], 
      'hl':[i for i in range(-30,30)], 'ol':[i for i in range(60,92)], 'el':[i for i in range(-45,45)], 'wl':[i for i in range(-45,45)]}

# Commands for getting mount parameters
MountReadCmd = { 
      'mType':'GU',
      'MaxR':'GX92','GuideR':'GX90','Acc':'GXE2',
      'mrot1':'%RR','mge1':'%GR','mst1':'%SR','mmu1':'%MR','mbl1':'%BR','mlc1':'%cR','mhc1':'%CR', 
      'mrot2':'%RD','mge2':'%GD','mst2':'%SD','mmu2':'%MD','mbl2':'%BD','mlc2':'%cD','mhc2':'%CD', 
      'hl':'Gh', 'ol':'Go', 'el':'GXE9', 'wl':'GXEA'
      } 
MountSetCmd = {
      'mType':'S!',
      'MaxR':'SX92', 'GuideR':'SX90', 'Acc':'SXE2',
      'mrot1':'$RR', 'mge1':'$GR', 'mst1':'$SR', 'mmu1':'$MR', 'mbl1':'$BR', 'mlc1':'$cR', 'mhc1':'$CR',
      'mrot2':'$RD', 'mge2':'$GD', 'mst2':'$SD', 'mmu2':'$MD', 'mbl2':'$BD', 'mlc2':'$cD', 'mhc2':'$CD',
      'hl':'Sh', 'ol':'So', 'el':'SXE9', 'wl':'SXEA'}


sgCommTypeSerial = [sg.Radio('Serial', "RADIO1", size=(8, 1), enable_events=True, key='-Serial-'),
          sg.Text('Device:', size=(10, 1)),
          sg.Combo('ComPorts', [], key='-ComPorts-', size=(20, 1))]

sgCommTypeTCP = [sg.Radio('TCP', "RADIO1", default = True, size=(8, 1), enable_events=True, key='-TCPIP-'),
          sg.Text('IP Address:', size=(10, 1)),
          sg.Input('192.168.0.12', key='-IPADDR-', size=(20, 1))]
  

# Save all fields from the GUI form into a JSON file
def saveFile(Filename):
  logText("saving to file")
  f = open(Filename,'w')   
  logText('Saving mount data to '+ Filename)
  f.write(json.dumps(Mount)) 
  f.close() 
  logText('done')


# Load all fields from the JSON file into the GUI 
def loadFile(Filename):
  f = open(Filename,'r')   
  logText('Loading mount data from '+ Filename)
  Mount = json.load(f) 
  f.close() 
  return (Mount)


def sgSpin(tag, width=5):
  r = MountDef[tag]
  return ( sg.Spin(values=r, initial_value=r[0], key=tag,size=(width,1), enable_events=True))


def sgLabel(text):
  return sg.Text(text, size=(20,1))


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



def setMountType():
  comm = openPort()
  if (comm == None):
    return
  cmdStr = ":" + MountSetCmd['mType']
  if (Mount['mType'] == 'German'):
    cmdStr +=  '0#'
  elif (Mount['mType'] == 'Fork'):
    cmdStr +=  '1#'
  elif (Mount['mType'] == 'Alt Az'):
    cmdStr +=  '2#'
  elif (Mount['mType'] == 'Alt Az Fork'):
    cmdStr +=  '3#'
  print (cmdStr)
  comm.write(cmdStr.encode('utf-8'))
   


def writeData():
  comm = openPort()
  if (comm == None):
    return

  for tag in list(MountSetCmd.keys()):
    cmdStr = ":" + MountSetCmd[tag]

    if tag == 'mType':
      # Mount type
      # skip this - use special button instead
      continue

    elif ((tag == 'mlc1') or (tag == 'mlc2')
          or (tag == 'mhc1') or (tag == 'mhc2')):
      # motor current high and low settings
      # Current values need to be divided by 10
      cmdStr += str(int(int(Mount[tag]) / 10))
      
    elif ((tag == 'mrot1') or (tag == 'mrot2')):
      # motor rotation direction
      if Mount[tag] == 'Direct':
        cmdStr += '0'
      elif Mount[tag] == 'Reverse':
        cmdStr += '1'
      else:
        continue

    elif ((tag == 'mmu1') or (tag == 'mmu2')):
      # Microsteps
      # Microsteps are coded as the exponent of 2
      print("Microsteps: %s -> %d" % (Mount[tag], int(math.log(int(Mount[tag]), 2))))
      cmdStr += str(int(math.log(int(Mount[tag]), 2)))

    elif ((tag == 'el') or (tag == 'wl')):    # EL / WL limits are stored in quarters of a degree
      cmdStr += ':' + str(int(int(Mount[tag]) * 4))

    elif ((tag == 'hl') or (tag == 'ol')):          # horizon and overhead limits 
      cmdStr += str(int(Mount[tag]))

    elif tag == 'Acc':
      cmdStr += ':' + str(int(float(Mount[tag]) * 10))

    elif tag == 'MaxR':                             # max slewing rate
      cmdStr += ':' + str(Mount[tag])

    elif tag == 'GuideR':                           # guiding rate
      cmdStr += ':' + str(int(float(Mount[tag]) * 100))

    elif ((tag == 'mge1') or (tag == 'mge2')):      # gear reduction
      cmdStr += str(Mount[tag])

    elif ((tag == 'mst1') or (tag == 'mst2')):      # motor steps per revolution
      cmdStr += str(Mount[tag])

    elif ((tag == 'mbl1') or (tag == 'mbl2')):      # backlash
      cmdStr += str(Mount[tag])

    cmdStr += "#"
    print("Tag: %s,  command: %s" % (tag, cmdStr))
    comm.write(cmdStr.encode('utf-8'))
    # Read response byte to allow some time before the next command
    resp = (comm.read_until(b'1', 10)).decode('utf-8')
    print ("response %s" % resp)



def logText(t):
  print (t)


# Set the mount values into the app controls
def updateView():
  for tag in list(Mount.keys()): 
    window[tag].Update(value = Mount[tag])


# Open telnet connection, read values from TeenAstro
def readData():
  comm = openPort()
  if (comm == None):
    return

  for tag in list(MountReadCmd.keys()): 
    cmdStr = ":" + MountReadCmd[tag] + "#"  

    comm.write(cmdStr.encode('utf-8'))
    resp =  (comm.read_until(b'#', 100)).decode('utf-8')[:-1]

    if (tag == 'mType'):
      mt = resp[12]     # Mount type is byte number 12 in the result string
      if (mt == 'E'):
        Mount[tag] = 'German'
      elif (mt == 'K'):
        Mount[tag] = 'Fork'
      elif (mt == 'A'):
        Mount[tag] = 'Alt Az'
      elif (mt == 'k'):
        Mount[tag] = 'Alt Az Fork'   

    elif ((tag == 'mlc1') or (tag == 'mlc2') or (tag == 'mhc1') or (tag == 'mhc2')):   # Current values need multiply by 10    
      Mount[tag] = 10 * int(resp)

    elif ((tag == 'mrot1') or (tag == 'mrot2')):
      if (resp == '0'):
        Mount[tag] = 'Direct'
      else:  
        Mount[tag] = 'Reverse'

    elif ((tag == 'mmu1') or (tag == 'mmu2')):  # Microsteps are coded as the exponent of 2
      Mount[tag] = int(math.pow(2, int(resp)))

    elif ((tag == 'el') or (tag == 'wl')):      # EL / WL limits are indicated in quarters of a degree
      Mount[tag] = int(int(resp) / 4)

    elif ((tag == 'hl') or (tag == 'ol')):      # Trim the trailing asterisk of horizon / overhead limits 
      Mount[tag] = resp[:-1]

    else:
      Mount[tag] = resp
  logText('done')
  comm.close()

# Main program

sg.SetOptions(
       input_elements_background_color='#F7F3EC',
       progress_meter_color = ('green', 'blue'),
       button_color=('black','lightgray'))

readWriteRow = sg.Column([[sg.Button('Read from TeenAstro'), sg.Button('Write to TeenAstro'),
               sg.Button('Load from File', pad=((15,5),(5,5))), sg.Button('Save to File')]])
separatorRow = sg.Column([[sg.Text('_' * 80)]])

mountTypeRow = sg.Column([[sg.Text('Mount Type'), sgSpin('mType', width=12),
                           sg.Button('Set in TeenAstro'),
                           sg.Text('(this reboots TeenAstro)')]])

# Initialize the default mount parameters
Mount = {}
for tag in list(MountDef.keys()):
  Mount[tag] = (MountDef[tag])[0]


topRow = sg.Column([[sg.Text('IP Address:'), sg.Input('192.168.0.12', key='IPAddr' ), sg.Button('Read TeenAstro'),   sg.Button('Write to TeenAstro')],
      [sg.Button('Load'), sg.Button('Save')],
      [sg.Text('Mount Type'), sgSpin('mType', width=12)]])


speedFrame = sg.Frame('Speeds', [[sgLabel('Max Slewing Speed'), sgSpin('MaxR')],
          [sgLabel('Guiding Speed'), sgSpin('GuideR')],
          [sgLabel('Acceleration'), sgSpin('Acc')]])

motFrame1 = sg.Frame('RA Motor', [[sgLabel('Rotation'), sgSpin('mrot1', width=8)],
          [sgLabel('Gear'), sgSpin('mge1')],
          [sgLabel('Steps'), sgSpin('mst1')],
          [sgLabel('Microsteps'), sgSpin('mmu1')],
          [sgLabel('Backlash'), sgSpin('mbl1')],
          [sgLabel('Low / High current'), sgSpin( 'mlc1'), sgSpin('mhc1')]])

motFrame2 = sg.Frame('Dec Motor', [[sgLabel('Rotation'), sgSpin('mrot2', width=8)],
          [sgLabel('Gear'), sgSpin('mge2')],
          [sgLabel('Steps'), sgSpin('mst2')],
          [sgLabel('Microsteps'), sgSpin('mmu2')],
          [sgLabel('Backlash'), sgSpin('mbl2')],
          [sgLabel('Low / High current'), sgSpin( 'mlc2'), sgSpin('mhc2')]])


limitFrame = sg.Frame('Limits', 
        [[sgLabel('Horizon'), sgSpin('hl')],
        [sgLabel('Overhead'), sgSpin('ol')],
        [sgLabel('Past Meridian East'), sgSpin('el')],
          [sgLabel('Past Meridian West'), sgSpin('wl')]]
          )

secondRow = sg.Column([[speedFrame, limitFrame]])
thirdRow = sg.Column([[motFrame1, motFrame2]])
bottomRow = sg.Column([[sg.Output(key='Log',  size=(80, 3))]])

layout = [sgCommTypeSerial,
          sgCommTypeTCP,
          [readWriteRow],
          [separatorRow],
          [mountTypeRow],
          [secondRow],
          [thirdRow],
          [bottomRow]
         ]


window = sg.Window('TAConfig 1.0', layout)


while True:
  event, values = window.Read()

  if event == 'Read from TeenAstro':
    readData()
    updateView()

  elif event == 'Write to TeenAstro':
    writeData()
  
  elif event == 'Set in TeenAstro':
    setMountType()

  elif event == 'Load from File':
    w = sg.Window('Open File', [[sg.Input(), sg.FileBrowse()], [sg.OK(), sg.Cancel()] ])
    ev, val = w.Read()
    try:
      Mount = loadFile(val[0])
    except:
      logText("Error reading %s" % val)
    w.Close();
    updateView()

  elif event == 'Save to File':
    w = sg.Window('Save File', [[sg.Input(), sg.SaveAs(file_types=(("JSON Files", "*.json"),))], [sg.OK(), sg.Cancel()] ])
    ev, val = w.Read()
    try:
      saveFile(val[0])
    except:
      logText(val)
    w.Close()

  elif event != None:                 # handle changes on spin boxes
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
      Mount[tag] = value

  elif event == None:
    break


