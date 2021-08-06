#!/usr/bin/env python3
# -*- coding: UTF-8 -*-#
# Load and save TeenAstro configuration
#
# (C) 2020, François Desvallées
# (C) 2020, Lukas Zimmermann, Basel

import PySimpleGUI as sg
import platform, re, json, sys, math, time
from telnetlib import Telnet
import serial
import serial.tools.list_ports


# Global variables
Filename = 'TAConfig.json'
portType = None

SiteDef = {
      'NS':['North','South'],'latDeg':[i for i in range(0,90)], 'latMin':[i for i in range(0,60)], 
      'EW':['East','West'],'longDeg':[i for i in range(0,180)], 'longMin':[i for i in range(0,60)],
      'timeZone':[i/10 for i in range(-110,120,5)], 'elevation':[i for i in range(-200,8000)],
      'siteNum':[i for i in range(0,4)],
      'sitename':'Site 0'  
      }

MountDef = { 'mType':['German', 'Fork', 'Alt Az', 'Alt Az Fork'],
      'DefaultR':['Guide', 'Slow', 'Medium','Fast', 'Max'],
      'MaxR':[i for i in range(32,4000)],'GuideR':[i/100 for i in range(1,100)],'Acc':[i/10 for i in range(1,251)],
      'SlowR':[i for i in range(1,255)],'MediumR':[i for i in range(1,255)],'FastR':[i for i in range(1,255)],
      'mrot1':['Direct','Reverse'],'mge1':[i for i in range(1,60000)],'mst1':[i for i in range(1,400)],'mmu1':[8,16,32,64,128,256],
      'mbl1':[i for i in range(0,999)],'mlc1':[i for i in range(100,2000,10)],'mhc1':[i for i in range(100,2000)], 
      'msil1':[0,1],
      'mrot2':['Direct','Reverse'],'mge2':[i for i in range(1,60000)],'mst2':[i for i in range(1,400)],'mmu2':[8,16,32,64,128,256],
      'mbl2':[i for i in range(0,999)],'mlc2':[i for i in range(100,2000,10)],'mhc2':[i for i in range(100,2000)], 
      'msil2':[0,1],
      'hl':[i for i in range(-30,30)], 'ol':[i for i in range(60,92)], 'el':[i for i in range(-45,45)], 
      'wl':[i for i in range(-45,45)], 'ul':[i for i in range(9,12)],'poleAlign':['True', 'Apparent'], 'corrTrack':['enabled','disabled']
      }

# Commands for getting mount parameters
MountReadCmd = { 
      'mType':'GXI','DefaultR':'GXRD',
      'MaxR':'GXRX','GuideR':'GXR0','Acc':'GXRA', 'SlowR':'GXR1','MediumR':'GXR2','FastR':'GXR3',
      'mrot1':'GXMRR','mge1':'GXMGR','mst1':'GXMSR','mmu1':'GXMMR','mbl1':'GXMBR','mlc1':'GXMcR','mhc1':'GXMCR', 'msil1':'GXMmR',
      'mrot2':'GXMRD','mge2':'GXMGD','mst2':'GXMSD','mmu2':'GXMMD','mbl2':'GXMBD','mlc2':'GXMcD','mhc2':'GXMCD', 'msil2':'GXMmD',
      'hl':'GXLH', 'ol':'GXLO', 'el':'GXLE', 'wl':'GXLW','ul':'GXLU', 'poleAlign':'GXAp', 'corrTrack':'GXI'
      } 
MountSetCmd = {
      'mType':'S!','DefaultR':'SXRD:',
      'MaxR':'SXRX:','GuideR':'SXR0:','Acc':'SXRA:', 'SlowR':'SXR1:','MediumR':'SXR2:','FastR':'SXR3:',
      'mrot1':'SXMRR:','mge1':'SXMGR:','mst1':'SXMSR:','mmu1':'SXMMR:','mbl1':'SXMBR:','mlc1':'SXMCR:','mhc1':'SXMcR:', 'msil1':'SXMmR:',
      'mrot2':'SXMRD:','mge2':'SXMGD:','mst2':'SXMSD:','mmu2':'SXMMD:','mbl2':'SXMBD:','mlc2':'SXMCD:','mhc2':'SXMcD:', 'msil2':'SXMmD:',
      'hl':'SXLH:', 'ol':'SXLO:', 'el':'SXLE:', 'wl':'SXLW:','ul':'SXLU:', 'poleAlign':'SXAp:', 'corrTrack':'T'
      } 




# Save all fields from the GUI form into a JSON file
def saveFile(Filename):
  logText("saving to file")
  f = open(Filename,'w')   
  logText('Saving mount data to '+ Filename)
  fileObj = [Mount, [sites[0].serialize(),sites[1].serialize(),sites[2].serialize(),sites[3].serialize()]]
  f.write(json.dumps(fileObj,indent=4)) 
  f.close() 
  logText('done')


# Load all fields from the JSON file into the GUI 
def loadFile(Filename):
  f = open(Filename,'r')   
  logText('Loading mount data from '+ Filename)
  fileObj = json.load(f)
  Mount = fileObj[0]
  for i in range(0,4):
    sites[i].unserialize(fileObj[1][i]) 
  f.close() 
  return (Mount, sites)


def sgSpin(tag, width=5):
  r = MountDef[tag]
  return ( sg.Spin(values=r, initial_value=r[0], key=tag,size=(width,1), enable_events=True))

def siteSpin(tag, width=5):
  r = SiteDef[tag]
  return ( sg.Spin(values=r, initial_value=r[0], key=tag,size=(width,1), enable_events=True))


def sgLabel(text):
  return sg.Text(text, size=(20,1))


def openPort():
  if window['-Serial-'].Get():
    try:
      logText('Opening port '+ values['-ComPorts-'])
      comm = serial.Serial(port=values['-ComPorts-'],
                        baudrate=57600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE,
                        timeout=None, xonxoff=False, rtscts=False, write_timeout=None, dsrdtr=False,
                        inter_byte_timeout=None)
      global portType
      logText('Connected')
      portType = 'serial'
      return comm
    except:
      logText('Error opening port')
      return None
      
  else:
    try:
      logText('Opening port '+ values['-IPADDR-'] + ' 9999')
      comm = Telnet(values['-IPADDR-'], '9999')          # 9999 is the hard-coded IP port of TeenAstro
      portType = 'telnet'
      logText('Connected')
      return comm
    except:
      logText('Error opening port')
      return None

def getValue(comm, cmd):
  cmdStr = ":" + cmd + "#"  
  comm.write(cmdStr.encode('utf-8'))
  try:
    val = comm.read_until(b'#', 100).decode('utf-8')[:-1]
  except:  
    logText("getValue Error: %s" % cmd)
    val = '?'
  return (val)

def sendCommand(comm, cmdStr):
  global portType
  comm.write(cmdStr.encode('utf-8'))
  # handle differences between serial and telnet
  if portType == 'serial':
    resp =  (comm.read(1)).decode('utf-8')  
  else:
    resp =  (comm.read_some()).decode('utf-8')  
  print (cmdStr, resp)

def setMountType():
  if (comm == None):
    return
  cmdStr = ':' + MountSetCmd['mType']
  if (Mount['mType'] == 'German'):
    cmdStr +=  '0#'
  elif (Mount['mType'] == 'Fork'):
    cmdStr +=  '1#'
  elif (Mount['mType'] == 'Alt Az'):
    cmdStr +=  '2#'
  elif (Mount['mType'] == 'Alt Az Fork'):
    cmdStr +=  '3#'
  comm.write(cmdStr.encode('utf-8'))
   


def writeMountData():
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

    elif ((tag == 'msil1') or (tag == 'msil2')):
      cmdStr += Mount[tag]

    elif ((tag == 'mmu1') or (tag == 'mmu2')):
      # Microsteps
      # Microsteps are coded as the exponent of 2
      cmdStr += str(int(math.log(int(Mount[tag]), 2)))

    elif ((tag == 'el') or (tag == 'wl')):    # EL / WL limits are stored in quarters of a degree
      cmdStr += str(int(int(Mount[tag]) * 4))

    elif ((tag == 'hl') or (tag == 'ol')):          # horizon and overhead limits 
      cmdStr += str(int(Mount[tag]))

    elif tag == 'ul':                               # under pole limit
      cmdStr += str(int(float(Mount[tag]) * 10))

    elif tag == 'Acc':
      cmdStr += str(int(float(Mount[tag]) * 10))

    elif tag == 'DefaultR':                         # default rate: 0 to 4
      if (Mount[tag] == 'Guide'):
        index = 0
      elif (Mount[tag] == 'Slow'):
        index = 1
      elif (Mount[tag] == 'Medium'):
        index = 2
      elif (Mount[tag] == 'Fast'):
        index = 3
      elif (Mount[tag] == 'Max'):
        index = 4
      cmdStr += '%d' % index

    elif tag == 'GuideR':                           # guiding rate
      cmdStr += str(int(float(Mount[tag]) * 100))

    elif (tag == 'SlowR') or (tag == 'MediumR') or (tag == 'FastR'):
      cmdStr += str(int(Mount[tag]))

    elif (tag == 'poleAlign'):
      if (Mount[tag] == 'True'):
        cmdStr += 't'
      else:
        cmdStr += 'a'

    elif (tag == 'corrTrack'):
      if (Mount[tag] == 'enabled'):
        cmdStr += 'c'
      else:
        cmdStr += 'n'

    else:                         # all other cases
      cmdStr += str(Mount[tag])

    cmdStr += "#"
#    print("Tag: %s,  command: %s" % (tag, cmdStr))

    sendCommand(comm, cmdStr)





# splits Meade formats sDD*MM’SS and HH:MM:SS to lists
def dmsSplit(dms):
  dms = dms.replace('*',' ').replace("'",' ').replace(':',' ')
  try:
    d_m_s = dms.split()
    return list(map(lambda x: int(x), d_m_s))
  except:
    d_m = dms.split()
    return list(map(lambda x: int(x), d_m))

def logText(t):
  print (t)


# Set the mount values into the app controls
def updateView():
  for tag in list(Mount.keys()): 
    window[tag].Update(value = Mount[tag])



# Open connection, read values from TeenAstro
def readVersions():
  if (comm == None):
    return
  window['mainUnitVersion'].update(getValue(comm, 'GVN')) 
  window['boardVersion'].update(getValue(comm, 'GVB')) 
  driverVersion = getValue(comm, 'GVb')
  if driverVersion == '1': driverText = 'TOS100'
  elif driverVersion == '2': driverText = 'TMC2130'
  elif driverVersion == '3': driverText = 'TMC5160'
  else: driverText = 'unknown'
  window['driverVersion'].update(driverText) 
    
def clearVersions():
  window['mainUnitVersion'].update('') 
  window['boardVersion'].update('') 
  window['driverVersion'].update('') 


def readMountData():
  if (comm == None):
    return
  for tag in list(MountReadCmd.keys()): 
    resp = getValue(comm, MountReadCmd[tag]) 
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

    elif ((tag == 'msil1') or (tag == 'msil2')):
      Mount[tag] = resp

    elif ((tag == 'mmu1') or (tag == 'mmu2')):  # Microsteps are coded as the exponent of 2
      Mount[tag] = int(math.pow(2, int(resp)))

    elif ((tag == 'el') or (tag == 'wl')):      # EL / WL limits are indicated in quarters of a degree
      Mount[tag] = int(int(resp) / 4)

    elif (tag == 'ul'):      # UL limit is coded in tenth of RA hour 
      Mount[tag] = int(float(resp) / 10)

    elif ((tag == 'hl') or (tag == 'ol')):      # Trim the trailing asterisk of horizon / overhead limits 
      Mount[tag] = resp[:-1]

    elif (tag == 'MaxR'):
      Mount[tag] = int(resp)
#      print (tag, resp)

    elif (tag == 'GuideR'):
      Mount[tag] = float(resp)

    elif (tag == 'DefaultR'):
      if (resp == '0'):
        Mount[tag] = 'Guide'
      elif (resp == '1'):
        Mount[tag] = 'Slow'
      elif (resp == '2'):
        Mount[tag] = 'Medium'
      elif (resp == '3'):
        Mount[tag] = 'Fast'
      elif (resp == '4'):
        Mount[tag] = 'Max'

    elif (tag == 'SlowR') or (tag == 'MediumR') or (tag == 'FastR'):  
      Mount[tag] = int(float(resp))

    elif (tag == 'poleAlign'):
      if (resp == 'a'):
        Mount[tag] = 'Apparent'
      else:
        Mount[tag] = 'True'

    elif (tag == 'corrTrack'):
      ct = resp[10]     # Corrected tracking is byte number 10 in the result string
      if (ct == 'c'):
        Mount[tag] = 'enabled'
      else:  
        Mount[tag] = 'disabled'
    else:
      Mount[tag] = resp



class Site(object):
  def __init__(self):
    self.name = ''
    self.latitude = [0,0]
    self.longitude = [0,0]
    self.elevation = 0
    self.currentSite = 0
    self.timeZone = 0.0

  def read(self, comm, i):
    setCurrentSite(comm, i)
    self.latitude = dmsSplit(getValue(comm, 'Gt'))
    self.longitude = dmsSplit(getValue(comm, 'Gg'))
    self.elevation = int(getValue(comm, 'Ge'))
    self.timeZone = -float(getValue(comm, 'GG'))

    if (i==0):
      self.name = getValue(comm, 'GM')
    elif (i==1):  
      self.name = getValue(comm, 'GN')
    elif (i==2):  
      self.name = getValue(comm, 'GO')
    elif (i==3):  
      self.name = getValue(comm, 'GP')

  def write(self, comm, i):
    setCurrentSite(comm, i)
    sendCommand(comm,':St%+03d*%02d#' % (self.latitude[0], self.latitude[1]))
    sendCommand(comm,':Sg%+04d*%02d#' % (self.longitude[0], self.longitude[1]))
    sendCommand(comm,':Se%+04d#' % self.elevation)
    sendCommand(comm,':SG%+02.1f#' % (-float(self.timeZone)))
    if (i==0):
      sendCommand(comm,':SM%s#' % self.name)
    elif (i==1):  
      sendCommand(comm,':SN%s#' % self.name)
    elif (i==2):  
      sendCommand(comm,':SO%s#' % self.name)
    elif (i==3):  
      sendCommand(comm,':SP%s#' % self.name)

  def setLatitude(self, sign, deg, min):
    if (sign == 'South'):
      self.latitude = [-int(deg),int(min)]
    else:
      self.latitude = [int(deg),int(min)]

  def setLongitude(self, sign, deg, min):
    if (sign == 'East'):
      self.longitude = [-int(deg),int(min)]
    else:
      self.longitude = [int(deg),int(min)]

  def setTimeZone(self, z):
    self.timeZone = float(z)

  def setName(self, name):
    self.name = name  

  def setElevation(self, elev):
    self.elevation = int(elev)  

  # used for saving as JSON
  def serialize(self):
    return (dict(name=self.name,
            latitude=self.latitude, 
            longitude=self.longitude, 
            elevation=self.elevation,
            currentSite=self.currentSite, 
            timeZone=self.timeZone))

  def unserialize(self, obj):
    self.name = obj['name']
    self.latitude = obj['latitude'] 
    self.longitude = obj['longitude'] 
    self.elevation = obj['elevation']
    self.currentSite = obj['currentSite']
    self.timeZone = obj['timeZone']



def setCurrentSite(comm, i):
  cmdStr = ':W%d#'%i                # need non-standard write method for this command (???)
  comm.write(cmdStr.encode('utf-8'))
  time.sleep(0.1)

# update internal values from site tab
def updateCurrentSite():
  currentSiteNum = int(window['siteNum'].Get())
  s = sites[currentSiteNum]   
  s.setLatitude(window['NS'].Get(),window['latDeg'].Get(), window['latMin'].Get())
  s.setLongitude(window['EW'].Get(),window['longDeg'].Get(), window['longMin'].Get())
  s.setName(window['siteName'].Get())
  s.setTimeZone(window['timeZone'].Get())
  s.setElevation(window['elevation'].Get())

# set values into UI "Sites" tab
def updateSiteTab():
  currentSiteNum = int(window['siteNum'].Get())
  s = sites[currentSiteNum] 
  if (s.latitude[0] >= 0):
    window['NS'].update('North')
  else:  
    window['NS'].update('South')
  window['latDeg'].update(abs(s.latitude[0]))
  window['latMin'].update(s.latitude[1])
  if (s.longitude[0] >= 0):
    window['EW'].update('West')
  else:  
    window['EW'].update('East')
  window['longDeg'].update(abs(s.longitude[0]))
  window['longMin'].update(s.longitude[1])
  window['elevation'].update(s.elevation)
  window['timeZone'].update(s.timeZone)
  window['siteName'].update(s.name)


def initSites():
  sites = []
  for i in range(0,4):
    sites.append(Site())
  return (sites)


def readSiteData():
  if (comm == None):
    return
  # remember the current site
  currentSiteNum = int(getValue(comm, 'W?'))
  # read all 4 sites
  for i in range(0,4):
    s = sites[i]
    s.read(comm, i)

  # reset current site and update window 
  setCurrentSite(comm, currentSiteNum)
  updateSiteTab()

def writeSiteData():
  if (comm == None):
    return
  # remember the current site
  currentSiteNum = int(window['siteNum'].Get())
  for i in range(0,4):
    s = sites[i]
    s.write(comm, i)
  # reset current site and update window 
  setCurrentSite(comm, currentSiteNum)


def updateStatus(comm):      
    window['date'].update("Date: %s" % getValue(comm, 'GC'))
    window['localTime'].update("Local Time: %s" % getValue(comm, 'GL'))
    window['sidTime'].update("Sidereal Time: %s" % getValue(comm, 'GS'))
    window['ra'].update("Right Ascension: %s" % getValue(comm, 'GR'))
    window['dec'].update("Declination: %s" % getValue(comm, 'GD'))
    window['az'].update("Azimuth: %s" % getValue(comm, 'GZ'))
    window['alt'].update("Altitude: %s" % getValue(comm, 'GA'))
    window['axis1'].update("Axis 1: %s" % getValue(comm, 'GXDP0'))
    window['axis2'].update("Axis 2: %s" % getValue(comm, 'GXDP1'))
    window['pierside'].update("Pier Side: %c" % getValue(comm, 'GXI')[13])


# Main program

sg.SetOptions(
       input_elements_background_color='#F7F3EC',
       progress_meter_color = ('green', 'blue'),
       button_color=('black','lightgray'))

readWriteRow = sg.Column([[sg.Button('Read from TeenAstro'), sg.Button('Write to TeenAstro'),
               sg.Button('Load from File', pad=((15,5),(5,5))), sg.Button('Save to File')]])

mountTypeRow = sg.Column([[sg.Text('Mount Type'), sgSpin('mType', width=12),
                           sg.Button('Set and reboot')]])

sites = initSites()

# Initialize the default mount parameters
Mount = {}
for tag in list(MountDef.keys()):
  Mount[tag] = (MountDef[tag])[0]

sgCommTypeSerial = [sg.Radio('Serial', "RADIO1", size=(8, 1), enable_events=True, key='-Serial-'),
          sg.Text('Device:', size=(10, 1)),
          sg.Combo('ComPorts', [], key='-ComPorts-', size=(20, 1), disabled=True)]

sgCommTypeTCP = [sg.Radio('TCP', "RADIO1", default = True, size=(8, 1), enable_events=True, key='-TCPIP-'),
          sg.Text('IP Address:', size=(10, 1)),
          sg.Input('192.168.0.108', key='-IPADDR-', size=(20, 1))]
  

speedFrame = sg.Frame('Speeds', 
          [
          [sgLabel('Default Speed'), sgSpin('DefaultR')],
          [sgLabel('Guide Speed'), sgSpin('GuideR')],
          [sgLabel('Slow Speed'), sgSpin('SlowR')],
          [sgLabel('Medium Speed'), sgSpin('MediumR')],
          [sgLabel('Fast Speed'), sgSpin('FastR')],
          [sgLabel('Max Speed'), sgSpin('MaxR')],
          [sgLabel('Acceleration'), sgSpin('Acc')]])

motFrame1 = sg.Frame('RA Motor', [[sgLabel('Rotation'), sgSpin('mrot1', width=8)],
          [sgLabel('Gear'), sgSpin('mge1')],
          [sgLabel('Steps'), sgSpin('mst1')],
          [sgLabel('Microsteps'), sgSpin('mmu1')],
          [sgLabel('Backlash'), sgSpin('mbl1')],
          [sgLabel('Low/High current, Silent'), sgSpin( 'mlc1'), sgSpin('mhc1'), sgSpin('msil1')]])

motFrame2 = sg.Frame('Dec Motor', [[sgLabel('Rotation'), sgSpin('mrot2', width=8)],
          [sgLabel('Gear'), sgSpin('mge2')],
          [sgLabel('Steps'), sgSpin('mst2')],
          [sgLabel('Microsteps'), sgSpin('mmu2')],
          [sgLabel('Backlash'), sgSpin('mbl2')],
          [sgLabel('Low/High current, Silent'), sgSpin( 'mlc2'), sgSpin('mhc2'), sgSpin('msil2')]])

limitFrame = sg.Frame('Limits', 
        [[sgLabel('Horizon'), sgSpin('hl')],
        [sgLabel('Overhead'), sgSpin('ol')],
        [sgLabel('Past Meridian East'), sgSpin('el')],
        [sgLabel('Past Meridian West'), sgSpin('wl')],
        [sgLabel('Under Pole'), sgSpin('ul')]])

alignmentFrame = sg.Frame('Polar Alignment and Tracking', 
        [[sgLabel('True / Apparent Pole'), sgSpin('poleAlign')],
        [sgLabel('Corrected Tracking'), sgSpin('corrTrack')]])

siteFrame = sg.Frame('Sites', 
          [[sgLabel('Site Number'), siteSpin('siteNum')],
          [sgLabel('Site Name'), sg.InputText('Site 0', size=(20,1), key='siteName')],
          [sgLabel('Latitude'), siteSpin('NS'), siteSpin('latDeg'), siteSpin('latMin')],
          [sgLabel('Longitude'), siteSpin('EW'), siteSpin('longDeg'), siteSpin('longMin')],
          [sgLabel('Time Zone'), siteSpin('timeZone')],
          [sgLabel('Elevation'), siteSpin('elevation')]])

timeFrame = sg.Frame('Time', 
          [[sg.Text('Date', key='date',size=(30,1))],
          [sg.Text('Local Time', key='localTime',size=(30,1))],
          [sg.Text('Sidereal Time', key='sidTime',size=(30,1))]])

coordFrame = sg.Frame('Coordinates', 
          [[sg.Text('Right Ascension', key='ra',size=(30,1))],
          [sg.Text('Declination', key='dec',size=(30,1))],
          [sg.Text('Azimuth', key='az',size=(30,1))],
          [sg.Text('Altitude', key='alt',size=(30,1))]])

debugFrame = sg.Frame('Debug', 
          [[sg.Text('Axis 1 count', key='axis1',size=(30,1))],
          [sg.Text('Axis 2 count', key='axis2',size=(30,1))],
          [sg.Text('Pier side', key='pierside',size=(30,1))]],
          )

commFrame = sg.Frame('Comm Port',[sgCommTypeSerial,sgCommTypeTCP,[sg.Button('Connect', key='connect')]])

versionFrame = sg.Frame('Versions',
          [
          [sg.Text('Main Unit:'), sg.Text('', key='mainUnitVersion',size=(30,1))],
          [sg.Text('Board:'), sg.Text('',key='boardVersion',size=(30,1))],
          [sg.Text('Stepper Driver:'), sg.Text('',key='driverVersion',size=(30,1))]]
          )

mountTab = [[mountTypeRow],[speedFrame, sg.Column([[limitFrame], [alignmentFrame]])],[motFrame1, motFrame2]]
siteTab = [[siteFrame]]
statusTab = [[timeFrame],[coordFrame],[debugFrame]]
bottomRow = sg.Output(key='Log',  size=(80, 4))

topRow = [commFrame, versionFrame]

layout = [ topRow,
          [sg.TabGroup([[sg.Tab('Mount', mountTab),
            sg.Tab('Sites', siteTab),
            sg.Tab('Status', statusTab)]])],
          [readWriteRow],
          [bottomRow]
         ]


window = sg.Window('TAConfig 1.3', layout)

comm = None

while True:
  event, values = window.Read(1000)
  if (event =='__TIMEOUT__'):
    if (comm == None):
      continue
    updateStatus(comm)
    continue

  if (event == 'connect'):
    if (comm == None):
      comm = openPort()   
      if (comm != None):
        window['connect'].update('Disconnect')
        window['-ComPorts-'].update(disabled = True)
        window['-Serial-'].update(disabled = True)
        window['-TCPIP-'].update(disabled = True)
        readVersions()

    else:
      comm.close()
      logText('Disconnected')
      clearVersions()
      comm = None
      window['connect'].update('Connect')
      window['-ComPorts-'].update(disabled = False)
      window['-Serial-'].update(disabled = False)
      window['-TCPIP-'].update(disabled = False)

  elif event == 'Read from TeenAstro':
    readMountData()
    readSiteData()
    updateView()

  elif event == 'Write to TeenAstro':
    writeMountData()
    writeSiteData()
  
  elif event == 'Set in TeenAstro':
    setMountType()

  elif event == 'Load from File':
    w = sg.Window('Open File', [[sg.Input(), sg.FileBrowse()], [sg.OK(), sg.Cancel()] ])
    ev, val = w.Read()
    try:
      (Mount, sites) = loadFile(val[0])
    except:
      logText("Error reading %s" % val)
    w.Close();
    updateView()
    updateSiteTab()

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
    value = window[tag].Get()
    if (tag in list(SiteDef.keys())):
      if (tag == 'siteNum'):          # changed site number?
        updateSiteTab()               # update UI
      else:
        updateCurrentSite()           # any change in UI: update internal values
      i = int(window['siteNum'].Get())  

    if tag == '-Serial-':
      print('Serial selected')
      window['-ComPorts-'].SetFocus()
      window['-ComPorts-'].update(disabled = False)
      window['-IPADDR-'].update(disabled = True)
      # collect available serial ports on the system
      serPortList = serial.tools.list_ports.comports()
      serPortsDetected = []
      for port in serPortList:
        serPortsDetected.append(port.device)
      window['-ComPorts-'].update(values = sorted(serPortsDetected))

    elif tag == '-TCPIP-':
      print('TCP/IP selected')
      window['-ComPorts-'].update(disabled = True)
      window['-IPADDR-'].SetFocus()
      window['-IPADDR-'].update(disabled = False)

    else:
      Mount[tag] = value

  elif event == None:
    break


