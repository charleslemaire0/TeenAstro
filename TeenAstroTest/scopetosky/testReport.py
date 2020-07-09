import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.projections as prj
import numpy as np
import skyfield
import math, sys
from skyfield.api import load, Topos, Star

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
    try:
      (d,m) = dms.split()
      if (float(d) >= 0):
        return float(d) + float(m)/60 
      else:
        return float(d) - float(m)/60 
    except:
        return float(dms)

# Equatorial to horizontal conversion - all parameters in radians
def eq2hor(H, delta, latitude):
  cosphi = np.cos(latitude)
  sinphi = np.sin(latitude)
  sindelta = np.sin(delta)
  cosdelta = np.cos(delta)
  altitude = np.arcsin(sindelta*sinphi + cosdelta * cosphi * np.cos(H))
  x = -(sinphi * np.cos(delta) * np.cos(H)) + cosphi * np.sin (delta)
  y = np.cos(delta) * np.sin (H)
  azimuth = -np.arctan2(y,x)
#  over = azimuth<0
#  azimuth[over] = azimuth[over] + 2 * math.pi
#  azimuth[azimuth>2*math.pi] = azimuth[azimuth>2*math.pi] - 2 * math.pi
  return (azimuth,altitude)

# uses arctan2 - see Wikipedia
def hor2eq(azimuth, altitude, latitude):
  cosphi = np.cos(latitude)
  sinphi = np.sin(latitude)
  x = sinphi * np.cos(altitude) * np.cos(azimuth)+cosphi * np.sin(altitude)
  y = np.cos(altitude) * np.sin(azimuth)

  H = np.arctan2(y,x)
  delta = sinphi * np.sin(altitude) - cosphi * np.cos(altitude) * np.cos(azimuth)
  return (H,delta)


# Cartesian to polar conversion
def cart2pol(x, y):
    rho = np.sqrt(x**2 + y**2)
    theta = np.arctan2(y, x)
    return(rho, theta)

def pol2cart(rho, theta):
    x = rho * np.cos(theta)
    y = rho * np.sin(theta)
    return(x, y)

# returns the HA/Dec (in radians) corresponding to a pair of axis positions
def axis2hadec(axis1, axis2, flipped):
  ha = math.pi + np.radians(axis1)

  # Reverse when pier side is west
  ha[flipped] = ha[flipped] + math.pi
  over = ha>2*math.pi
  ha[over] = ha[over] - 2*math.pi 

  dec = math.pi - np.radians(axis2)
  dec[flipped] = math.pi - dec[flipped]

  return (ha,dec)

def chordLength(lambda1, phi1, lambda2, phi2):
  deltaX = np.cos(phi2) * np.cos (lambda2) - np.cos(phi1) * np.cos (lambda1)
  deltaY = np.cos(phi2) * np.sin (lambda2) - np.cos(phi1) * np.sin (lambda1)
  deltaZ = np.sin(phi2) - np.sin(phi1)
  C = np.sqrt(deltaX*deltaX+deltaY*deltaY+deltaZ*deltaZ)
  return 2 * np.arcsin(C/2)

# Read the input file, parsing the coordinates as needed (RA is kept in hours)
if (len(sys.argv)<2):
  print ('Syntax is TATestReport.py <input file>')
  sys.exit()
res = pd.read_csv(sys.argv[1],skipinitialspace=True, converters={'RA':dms2deg,'Dec':dms2deg})


# Assign the variables. Each variable represents a column of values (a numpy array)
ra = np.array(res['RA'])
dec = np.array(res['Dec'])
delta1 = np.array(res['delta1'])
delta2 = np.array(res['delta2'])
azimuth = np.array(res['azimuth'])
altitude = np.array(res['altitude'])
SidT = np.array(res['SidT'])
pierSide = np.array(res['pierSide'])
actualAxis1 = np.array(res['actualAxis1'])
actualAxis2 = np.array(res['actualAxis2'])
computedAxis1 = np.array(res['computedAxis1'])
computedAxis2 = np.array(res['computedAxis2'])
latitude = np.array(res['latitude'])

flipped = pierSide=='W'

# actual and computed HA and dec 
(actualHa, actualDec) = axis2hadec(actualAxis1, actualAxis2, flipped)
(computedHa, computedDec) = axis2hadec(computedAxis1,computedAxis2,flipped)
eqDistance = 3600 * np.degrees(chordLength(actualHa, actualDec, computedHa, computedDec))

# actual and computed altitude and azimuth
(actualAz,actualAlt) = eq2hor(actualHa,actualDec,np.radians(latitude))
(computedAz,computedAlt) = eq2hor(computedHa,computedDec,np.radians(latitude))
horDistance = 3600 * np.degrees(chordLength(actualAz, actualAlt, computedAz, computedAlt))


rho = 3600 * np.degrees(chordLength(actualHa, actualDec, computedHa, computedDec))
theta = np.arctan2(computedAlt-actualAlt, computedAz-actualAz) + np.radians(azimuth) 


(x,y) = pol2cart(rho, theta)

# Color the arrows according to the pier side
colors=np.full(len(res.index), 'blue')
colors[flipped] = 'red'


# Find the largest error to display on the graph
maxError = np.amax(rho)
maxIndex = np.where(rho == maxError)
legend = 'Max Error: %3.0f" at RA=%2.2f, Dec=%2.2f, Az=%2.2f, Alt=%2.2f' % (maxError, ra[maxIndex],dec[maxIndex],azimuth[maxIndex], altitude[maxIndex])

# Create the arrow graph
ax = plt.subplot(111, projection='polar')
ax.quiver(np.radians(azimuth), altitude, x, y,  scale= 2000, width = .004, headwidth=2, color=colors)

# Plot with 90 degrees declination in the middle, north at bottom, RA increasing clockwise
ax.set_rlim(ymin=90, ymax=0)
ax.set_rgrids(radii= [30,60], labels=None, angle=None, fmt=None)
ax.set_theta_zero_location('S')

ax.annotate(legend, xy=(0,0), xycoords='figure pixels')

ax.set_theta_direction(1)
ax.set_title("TeenAstro vs ScopeToSky", va='bottom')
plt.show()

