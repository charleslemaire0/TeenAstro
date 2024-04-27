import matplotlib.pyplot as plt
import matplotlib.lines as lines
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from skyfield.api import wgs84, load, position_of_radec, utc, Star
from teenastro import TeenAstro, deg2dms
import numpy as np  
import sys, time
from pandas import read_csv
from skyfield.positionlib import Apparent, Barycentric, Astrometric, Distance
from skyfield.earthlib import refraction
from skyfield.projections import build_stereographic_projection
from trackingPlot import eqAxesToEqu, sign, altazAxesToAltAz


class alignmentPlot():
  def __init__(self, window, ts, ta, dpi):
    self.window = window
    self.ts = ts
    self.ta = ta
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
    self.fig, self.ax = plt.subplots(figsize=(6, 6), dpi=dpi)
    plt.subplots_adjust(left=0, bottom=0, right=1, top=1, wspace=0, hspace=0)
    self.fig.add_artist(lines.Line2D([0.45, 0.55], [0.5, 0.5], linewidth=1, color='black')) # cursor H
    self.fig.add_artist(lines.Line2D([0.5, 0.5], [0.45, 0.55], linewidth=1, color='black')) # cursor V
    self.figure_canvas_agg = FigureCanvasTkAgg(self.fig, master=self.window['alignment_cv'].TKCanvas)
    self.home_error_axis1 = 0.0
    self.home_error_axis2 = 0.0
    self.state = 'STOP'
    self.ra = 0
    self.dec = 0

  def connect(self, ta):
    self.ta = ta
    self.lat = self.ta.getLatitude()
    self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
    self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
    self.subName = self.ta.getSubName()
    self.start()

  def handleEvent(self, ev, v, w):
    if (ev == 'startStopAlignment'):
      if not self.ta.isConnected():
        self.log('Not connected')
        return

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
      self.ra = star.ra_hours.values[0]
      self.dec = star.dec_degrees.values[0]
      self.log('Goto {0} RA:{1:2.2f} Dec:{2:3.2f}'.format (self.window['alignmentTarget'].get(), self.ra, self.dec))
      self.log (self.ta.gotoRaDec(self.ra, self.dec))

    if (ev == 'CenteringSpeed'):
      self.ta.setRate(int(self.window['CenteringSpeed'].get()))

    if (ev == 'alignmentSync'):
      self.log('Sync {0} RA:{1:2.2f} Dec:{2:3.2f}'.format (self.window['alignmentTarget'].get(), self.ra, self.dec))
      self.log (self.ta.syncRaDec())

    # Simulate plate-solve:
    # Sync to currentPos+1º, then slew to currentPos 
    if (ev == 'syncSlew'):
      ra = self.ta.getRA() 
      dec = self.ta.getDeclination()
      ra1 = ra + 1 / 15.0 
      dec1 = dec + 1
      res = self.ta.setTarget(ra1,dec1)
      if (res != 'ok'):
        self.log('Error setting Ra/Dec')
      else:
        self.ta.syncRaDec()
        self.log('Current: {0:2.2f} / {1:3.2f}, Sync to {2:2.2f} / {3:3.2f}'.format (ra, dec, ra1, dec1))
      time.sleep(0.5)
      ra2 = self.ta.getRA() 
      dec2 = self.ta.getDeclination()
      self.log('Now: {0:2.2f} / {1:3.2f}, Goto {2:2.2f} / {3:3.2f} '.format (ra2, dec2, ra, dec))
      self.ta.gotoRaDec(ra, dec)
      self.log('done')

    if (ev == 'alignN'):
      self.log('Move N')
      self.ta.moveCmd('n')
      time.sleep(1.0)
      self.ta.stopCmd('n')
    if (ev == 'alignS'):
      self.log('Move S')
      self.ta.moveCmd('s')
      time.sleep(1.0)
      self.ta.stopCmd('s')
    if (ev == 'alignE'):
      self.log('Move E')
      self.ta.moveCmd('e')
      time.sleep(1.0)
      self.ta.stopCmd('e')
    if (ev == 'alignW'):
      self.log('Move W')
      self.ta.moveCmd('w')
      time.sleep(1.0)
      self.ta.stopCmd('w')

    if (ev == 'startAlign'):
      self.log('Start Alignment')
      self.log(self.ta.startAlignment())
    if (ev == 'clearAlign'):
      self.log('Clear Alignment')
      self.log(self.ta.clearAlignment())
    if (ev == 'addStar'):
      self.log('Add Star')
      self.log(self.ta.addStar())



    if (ev =='__TIMEOUT__'):
      if not self.ta.isConnected():
        return
      if (self.state == 'RUN'):
        self.home_error_axis1 = float(v['home_error_axis1']) / 60        # in degrees
        self.home_error_axis2 = float(v['home_error_axis2']) / 60 
        self.pole_error_az = float(v['pole_error_az'])  / 60
        self.pole_error_alt = float(v['pole_error_alt']) / 60
        self.update()

  def dec2string(self, dms):
    return "{0:03d}º{1:02d}'{2:02d}''".format(dms[0], dms[1], dms[2])

  def ra2string(self, dms):
    return "{0:02d}h{1:02d}m{2:02d}s".format(dms[0], dms[1], dms[2])

  def log(self, message):
    print (message)

  def render(self):
    self.ax.scatter(self.stars['x'], self.stars['y'],s=self.marker_size, color='k')
    self.figure_canvas_agg.draw()
    self.figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)

  def getAxisCoords(self):
    lst = self.ta.getLST()            # in hours
    if self.mountType in ['E', 'K']:        # Eq mount
      pierSide = self.ta.getPierSide()
      axis1 = self.ta.getAxis1()
      axis2 = self.ta.getAxis2()   
      ha, ra, dec = eqAxesToEqu(self.subName, pierSide, axis1, axis2, self.lat, lst)
      return (ra, dec, lst, ha) 
    else:                                   # AltAz mount
      axis1 = self.ta.getAxis1()
      axis2 = self.ta.getAxis2()   
      lst = self.ta.getLST()            # in hours
      az, alt = altazAxesToAltAz(axis1, axis2, self.lat)

      # use skyfield to find equ coordinates
      t = self.ts.from_datetime(self.ta.readDateTime())
      direction = self.site.at(t).from_altaz(alt_degrees=alt, az_degrees=az)

      ra, dec, distance = direction.radec()      
      ha = lst - ra.hours
      return (ra.hours, dec.degrees, lst, ha) 

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
    self.window['ra_comp'].Update(self.ra2string(deg2dms(ra)))
    self.window['dec_comp'].Update(self.dec2string(deg2dms(dec)))

    self.window['ra_disp'].Update(self.ra2string(deg2dms(self.ta.getRA())))
    self.window['dec_disp'].Update(self.dec2string(deg2dms(self.ta.getDeclination())))

    self.window['align_status'].Update(self.ta.alignStatus())
    self.window['align_stars'].Update(self.ta.alignNumStars())   

    projection = build_stereographic_projection(center)
    star_positions = self.site.at(t).observe(Star(ra_hours=self.stars.ra_hours, dec_degrees=self.stars.dec_degrees)) 
    self.stars['x'], self.stars['y'] = projection(star_positions)

  def update(self):
    ra, dec, lst, ha = self.getAxisCoords()
    ra = ra + self.home_error_axis1 /  15
    if (self.ta.getPierSide() == 'W'):
      dec = dec + self.home_error_axis2 
    else:
      dec = dec - self.home_error_axis2 
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
    self.lat = self.ta.getLatitude()
    self.lon = -self.ta.getLongitude()       # LX200 treats west longitudes as positive, Skyfield as negative
    self.site = self.planets['earth'] + wgs84.latlon(self.lat, self.lon)
    ra, dec, lst, ha = self.getAxisCoords()
    self.project(ra, dec)
    self.scatter = self.ax.scatter(self.stars['x'], self.stars['y'],s=self.marker_size, color='k')
    self.state = 'RUN'
