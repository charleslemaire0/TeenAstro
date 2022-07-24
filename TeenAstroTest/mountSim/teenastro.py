#!/usr/bin/env python3
#
# TeenAstro as a Python object
#
# (C) 2020, François Desvallées

import platform, re, json, sys, math
from telnetlib import Telnet
import serial, time, datetime
import serial.tools.list_ports
from threading import Timer

# Helper function
# convert from float to (d,m,s)
def deg2dms(dd):
    negative = dd < 0
    dd = abs(dd)
    minutes,seconds = divmod(dd*3600,60)
    degrees,minutes = divmod(minutes,60)
    if negative:
        if degrees > 0:
            degrees = -degrees
        elif minutes > 0:
            minutes = -minutes
        else:
            seconds = -seconds
    return (degrees,minutes,seconds)

def deg2dm(dd):
  (d,m,s) = deg2dms(dd)
  return (d,m)


# converts from Meade formats sDD*MM’SS and HH:MM:SS
def dms2deg(dms):
  dms = dms.replace('*',' ').replace("'",' ').replace(':',' ')
  try:
    (d,m,s) = dms.split()
    if (float(d) >= 0):
      return float(d) + float(m)/60 + float(s)/3600
    else:
      return float(d) - float(m)/60 - float(s)/3600
  except:
    (d,m) = dms.split()
    if (float(d) >= 0):
      return float(d) + float(m)/60 
    else:
      return float(d) - float(m)/60 


class TeenAstro(object):

  def __init__(self, portType, portName):
    self.portType = portType
    self.portName = portName
    self.port = None
    self.axis1Gear = 0
    self.axis2Gear = 0
    self.dateTime = datetime.datetime(2020,1,1)


  def open(self):
    if self.portType == 'serial':
      try:
        self.port = serial.Serial(port=self.portName,
                          baudrate=57600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                          stopbits=serial.STOPBITS_ONE,
                          timeout=None, xonxoff=False, rtscts=False, write_timeout=None, dsrdtr=False,
                          inter_byte_timeout=None)
        return self.port
      except:
        print('Error opening port')
        return None
        
    else:
      try:
        self.port = Telnet(self.portName, '9999')          # 9999 is the hard-coded IP port of TeenAstro
        return self.port
      except:
        print('Error opening port')
        return None

  def getValue(self, cmdStr):
    self.port.write(cmdStr.encode('utf-8'))
    # Read response byte to allow some time before the next command
    resp = (self.port.read_until(b'#', 100)).decode('utf-8')
    return resp.strip('#')

  def sendCommand(self, cmdStr):
    self.port.write(cmdStr.encode('utf-8'))

    # handle differences between serial and telnet
    if self.portType == 'serial':
      resp =  (self.port.read(1)).decode('utf-8')  
    else:
      resp =  (self.port.read_some()).decode('utf-8')  
        
    return resp

  def readGears(self):
    if (self.port != None):
      self.gear1 = self.getValue(':GXMGR#')
      self.steps1 = self.getValue(':GXMSR#')
      self.axis1Gear = int(self.gear1) * int(self.steps1)
      self.gear2 = self.getValue(':GXMGD#')
      self.steps2 = self.getValue(':GXMSD#')
      self.axis2Gear = int(self.gear2) * int(self.steps2)
      if self.getValue(':GXMRR#') == '1':
        self.axis1Reverse = True
      else:
        self.axis1Reverse = False
      if self.getValue(':GXMRD#') == '1':
        self.axis2Reverse = True
      else:
        self.axis2Reverse = False

  def readSite(self):
    if (self.port != None):
      self.latitude = dms2deg(self.getValue(':Gt#'))
      self.longitude = dms2deg(self.getValue(':Gg#'))
      self.UTCOffset = float(self.getValue(':GG#'))

  def setLatitude(self, latitude):
    if (self.sendCommand(':St%+03d*%02d#' % (deg2dm(latitude))) == '1'):
      self.latitude = latitude
    else:
      print ('Error setting latitude')  
 
  def setLongitude(self, longitude):
    if (self.sendCommand(':Sg%+04d*%02d#' % (deg2dm(longitude))) == '1'):
      self.longitude = longitude
    else:
      print ('Error setting longitude')  

  def setTimeZone(self, timeZone):
    if (self.sendCommand(':SG%+02.1f#' % (-float(timeZone))) == '1'):
      self.timeZone = timeZone
    else:
      print ('Error setting timeZone')  
 
  def setElevation(self, elevation):
    if (self.sendCommand(':Se%+04d#' % elevation) == 1):
      self.elevation = elevation
    else:
      print ('Error setting elevation')  

  def readDateTime(self):
    if (self.port != None):
      self.localTime = datetime.time.fromisoformat(self.getValue(':GL#'))
      (m,d,y) = self.getValue(':GC#').split('/')
      self.currentDate = datetime.date (int(y)+2000, int(m), int(d))
      tz = datetime.timezone(datetime.timedelta(hours=self.getTimeZone()))
      return datetime.datetime (int(y)+2000, int(m), int(d), self.localTime.hour, self.localTime.minute, self.localTime.second, 0, tz)

  def readSidTime(self):
    if (self.port != None):
      self.sidTime = dms2deg(self.getValue(':GS#')) # in decimal hours
      return self.sidTime

  def setLocalTime(self, t):
    if (self.port != None):
      self.sendCommand(":SL%02u:%02u:%02u#" % (t.hour, t.minute, t.second))

  def setDate(self, d):
    if (self.port != None):
      self.sendCommand(":SC%02u/%02u/%02u#" % (d.month, d.day, d.year % 100))

  def readStatus(self):
    if (self.port != None):
      try:
        self.status= self.getValue(':GXI#')
      except:
        print ("Error reading status")    

  def readMountType(self):
    self.readStatus()
    return (self.status[12])

  def isStopped(self):
    self.readStatus()
    return (int(self.status[0]) & 1 == 0)

  def isAtHome(self):
    self.readStatus()
    return (self.status[3] == 'H')

  def getErrorCode(self):
    self.readStatus()
    errorCodes = ['ERR_NONE','ERR_MOTOR_FAULT','ERR_HORIZON','ERR_LIMIT_SENSE','ERR_LIMIT_A1','ERR_LIMIT_A2','ERR_UNDER_POLE','ERR_MERIDIAN','ERR_SYNC'];
    return errorCodes[int(self.status[15])]

  def isTracking(self):
    self.readStatus()
    return (int(self.status[0]) & 1 == 1)

  def isSlewing(self):
    self.readStatus()
    return (int(self.status[0]) & 2 == 2)

  def getPierSide(self):
    self.readStatus()
    return (self.status[13])
    
  def getAxis1(self):
    if (self.port != None):
      try:
        self.axis1Degrees = dms2deg(self.getValue(':GXP1#').strip('#'))
        return self.axis1Degrees 
      except:
        print ("Error reading Axis1")
        return None

  def getAxis2(self):
    if (self.port != None):
      try:
        self.axis2Degrees = dms2deg(self.getValue(':GXP2#').strip('#'))
        return self.axis2Degrees 
      except:
        print ("Error reading Axis2")
        return None

  def goHome(self):
    self.sendCommand(":hC#")

  def flipMount(self):
    self.sendCommand(":MF#")

  def gotoAzAlt(self, az, alt):
    dmsAz = deg2dms(az)
    dmsAlt = deg2dms(alt)
    self.sendCommand(":Sz%03u*%02u:%02u#" % dmsAz)
    self.sendCommand(":Sa%+02d*%02u:%02u#" % dmsAlt)
    self.sendCommand(":MA#")

  def gotoRaDec(self, ra, dec):
    dmsRa = deg2dms(ra)
    dmsDec = deg2dms(dec)
    res1 = self.sendCommand(":Sr%02u:%02u:%02u#" % dmsRa)
    if (res1 != '1'):
      return ("error setting RA")

    res2 = self.sendCommand(":Sd%+03d:%02u:%02u#" % dmsDec)
    if (res2 != '1'):
      return ("error setting Dec")

    res = self.sendCommand(":MS#")
    if (res != '0'):
      return ("gotoRaDec error %s:" % res)
    return ("ok")

  def enableTracking(self):
    self.sendCommand(":Te#")

  def disableTracking(self):
    self.sendCommand(":Td#")

  def enableTrackingCompensation(self):
    self.sendCommand(":Tr#")

  def disableTrackingCompensation(self):
    self.sendCommand(":Tn#")

  def getAltitude(self):
    self.altitude = dms2deg(self.getValue(':GA#'))
    return self.altitude

  def getAzimuth(self):
    self.azimuth = dms2deg(self.getValue(':GZ#'))
    return self.azimuth

  def getRA(self):
    self.RA =  dms2deg(self.getValue(':GR#'))  
    return self.RA

  def getLST(self):
    self.RA =  dms2deg(self.getValue(':GS#'))  
    return self.RA

  def getDeclination(self):
    self.declination = dms2deg(self.getValue(':GD#'))
    return self.declination

  def getLatitude(self):
    self.latitude = dms2deg(self.getValue(':Gt#'))
    return self.latitude

  def getLongitude(self):
    self.longitude = dms2deg(self.getValue(':Gg#'))
    return self.longitude

  def getMeridianEastLimit(self):
    self.meridianEastLimit = int (self.getValue(':GXLE#')) / 4
    return self.meridianEastLimit

  def getMeridianWestLimit(self):
    self.meridianWestLimit = int (self.getValue(':GXLW#')) / 4
    return self.meridianWestLimit

  def getTimeZone(self):
    self.timeZone = -float (self.getValue(':GG#'))
    return self.timeZone



# Main program
def main():
  ta = TeenAstro('serial', '/dev/ttyACM0')
  #ta = TeenAstro('tcp', '192.168.0.12')
  ta.open()

  today = datetime.date.today()
  now = datetime.datetime.now()

  ta.setDate(today)
  ta.setLocalTime(now)

  ta.readSite()
  ta.readDateTime()
  ta.readSidTime()

  print ("latitude:", ta.latitude)
  print ("longitude:", ta.longitude)
  print ("Date:", ta.currentDate)
  print ("Time:", ta.localTime)
  print ("Sidereal time (radians):", math.radians(15 * ta.sidTime))

  '''
  ta.goHome()

  while ta.isSlewing():
    print ('.', end='', flush=True)
    time.sleep(1)
  '''

  ta.readGears()

  ta.gotoRaDec(15,40)

  while ta.isSlewing():
    print ('.', end='', flush=True)
    time.sleep(1)
  print("%03.4f, %03.4f" % (ta.getAxis1(), ta.getAxis2()))

  ta.gotoRaDec(16,40)

  while ta.isSlewing():
    print ('.', end='', flush=True)
    time.sleep(1)

  print("%03.4f, %03.4f" % (ta.getAxis1(), ta.getAxis2()))

  ta.gotoRaDec(17,40)

  while ta.isSlewing():
    print ('.', end='', flush=True)
    time.sleep(1)

  print("%03.4f, %03.4f" % (ta.getAxis1(), ta.getAxis2()))


if __name__ == "__main__":
    main()

