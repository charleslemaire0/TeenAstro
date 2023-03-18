import math, time, sys, argparse, csv, random
import numpy as np  
import trimesh
import trimesh.viewer
import glooey
import trimesh.transformations as tt
import pyglet
from pyglet import clock
from teenastro import TeenAstro, deg2dms
textInput = ''
cmdComplete = False

class Mount:
    def __init__(self, ta):

        self.mountType = ta.readMountType()     # one-letter code returned by TeenAstro 
        self.version = ta.getVersion()
        self.name = ta.getName()

        # Read the 3 parts that compose each type of mount
        if (self.mountType == 'E'):             # Equatorial German (GEM)
            self.base = trimesh.load('eq_german/base.stl')
            self.primary = trimesh.load('eq_german/primary.stl')
            self.secondary = trimesh.load('eq_german/secondary.stl')
        elif (self.mountType == 'K'):           # Equatorial Fork
            self.base = trimesh.load('eq_fork/base.stl')
            self.primary = trimesh.load('eq_fork/primary.stl')
            self.secondary = trimesh.load('eq_fork/secondary.stl')
        elif (self.mountType == 'A'):           # Altaz Tee
            self.base = trimesh.load('altaz_tee/base.stl')
            self.primary = trimesh.load('altaz_tee/primary.stl')
            self.secondary = trimesh.load('altaz_tee/secondary.stl')
        elif (self.mountType == 'k'):           # Altaz Fork
            self.base = trimesh.load('altaz_fork/base.stl')
            self.primary = trimesh.load('altaz_fork/primary.stl')
            self.secondary = trimesh.load('altaz_fork/secondary.stl')

        self.base.visual.face_colors = [100, 100, 100, 255]
        self.primary.visual.face_colors = [0, 0, 100, 255]
        self.secondary.visual.face_colors = [255, 0, 0, 255]

        # Read parameters from TeenAstro
        ta.readGears()
        self.axis1Degrees = ta.getAxis1()
        self.axis2Degrees = ta.getAxis2()
        self.alpha = self.beta = 0
        self.latitude = ta.getLatitude()

    def update(self, ta, scene):
        xaxis, yaxis, zaxis = [1,0,0],[0,1,0],[0,0,1]
        a1 = ta.getAxis1()
        a2 = ta.getAxis2()
        if (a1 == None) or (a2 == None):
            print("Connection Error")
            sys.exit(1)

        az = ta.getAzimuth()
        alt = ta.getAltitude()

# The transformations from axis positions (reported by TeenAstro) and rotations of the model
# Need to take into account the motor configurations 
# and also the S hemisphere which inverts the RA axis (for Firmware 2.x only)
        if (a1 != self.axis1Degrees):
            if (self.name == 'TeenAstroUniversal'):
                if ta.axis1Reverse:
                    dir = 1
                else:
                    dir = -1       
            else:
                if ta.axis1Reverse != (self.latitude>0):
                    dir = 1
                else:
                    dir = -1
            self.alpha = self.alpha + dir * np.deg2rad(self.axis1Degrees-a1)
            self.axis1Degrees = a1

        if (a2 != self.axis2Degrees):
            if ta.axis2Reverse:
                dir = 1
            else:
                dir = -1
            self.beta = self.beta + dir * np.deg2rad(self.axis2Degrees-a2)
            self.axis2Degrees = a2

# The clumsy code below represents the transformations required on the parts of each mount 
# so that they display properly. This is because I do not master completely the CAD program I used (SolveSpace)
# and it was quicker to do this than to properly align the parts in SolveSpace
        if (self.mountType == 'E'):             # Equatorial German (GEM)
            Mb1 = tt.rotation_matrix(np.deg2rad(90), yaxis)
            Mb2 = tt.rotation_matrix(np.deg2rad(-90), xaxis)
            Mb = tt.concatenate_matrices(Mb1, Mb2)
            Mp1 = tt.translation_matrix([-17,38,0])
            Mp2 = tt.rotation_matrix(np.deg2rad(90), yaxis)      
            Mp3 = tt.rotation_matrix(np.deg2rad(abs(self.latitude)-90), xaxis)      
            Mp4 = tt.rotation_matrix(self.alpha, yaxis)
            Mp = tt.concatenate_matrices(Mp1, Mp2, Mp3, Mp4)
            Ms1 = tt.rotation_matrix(np.deg2rad(90), xaxis)
            Ms2 = tt.translation_matrix([0,30,0])
            Ms3 = tt.rotation_matrix(-self.beta, yaxis)
            Ms = tt.concatenate_matrices(Mp,Ms1,Ms2,Ms3)

        elif (self.mountType == 'K'):           # Equatorial Fork
            Mb1 = tt.rotation_matrix(np.deg2rad(90), yaxis)
            Mb2 = tt.rotation_matrix(np.deg2rad(-90), xaxis)
            Mb = tt.concatenate_matrices(Mb1, Mb2)
            Mp1 = tt.translation_matrix([0,60,0])
            Mp2 = tt.rotation_matrix(np.deg2rad(-90), xaxis)      
            Mp3 = tt.rotation_matrix(np.deg2rad(abs(self.latitude)-90), yaxis)      
            Mp4 = tt.rotation_matrix(self.alpha, zaxis)
            Mp = tt.concatenate_matrices(Mp1, Mp2, Mp3, Mp4)
            Ms1 = tt.rotation_matrix(np.deg2rad(90), yaxis)
            Ms2 = tt.translation_matrix([-90, 0, 0])
            Ms3 = tt.rotation_matrix(-self.beta, yaxis)
            Ms = tt.concatenate_matrices(Mp,Ms1,Ms2,Ms3)

        elif (self.mountType == 'A'):           # Altaz Tee
            Mb = tt.rotation_matrix(0, xaxis)    
            Mp1 = tt.rotation_matrix(self.alpha, zaxis)
            Mp2 = tt.translation_matrix([0, 0, 80])
            Mp = tt.concatenate_matrices(Mp1,Mp2)
            Ms1 = tt.translation_matrix([20, 0, 16])
            Ms2 = tt.rotation_matrix(-self.beta, xaxis)
            Ms = tt.concatenate_matrices(Mp,Ms1,Ms2)

        elif (self.mountType == 'k'):           # Altaz Fork
            Mb = tt.rotation_matrix(np.deg2rad(180), yaxis)     
            Mp = tt.rotation_matrix(self.alpha, yaxis)
            Ms1 = tt.translation_matrix([0,56,0])
            Ms2 = tt.rotation_matrix(self.beta, zaxis)
            Ms = tt.concatenate_matrices(Mp,Ms1,Ms2)
        
        base = scene.graph.nodes_geometry[0]
        primary = scene.graph.nodes_geometry[1]
        secondary = scene.graph.nodes_geometry[2]

        # apply the transform to the node
        scene.graph.update(base, matrix=Mb)
        scene.graph.update(primary, matrix=Mp)
        scene.graph.update(secondary, matrix=Ms)

        # return the axis positions for logging
        return [a1,a2,az,alt]



# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-t', '--portType', help='TeenAstro connection type (tcp or serial)')
  parser.add_argument('-p', '--portName', help='TeenAstro IP address or serial port')
  parser.add_argument('-b', '--baudRate', help='TeenAstro baud rate')
  opts = parser.parse_args(args)
  return opts

testCase = [{'name':'North','az':0,'alt':0},{'name':'East','az':90,'alt':0}, {'name':'South','az':180,'alt':0}]


# Main program
class Application:

    def __init__(self, options):

        self.width, self.height = 800, 600
        window = self._create_window(width=self.width, height=self.height)

        gui = glooey.Gui(window)
        hbox = glooey.HBox()

        self.ta = self.init_TeenAstro(portType=options.portType, portName=options.portName, baudRate=options.baudRate)
        if self.ta == None:
            self.log ("Error connecting to TeenAstro")
            sys.exit(1)

        self.log('Connected')

        self.mount = Mount(self.ta)
        self.testData = []
        self.t1 = 0

        scene = trimesh.Scene([self.mount.base, self.mount.primary, self.mount.secondary])
        self.scene_widget1 = trimesh.viewer.SceneWidget(scene)
        self.update(0)
        hbox.add(self.scene_widget1)

        # text box - not yet implemented
#        self.textWidget = glooey.Label('hello')
#        hbox.add(self.textWidget)
        vbox = glooey.VBox()
        hbox.add(vbox)

        button = glooey.Button("Meridian Flip Test")
        button.push_handlers(on_click=self.startFlipTest)
        vbox.add(button)

        button = glooey.Button("Coordinates Test")
        button.push_handlers(on_click=self.startCoordTest)
        vbox.add(button)

        if self.mount.mountType == 'E':
            self.log('German Equatorial')
        elif self.mount.mountType == 'K':
            self.log('Equatorial Fork')
        elif self.mount.mountType == 'k':
            self.log('Alt-Az Fork')
        elif self.mount.mountType == 'A':
            self.log('Alt-Az Tee')

        self.log('Latitude:'+str(self.ta.getLatitude()))

        gui.add(hbox)
        pyglet.clock.schedule_interval(self.update, 1. / 5) 
        pyglet.app.run()

    def startFlipTest(self, dt):
        if self.mount.mountType != 'E':
            self.log('Can only run meridian flip test with German Equatorial mount')
            return
        if not self.ta.isAtHome():
            self.log('Error - mount is not at home')
            return

        self.flipTestState = 'start'
        pyglet.clock.schedule_interval(self.runFlipTest, 0.5) 

    def runFlipTest(self, dt):
        if self.ta.isSlewing():
            return

        code = self.ta.getErrorCode()
        if code!= 'ERR_NONE':
            self.log(code)
            self.log('Pier Side: %s' % self.ta.getPierSide())
            self.log('RA:%s Dec:%s' % (deg2dms(self.ra), deg2dms(self.dec)))
            pyglet.clock.unschedule(self.runFlipTest) 
            return

        if self.flipTestState == 'start':
            self.log('Starting Meridian Test')
            self.eastLimit = self.ta.getMeridianEastLimit()
            self.westLimit = self.ta.getMeridianWestLimit()

            self.initialRA = self.ta.readSidTime() + (1.0 + float(self.eastLimit)) / 15.0 # goto "eastLimit" east of south meridian  
            self.initialDec = 45.5
            self.ta.gotoRaDec(self.initialRA, self.initialDec)
            self.log('goto East Limit')
            self.flipTestState = 'goto1'
            return

        if self.flipTestState == 'goto1':
            self.ra = self.ta.getRA() - (0.05 + float(self.eastLimit + self.westLimit) / 15.0)  # go almost to west limit 
            self.dec = self.ta.getDeclination()
            self.log('goto West Limit')
            self.ta.gotoRaDec(self.ra, self.dec)
            self.flipTestState = 'goto2'
            self.t = self.startWaiting = time.time()
            return

        if self.flipTestState == 'goto2':
            if (self.ta.getPierSide() == 'W'):    # still on west side. 
                t = time.time()
                if (t < self.t + 1):
                    return
                self.t = t
                self.t1 = self.t1 + 1
                gt = random.randint(0,999)
                if gt % 4 == 0:
                    dir = 'w'
                elif gt % 4 == 1:
                    dir = 'e'
                elif gt % 4 == 2:
                    dir = 'n'
                elif gt % 4 == 3:
                    dir = 's'
                self.log('Guiding %s %d' % (dir, gt))
                self.ta.guideCmd(dir, gt)
                # every n seconds, issue a goto to the same position
                if self.t1 == 60:
                    self.t1 = 0
                    self.log('Goto %s %s' % (deg2dms(self.ra), deg2dms(self.dec)))
                    self.ta.gotoRaDec(self.ra, self.dec)
                    self.log('Requesting flip')
                    # poll a few times to see if we start slewing
                    loop = 0
                    while True:
                        if self.ta.isSlewing():
                            self.log('Slewing')
                            break
                        time.sleep(0.1)
                        loop = loop + 1
                        if loop == 10:
                            ha = 15.0 * (self.ta.getLST() - self.ra) 
                            self.log('Request to flip failed at hour angle %02.2f degrees' % ha)
                            break

            else:           # we have flipped - test is done
                self.log('meridian flip done after %d seconds - goto Home' % (self.t - self.startWaiting))
                self.ta.goHome()
                self.flipTestState = 'start'    # redo until end of time
                time.sleep(1)       # give some time before starting test again
#                pyglet.clock.unschedule(self.runFlipTest) 

    def startCoordTest(self,arg):
        self.testStep = 0
        self.testData = []
        pyglet.clock.schedule_interval(self.runCoordTest, 0.5) 

    def runCoordTest(self, dt):
        if self.ta.isSlewing():
            return

        if self.testStep == len(testCase):
            self.log ('done - go Home')
            self.ta.goHome()
            pyglet.clock.unschedule(self.runCoordTest) 
            fields = ['axis1', 'axis2', 'azimuth', 'altitude']
            with open('mountSim.csv', 'w') as f:
                write = csv.writer(f)
                write.writerow(fields)
                write.writerows(self.testData)            
            return

        self.log ('goto ' + testCase[self.testStep]['name'])
        res = self.ta.gotoAzAlt(testCase[self.testStep]['az'],testCase[self.testStep]['alt'])
        if (res):
            self.log(res)
        self.testStep = self.testStep + 1


    # May override this with a graphical window in the future
    def log(self, message):
        print (message)

    def init_TeenAstro(self, portType, portName, baudRate):
        ta = TeenAstro(portType=portType, portName=portName, baudRate=baudRate)
        p = ta.open()

        if (p == None):
          self.log ('Error connecting to TeenAstro')
          sys.exit()
        return ta

    def update(self, dt):
        axisPos = self.mount.update(self.ta, self.scene_widget1.scene)
        self.testData.append(axisPos)
        self.scene_widget1._draw()

    def _create_window(self, width, height):
        try:
            config = pyglet.gl.Config(sample_buffers=1,
                                      samples=4,
                                      depth_size=24,
                                      double_buffer=True)
            window = pyglet.window.Window(config=config,
                                          width=width,
                                          height=height)
        except pyglet.window.NoSuchConfigException:
            config = pyglet.gl.Config(double_buffer=True)
            window = pyglet.window.Window(config=config,
                                          width=width,
                                          height=height)

        @window.event
        # custom commands for low-level testing
        def on_text(ch):
            if (ch == 's'):
                self.ta.moveCmd('s')
                print ('move S')
            elif (ch == 'n'):
                self.ta.moveCmd('n')
                print ('move N')
            elif (ch == 'e'):
                self.ta.moveCmd('e')
                print ('move E')
            elif (ch == 'w'):
                self.ta.moveCmd('w')
                print ('move W')
            elif (ch == 'q'):
                self.ta.abort()
                print ('abort')
            elif (ch == 't'):
                print ('Enable Tracking')
                self.ta.enableTracking()
            elif (ch == 'd'): 
                print ('Disable Tracking')
                self.ta.disableTracking()
            elif (ch == '<'):               # RA test
                ra = self.ta.getRA()
                dec = self.ta.getDeclination()
                ra = ra + 1
                if ra > 24:
                    ra = ra - 24
                self.ta.gotoRaDec(ra, dec)
                print ('goto RA=%f Dec=%f'% (ra,dec))
            elif (ch == '>'):               # RA test
                ra = self.ta.getRA()
                dec = self.ta.getDeclination()
                ra = ra - 1
                if ra < 0:
                    ra = ra + 24
                self.ta.gotoRaDec(ra, dec)
                print ('goto RA=%f Dec=%f'% (ra,dec))
            elif (ch == 'v'):               # RA test
                ra = self.ta.getRA()
                dec = self.ta.getDeclination()
                if dec > 15:
                    dec = dec - 15
                self.ta.gotoRaDec(ra, dec)
                print ('goto RA=%f Dec=%f'% (ra,dec))
            elif (ch == '^'):               # RA test
                ra = self.ta.getRA()
                dec = self.ta.getDeclination()
                if dec > -75:
                    dec = dec - 15
                self.ta.gotoRaDec(ra, dec)
                print ('goto RA=%f Dec=%f'% (ra,dec))
            elif (ch == 'h'):
                self.ta.goHome()
                print ('go home')
            elif (ch == 'z'):
                self.ta.gotoAzAlt(0, 90)
                print ('go to Zenith')

        return window

 
if __name__ == '__main__':
    options = readOptions()
    Application(options)
