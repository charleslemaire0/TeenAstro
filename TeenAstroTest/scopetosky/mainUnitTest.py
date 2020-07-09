import sys, json, csv, argparse, math
from ruamel.yaml import YAML 
from telnetlib import Telnet
from teenastro import TeenAstro, dms2deg, deg2dms

import serial, time, datetime

yaml = YAML()


# Open TCP port to scopetosky (Javascript program running in background)
def openScopeToSkyPort():
#  try:
    comm = Telnet('localhost', '7070')
    return comm
#  except:
#    return None

# Send a command represented as a JSON object to scopeToSky.js and return the result
def sendCommand(p, cmd):
  try:
    p.write(json.dumps(cmd).encode('utf-8'))
    resp =  (p.read_until(b'\n', 100)).decode('utf-8')[:-1]    
  except:
    print ('Error sending command')
    resp = {};  
  return (resp)

# convert 'True' or 'False' strings to boolean values, or return string unchanged
# also strip leading and trailing whitespace
def str2bool(s):
  s = s.strip()
  true = ['true',  '1', 'yes']
  false = ['false', '0', 'no']
  if s.lower() in true:
    return True
  elif s.lower() in false:
    return False
  return s

# Print a list as a CSV line
def printCsv(l):
  s = ''
  for e in l:
    s = s + str(e) + ','
  print (s)

# Extract a list of fields from the JSON object output by scopeToSky.js
def printResult(resp, fields):
  data = json.loads(resp)
  pFields = []
  for f in fields:
    value = f.strip()    
    pFields.append(data[value])
  printCsv (pFields)

def taTest(ta,ra,dec):
  res = ta.gotoRaDec(ra,dec)
  if (res != 'ok'):
#    print (res)
    return None

  while ta.isSlewing():
    time.sleep(1)
  return (ta.getPierSide(),ta.getAxis1(), ta.getAxis2(), ta.getAzimuth(), ta.getAltitude())


def printLabels():
  print ("RA, Dec, computedAxis1, computedAxis2, azimuth, altitude, latitude, longitude, JD, SidT, pierSide, actualAxis1, actualAxis2, delta1, delta2,")


# Perform a list of test cases and print the result
def doTestCases(p, ta, cfgFile, testFile):
  f = open(cfgFile, 'r')   # static configuration 
  cmd = yaml.load(f.read())

  # set the site from config file into TeenAstro
  ta.setLatitude(float(cmd['latitude']))
  ta.setLongitude(-float(cmd['longitude'])) # scopetosky counts positive longitudes east of GMT
  ta.setTimeZone(float(cmd['timeZone']))

  with open(testFile, mode='r') as csv_file:       # list of test cases
    testCases = csv.DictReader(csv_file,delimiter=';')
    printLabels()
    for row in testCases:
      # get axis positions from TeenAstro
      try:
        (pierSide,axis1, axis2, az, alt) = taTest(ta, dms2deg (row['RAHA']),dms2deg (row['dec']))
      except:
        continue
      if (axis1 < 0):
        axis1 = axis1 + 360
      taSidT = ta.readSidTime()

      # get axis positions from ScopeToSky
      cmd['RAHA'] = row['RAHA'] 
      cmd['dec'] = row['dec'] 
      if (pierSide == 'E'):
        cmd['flipped'] = True
      else:
        cmd['flipped'] = False
      resp = json.loads(sendCommand (p,cmd))
      delta1 = 3600 * (axis1 - float (resp['primaryAxis']))
      delta2 = 3600 * (axis2 - float (resp['secondaryAxis']))

      # no point in comparing below 10 degrees
      if (float(resp['altitude']) > 10.0):
        print ("%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %c, %3.4f, %3.4f, %4.0f, %4.0f, " % (row['RAHA'], row['dec'], resp['primaryAxis'],resp['secondaryAxis'], resp['azimuth'], resp['altitude'], resp['latitude'], resp['longitude'], resp['JD'], resp['SidT'], pierSide, axis1, axis2, delta1, delta2))


# Run the default automatic test cases (series of RA circles at constant declination, from 80 degrees to -20 degrees) 
def runAutoTests(p, ta, cfgFile):
  f = open(cfgFile, 'r')   # static configuration 
  
  cmd = yaml.load(f.read())

  # set the site from config file into TeenAstro
  ta.setLatitude(float(cmd['latitude']))
  ta.setLongitude(-float(cmd['longitude'])) # scopetosky counts positive longitudes east of GMT
  ta.setTimeZone(float(cmd['timeZone']))

  printLabels()
  for dec in range(80, -40, -20):
      for ra in range (0, 24, 2):  
        try:
          result = taTest(ta, ra, dec)
          (pierSide,axis1, axis2, az, alt) = result
        except:
          continue
        if (axis1 < 0):
          axis1 = axis1 + 360
        taSidT = ta.readSidTime()
  
        # get axis positions from ScopeToSky
        cmd['RAHA'] = '%02d:00:00' % ra 
        cmd['dec'] = '%02d:00:00' % dec 
        if (pierSide == 'E'):
          cmd['flipped'] = True
        else:
          cmd['flipped'] = False
        resp = json.loads(sendCommand (p,cmd))
        delta1 = 3600 * (axis1 - float (resp['primaryAxis']))
        delta2 = 3600 * (axis2 - float (resp['secondaryAxis']))
  
        # no point in comparing below 10 degrees
        if (float(resp['altitude']) > 10.0):
          print ("%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %3.4f, %3.4f, %4.0f, %4.0f, " % (ra, dec, resp['primaryAxis'],resp['secondaryAxis'], resp['azimuth'], resp['altitude'], resp['latitude'], resp['longitude'], resp['JD'], resp['SidT'], pierSide, axis1, axis2, delta1, delta2))

    
# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-c', '--config', help='static configuration file - default is config.yaml')
  parser.add_argument('-t', '--testcase', help='list of test cases- default is to runAutoTests')
  parser.add_argument('-p', '--porttype', help='TeenAstro Comm port: tcp or serial')
  opts = parser.parse_args(args)
  if opts.config == None:
    opts.config = 'config.yml'
  if opts.testcase == None:
    opts.testcase = 'auto'
  if opts.porttype == None:
    opts.porttype = 'serial'
  return opts




# Main program
# Call the function to read the argument values
options = readOptions()

p = openScopeToSkyPort()
if (p == None):
  print ('Error connecting to scopetosky')
  sys.exit()

# change default types as needed 
if (options.porttype == 'serial'):
  ta = TeenAstro('serial', '/dev/ttyACM0')
else:
  ta = TeenAstro('tcp', '192.168.0.12')

p2 = ta.open()

if (p2 == None):
  print ('Error connecting to TeenAstro')
  sys.exit()


today = datetime.date.today()
now = datetime.datetime.now()

ta.setDate(today)
ta.setLocalTime(now)

#ta.readSite()
ta.readDateTime()
ta.readSidTime()
ta.readGears()

if (options.testcase == 'auto'):
  runAutoTests(p, ta, options.config)    
else:
  doTestCases(p, ta, options.config, options.testcase);

ta.disableTracking() 

