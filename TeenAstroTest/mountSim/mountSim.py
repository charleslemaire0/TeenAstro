import math, time, sys, argparse    
import numpy as np  
import trimesh
import trimesh.viewer
import glooey
import trimesh.transformations as tt
import pyglet
from teenastro import TeenAstro


class Mount:
    def __init__(self, ta):

        self.mountType = ta.readMountType()     # one-letter code returned by TeenAstro 

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
        self.axis1Steps = ta.getAxis1()
        self.axis2Steps = ta.getAxis2()
        self.alpha = self.beta = 0


    def update(self, ta, scene):
        xaxis, yaxis, zaxis = [1,0,0],[0,1,0],[0,0,1]
        a1 = ta.getAxis1()
        a2 = ta.getAxis2()

        if (a1 != self.axis1Steps):
            self.alpha = self.alpha + np.deg2rad((self.axis1Steps-a1))
            self.axis1Steps = a1
        if (a2 != self.axis2Steps):
            self.beta = self.beta + np.deg2rad((self.axis2Steps-a2))
            self.axis2Steps = a2

# The clumsy code below represents the transformations required on the parts of each mount 
# so that they display properly. This is because I do not master completely the CAD program I used (SolveSpace)
# and it was quicker to do this than to properly align the parts in SolveSpace
        if (self.mountType == 'E'):             # Equatorial German (GEM)
            Mb1 = tt.rotation_matrix(np.deg2rad(90), yaxis)
            Mb2 = tt.rotation_matrix(np.deg2rad(-90), xaxis)
            Mb = tt.concatenate_matrices(Mb1, Mb2)
            Mp1 = tt.translation_matrix([-17,38,0])
            Mp2 = tt.rotation_matrix(np.deg2rad(90), yaxis)      
            Mp3 = tt.rotation_matrix(np.deg2rad(-45), xaxis)      
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
            Mp3 = tt.rotation_matrix(np.deg2rad(-45), yaxis)      
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
            Ms1 = tt.translation_matrix([28, 0, 18])
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

# Declare function to define command-line arguments
def readOptions(args=sys.argv[1:]):
  parser = argparse.ArgumentParser(description="The parsing commands lists.")
  parser.add_argument('-p', '--ip', help='TeenAstro IP address')
  opts = parser.parse_args(args)
  if opts.ip == None:
    opts.ip = '192.168.0.21'
  return opts


# Main program
class Application:

    def __init__(self, options):

        self.width, self.height = 800, 600
        window = self._create_window(width=self.width, height=self.height)

        gui = glooey.Gui(window)
        hbox = glooey.HBox()

        self.ta = self.init_TeenAstro(options.ip)
        if self.ta == None:
            self.log ("Error connecting to TeenAstro")
            sys.exit(1)

        self.log('Connected')

        self.mount = Mount(self.ta)

        scene = trimesh.Scene([self.mount.base, self.mount.primary, self.mount.secondary])
        self.scene_widget1 = trimesh.viewer.SceneWidget(scene)
        self.update(0)
        hbox.add(self.scene_widget1)

        # text box - not yet implemented
#        self.textWidget = glooey.Label(self.text)
#        hbox.add(self.textWidget)

        if self.mount.mountType == 'E':
            self.log('German Equatorial')
        elif self.mount.mountType == 'K':
            self.log('Equatorial Fork')
        elif self.mount.mountType == 'k':
            self.log('Alt-Az Fork')
        elif self.mount.mountType == 'A':
            self.log('Alt-Az Tee')

        gui.add(hbox)
        pyglet.clock.schedule_interval(self.update, 1. / 5) 
        pyglet.app.run()

    # May override this with a graphical window in the future
    def log(self, message):
        print (message)

    def init_TeenAstro(self, ip):
        ta = TeenAstro('tcp', ip)
        p = ta.open()

        if (p == None):
          self.log ('Error connecting to TeenAstro')
          sys.exit()
        return ta

    def update(self, dt):
        self.mount.update(self.ta, self.scene_widget1.scene)
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
        def on_key_press(symbol, modifiers):
            if modifiers == 0:
                if symbol == pyglet.window.key.Q:
                    window.close()
        return window

 
if __name__ == '__main__':
    options = readOptions()
    Application(options)
