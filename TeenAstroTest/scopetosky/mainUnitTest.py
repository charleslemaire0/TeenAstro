import sys, json, csv, argparse, math
from ruamel.yaml import YAML 
from telnetlib import Telnet
from teenastro import TeenAstro, dms2deg, deg2dms

import serial, time, datetime

yaml = YAML()


def openPort():
    try:
      comm = Telnet('localhost', '7070')
#      print('Opening port :'+ 'localhost' + ' 7070')
      return comm
    except:
      print('Error opening port')
      return None

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
  ta.gotoRaDec(ra,dec)

  while ta.isSlewing():
#    print ('.', end='', flush=True)
    time.sleep(1)
#  print("%c, %03.4f, %03.4f" % (ta.getPierSide(),ta.getAxis1(), ta.getAxis2()))
  return (ta.getPierSide(),ta.getAxis1(), ta.getAxis2(), ta.getAzimuth(), ta.getAltitude())


# Perform a list of test cases and print the result
def doTestCases(p, cfgFile, testFile):
  f = open(cfgFile, 'r')   # static configuration 
  cmd = yaml.load(f.read())
  with open(testFile, mode='r') as csv_file:       # list of test cases
    testCases = csv.DictReader(csv_file,delimiter=';')

    print ("RA, Dec, computedAxis1, computedAxis2, pierSide, actualAxis1, actualAxis2, delta1, delta2, deltaAz, deltaAlt")
    for row in testCases:
#      for field in inputFields:   # assume fields are RAHA, dec
      # get axis positions from TeenAstro
      (pierSide,ra, dec, az, alt) = taTest(ta, dms2deg (row['RAHA']),dms2deg (row['dec']))
      if (ra < 0):
        ra = ra + 360
      taSidT = ta.readSidTime()

      # get axis positions from ScopeToSky
      cmd['RAHA'] = row['RAHA'] 
      cmd['dec'] = row['dec'] 
      if (pierSide == 'E'):
        cmd['flipped'] = True
      else:
        cmd['flipped'] = False
      resp = json.loads(sendCommand (p,cmd))
      delta1 = 3600 * (ra - float (resp['primaryAxis']))
      delta2 = 3600 * (dec - float (resp['secondaryAxis']))

#      scSidT = (24.0 * float(resp['SidT'])) / (2*math.pi)
#      deltaTime = 3600 * (taSidT - scSidT) 
      deltaAz = 3600 * (az - float (resp['azimuth']))
      deltaAlt = 3600 * (alt - float (resp['altitude']))      

      print ("%s, %s, %s, %s, %c, %3.4f, %3.4f, %4.0f, %4.0f, %4.0f, %4.0f" % (row['RAHA'], row['dec'], resp['primaryAxis'],resp['secondaryAxis'], pierSide, ra, dec, delta1, delta2, deltaAz,deltaAlt))

# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-c', '--config')
  parser.add_argument('-t', '--testcase')
  opts = parser.parse_args(args)
  if opts.config == None:
    opts.config = 'config.yml'
  if opts.testcase == None:
    opts.testcase = 'tests.csv'
  return opts




# Main program
# Call the function to read the argument values
options = readOptions()
p = openPort()
if (p == None):
  sys.exit()

ta = TeenAstro('serial', '/dev/ttyACM0')
#ta = TeenAstro('tcp', '192.168.0.12')

ta.open()
if (p == None):
  print ('Error connecting to TeenAstro')
  sys.exit()

today = datetime.date.today()
now = datetime.datetime.now()

ta.setDate(today)
ta.setLocalTime(now)

ta.readSite()
ta.readDateTime()
ta.readSidTime()
ta.readGears()

doTestCases(p, options.config, options.testcase);

ta.disableTracking() 

