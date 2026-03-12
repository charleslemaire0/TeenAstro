from jinja2 import Environment, FileSystemLoader
from pyongc import ongc, data
import numpy as np

constellations = [
  "And","Ant","Aps","Aql","Aqr","Ara","Ari","Aur","Boo","CMa","CMi","CVn","Cae","Cam","Cap","Car","Cas","Cen","Cep","Cet","Cha","Cir",
  "Cnc","Col","Com","CrA","CrB","Crt","Cru","Crv","Cyg","Del","Dor","Dra","Equ","Eri","For","Gem","Gru","Her","Hor","Hya","Hyi","Ind",
  "LMi","Lac","Leo","Lep","Lib","Lup","Lyn","Lyr","Men","Mic","Mon","Mus","Nor","Oct","Oph","Ori","Pav","Peg","Per","Phe","Pic","PsA",
  "Psc","Pup","Pyx","Ret","Scl","Sco","Sct","Ser","Sex","Sge","Sgr","Tau","Tel","TrA","Tri","Tuc","UMa","UMi","Vel","Vir","Vol","Vul"," ---"
]


objectTypes = [
  "Galaxy",        "Open Cluster",   "Star",           "Double star", "Other",      "Galaxy Pair",    "Galaxy Triplet",
  "Galaxy Group",  "Globular Cluster", "Planetary Nebula", "Nebula",      "HII Ionized region", "Star cluster + Nebula", "Asterism",
  "Reflection Nebula", "Supernova remnant",  "Emission Nebula",  "Non Existant","Nova",       "Duplicate",      "Dark Nebula"
]

# Initialize template system
env = Environment(loader = FileSystemLoader('templates'))
template = env.get_template("ic_template.jinja")


# convert an angle expressed in radians to a short representing degrees (360º -> 65535)
def raToShort(ra_rad):
    return int(np.rad2deg(ra_rad) * (65535 / 360))

def decToShort(dec_rad):
    return int(np.rad2deg(dec_rad) * (65535 / 180))

class icObject():
  def __init__(self, dso):
    self.valid = True
    self.name   = 0 

    try:
        if (dso.magnitudes[0] > 15.0):
            self.valid = False
            return
    except:     # if magnitude is not defined, keep it
        self.valid = True

    try:
        if (dso.surface_brightness < 21.5):
            self.valid = False
    except:         # if surface brightness is not defined, keep it in list
        self.valid = True

    # handle Se1 and Se2 as Ser
    if (dso.constellation == 'Se1' or dso.constellation == 'Se2'):
        dso.constellation = 'Ser'
    try:
        self.constellation = constellations.index(dso.constellation)
    except:
        print (dso.constellation)
        self.valid = False 
    try:
        self.type = objectTypes.index(dso.type)
    except:
        self.valid = False 

    self.subId = 0
    # split the string, only use first word
    # if last character is a letter, use that for subId
    name = dso.name.split()[0]
    ch = name[-1]

    if (ch in 'ABCDN'):
        name = name[:-1]                # truncate last character
        self.subId = ' ABCDN'.index(ch)   # set subId to index of last character (index 0 means no subId)
#        print(dso.name, self.subId)
    try:
        self.id = int(name[2:])
    except:
        self.valid = False
 
    try:
        self.mag = int ((dso.magnitudes[0] + 2.5) * 10)
    except:
        self.valid = False

    # convert coordinates from radians (float) to a short integer     
    self.ra = raToShort(dso.rad_coords[0])
    self.dec = decToShort(dso.rad_coords[1])


# Get NGC objects
dsoList = ongc.listObjects(catalog = 'IC', uptovmag = 16.0)

# create an empty list and populate with processed objects
icList = []

for dso in dsoList:
    ic = icObject(dso)
    if ic.valid:
        icList.append(ic)

with open("renders/ta_ic_select_c.h", 'w') as f:
    print (template.render(numObjects=len(icList), objects=icList), file=f)