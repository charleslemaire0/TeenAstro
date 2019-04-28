#include <Arduino.h>
#include "TeenAstroCatalogOld.h"
#include "math.h"

// Do not change anything in the structs or arrays below, since they
// have to be in sync with the extractions scripts.


// Type of object, in the Open NGC database. Do not change the
// order of this, since it has to match what is in Open NGC
const char* Txt_Object_Type[21] = {
  "Galaxy",        "Open Cluster",   "Star",           "Double Star", "Other",      "Galaxy Pair",    "Galaxy Triplet",
  "Galaxy Group",  "Globular Clstr", "Planetary Nbla", "Nebula",      "Hii Region", "Cluster+Nebula", "Asterism",
  "Reflectn Nbla", "SuperNov Rmnt",  "Emission Nbla",  "Non Existant","Nova",       "Duplicate",      "Dark Nebula"
};

// Constellation abbreviation, alphabetical order
const char* Txt_Constellations[89] = {
  "And", //  0
  "Ant", //  1
  "Aps", //  2
  "Aql", //  3
  "Aqr", //  4
  "Ara", //  5
  "Ari", //  6
  "Aur", //  7
  "Boo", //  8
  "CMa", //  9
  "CMi", //  10
  "CVn", //  11
  "Cae", //  12
  "Cam", //  13
  "Cap", //  14
  "Car", //  15
  "Cas", //  16
  "Cen", //  17
  "Cep", //  18
  "Cet", //  19
  "Cha", //  20
  "Cir", //  21
  "Cnc", //  22
  "Col", //  23
  "Com", //  24
  "CrA", //  25
  "CrB", //  26
  "Crt", //  27
  "Cru", //  28
  "Crv", //  29
  "Cyg", //  30
  "Del", //  31
  "Dor", //  32
  "Dra", //  33
  "Equ", //  34
  "Eri", //  35
  "For", //  36
  "Gem", //  37
  "Gru", //  38
  "Her", //  39
  "Hor", //  40
  "Hya", //  41
  "Hyi", //  42
  "Ind", //  43
  "LMi", //  44
  "Lac", //  45
  "Leo", //  46
  "Lep", //  47
  "Lib", //  48
  "Lup", //  49
  "Lyn", //  50
  "Lyr", //  51
  "Men", //  52
  "Mic", //  53
  "Mon", //  54
  "Mus", //  55
  "Nor", //  56
  "Oct", //  57
  "Oph", //  58
  "Ori", //  59
  "Pav", //  60
  "Peg", //  61
  "Per", //  62
  "Phe", //  63
  "Pic", //  64
  "PsA", //  65
  "Psc", //  66
  "Pup", //  67
  "Pyx", //  68
  "Ret", //  69
  "Scl", //  70
  "Sco", //  71
  "Sct", //  72
  "Ser", //  73
  "Sex", //  74
  "Sge", //  75
  "Sgr", //  76
  "Tau", //  77
  "Tel", //  78
  "TrA", //  79
  "Tri", //  80
  "Tuc", //  81
  "UMa", //  82
  "UMi", //  83
  "Vel", //  84
  "Vir", //  85
  "Vol", //  86
  "Vul", //  87
  "Unknown" // 88
};

// Catalog names. 
const char * Txt_Catalog[] = {
  "Star ",
  "M ",
  "NGC ",
  "NGC ",
  "IC ",
  "None "
};

// Data for different catalogs, each is a collection of certain celestial objects
// These are machine generated using scripts from Open NGC and KStars' Star data
// Do NOT edit manually. Rerun the scripts instead. 
#include "messier.h"
#include "herschel.h"
#include "stars.h"
#include "ngc.h"
#include "ic.h"



// ----------------------------------------------------------
// Catalog Manager

template <typename T> void PROGMEM_readAnything(const T * sce, T& dest)
{
  memcpy_P(&dest, sce, sizeof(T));
}

template <typename T> T PROGMEM_getAnything(const T * sce)
{
  static T temp;
  memcpy_P(&temp, sce, sizeof(T));
  return temp;
}

// initialization
void CatMgr::setLat(double lat) {
  _lat=lat;
  if (lat<9999) {
    _cosLat=cos(lat/Rad);
    _sinLat=sin(lat/Rad);
  }
}

void CatMgr::setLstT0(double lstT0) {
  _lstT0=lstT0;
  _mil = millis();
}

double CatMgr::getLst()
{
  unsigned long deltat = millis() - _mil;
  double lst = (_lstT0 + (double)deltat / 3600000.0)*1.0027;
  return  lst;
}

void CatMgr::select(Catalog cat) {
  //first free memory!!
  if (_cat != cat)
  {
    memcpy(&_active_starCat, &_null_starCat, sizeof(star_t));
    memcpy(&_active_dsoCat, &_null_dsoCat, sizeof(dso_t));
  }
  _cat = cat;
  switch (_cat)
  {
  case STAR:
    _selected = 0;
    break;
  case MESSIER:
    _selected = 1;
    break;
  case HERSCHEL:
    _selected = 2;
    break;
  case NGC:
    _selected = 3;
    break;
  case IC:
    _selected = 4;
    break;
  }
}

Catalog CatMgr::getCat() {
  return _cat;
}

const char* CatMgr::catalogStr() {
  return Txt_Catalog[_selected];
}

// catalog filtering
bool CatMgr::canFilter() {
  return ((_lat<9999) && (_lstT0!=0));
}

void CatMgr::filter(FilterMode fm) {
  _fm=fm;
}

bool CatMgr::isFiltered() {
  double a,z,ar,ad;
  if (!canFilter()) return false;
  switch (_fm) {
    case FM_NONE:
      return false;
    break;
    case FM_ABOVE_HORIZON:
      EquToHor(ra(),dec(),&a,&z);
      return a<0.0;
    break;
    case FM_ALIGN_ALL_SKY:
      // minimum 10 degrees altitude (to limit unlikely align stars and to minimime refraction effects)
      EquToHor(ra(),dec(),&a,&z);
      if (a<10.0) return true;
      // minimum 5 degrees from the pole (for accuracy)
      if (_lat>=0.0) {
        if (dec()>85.0) return true;
      } else {
        if (dec()<-85.0) return true;
      }
      return false;
    break;
    case FM_ALIGN_3STAR_1:
      // minimum 10 degrees altitude (to limit unlikely align stars and to minimime refraction effects)
      EquToHor(ra(),dec(),&a,&z);
      if (a<10.0) return true;
      // maximum 30 degrees from align location
      ar = HAToRA(-1.0 * 15.0);
      if (_lat >= 0.0) ad = 10.0; else ad = -10.0;
      if (DistFromEqu(ar,ad)>30.0) return true;
      // on our side of meridian
      if (ha()>0) return true;
      return false;
    break;
    case FM_ALIGN_3STAR_2:
      // minimum 10 degrees altitude (to limit unlikely align stars and to minimime refraction effects)
      EquToHor(ra(),dec(),&a,&z);
      if (a<10.0) return true;
      // maximum 30 degrees from align location
      ar = HAToRA(+1.0 * 15.0);
      if (_lat >= 0.0) ad = 10.0; else ad = -10.0;
      if (DistFromEqu(ar,ad)>30.0) return true;
      // on our side of meridian
      if (ha()<0) return true;
      return false;
    break;
    case FM_ALIGN_3STAR_3:
      // minimum 10 degrees altitude (to limit unlikely align stars and to minimize refraction effects)
      EquToHor(ra(),dec(),&a,&z);
      if (a<10.0) return true;
      // maximum 30 degrees from align location
      ar = HAToRA(+6.0 * 15.0);
      if (_lat >= 0.0) ad = 45.0; else ad = -45.0;
      if (DistFromEqu(ar,ad)>30.0) return true;
      return false;
    break;
  }
}

// select catalog record
void CatMgr::setIndex(int index) {
  _idx[_selected]=index;
  read();
  decIndex();
  incIndex();
}

int CatMgr::getIndex() {
  return _idx[_selected];
}

int CatMgr::getMaxIndex() {
  return _maxIdx[_selected];
}

void CatMgr::read()
{
  switch (_cat)
  {
  case STAR:
    PROGMEM_readAnything(&Cat_Stars[_idx[_selected]], _active_starCat);
    break;
  case MESSIER:
    PROGMEM_readAnything(&Cat_Messier[_idx[_selected]], _active_dsoCat);
    break;
  case HERSCHEL:
    PROGMEM_readAnything(&Cat_Herschel[_idx[_selected]], _active_dsoCat);
    break;
  case NGC:
    PROGMEM_readAnything(&Cat_NGC[_idx[_selected]], _active_dsoCat);
    break;
  case IC:
    PROGMEM_readAnything(&Cat_IC[_idx[_selected]], _active_dsoCat);
    break;
  }
}

void CatMgr::incIndex() {
  int i=_maxIdx[_selected]+1;
  do {
    i--;
    _idx[_selected]++;
    if (_idx[_selected]>_maxIdx[_selected])
      _idx[_selected]=0;
    read();
  } while (isFiltered() && (i>0));
}


void CatMgr::decIndex() {
  int i=_maxIdx[_selected]+1;
  do {
    i--;
    _idx[_selected]--;
    if (_idx[_selected]<0)
      _idx[_selected]=_maxIdx[_selected];
    read();
  } while (isFiltered() && (i>0));
}

// get catalog contents

// RA in degrees
double CatMgr::ra() {
  double f;
  if (_cat==STAR)     f = _active_starCat.RA; else
  if (_cat==MESSIER)  f = _active_dsoCat.RA; else
  if (_cat==NGC)      f = _active_dsoCat.RA; else
  if (_cat==IC)       f = _active_dsoCat.RA; else
  if (_cat==HERSCHEL) f = _active_dsoCat.RA; else f=0;
  f /= ra_cf;
  return f;
}


// HA in degrees
double CatMgr::ha() {
  if (!canFilter()) return 0;
  double lstH = getLst();
  double h=(lstH*15.0-ra());
  while (h>180.0) h-=360.0;
  while (h<-180.0) h+=360.0;
  return h;
}

void CatMgr::raHMS(uint8_t& h, uint8_t& m, uint8_t& s) {
  double f = ra()/15.;
  double h1,m1,s1;

  h1=floor(f);
  m1=(f-h1)*60;
  s1=(m1-floor(m1));

  s1=s1*60.0;

  h = (int)h1;
  m = (int)m1;
  s = (int)s1;
}

// Dec in degrees
double CatMgr::dec() {
  double f;
  if (_cat==STAR)     f = _active_starCat.DE; else
  if (_cat==MESSIER)  f = _active_dsoCat.DE; else
  if (_cat==NGC)      f = _active_dsoCat.DE; else
  if (_cat==IC)       f = _active_dsoCat.DE; else
  if (_cat==HERSCHEL) f = _active_dsoCat.DE; else f=0;
  f /= de_cf;
  return f;
}

void CatMgr::decDMS(short& d, uint8_t& m, uint8_t& s) {
  double f = dec();
  double d1, m1, s1;
  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1;
  if (f<0) {
    d *= -1;
  }

  m = (int)m1;
  s = (int)s1;
}

int CatMgr::epoch() {
  if (_cat==STAR) return 2000; else
  if (_cat==MESSIER) return 2000; else
  if (_cat==NGC) return 2000; else
  if (_cat==IC) return 2000; else
  if (_cat==HERSCHEL) return 2000; else return 0;
}

// Alt in degrees
double CatMgr::alt() {
  double a,z;
  EquToHor(ra(),dec(),&a,&z);
  return a;
}

// Azm in degrees
double CatMgr::azm() {
  double a,z;
  EquToHor(ra(),dec(),&a,&z);
  return z;
}

double CatMgr::magnitude() {
  double m=250;
  if (_cat==STAR)     m = _active_starCat.Mag; else
  if (_cat==MESSIER)  m = _active_dsoCat.Mag; else
  if (_cat==NGC)      m = _active_dsoCat.Mag; else
  if (_cat==IC)       m = _active_dsoCat.Mag; else
  if (_cat==HERSCHEL) m = _active_dsoCat.Mag;
  return (m-20)/10.0;
}

byte CatMgr::constellation() {
  if (_cat==STAR)     return _active_starCat.Cons; else
  if (_cat==MESSIER)  return _active_dsoCat.Cons; else
  if (_cat==NGC)      return _active_dsoCat.Cons; else
  if (_cat==IC)       return _active_dsoCat.Cons; else
  if (_cat==HERSCHEL) return _active_dsoCat.Cons; else return 89;
}

const char* CatMgr::constellationStr() {
  return Txt_Constellations[constellation()];
}

byte CatMgr::objectType() {
  if (_cat==MESSIER)  return _active_dsoCat.Obj_type; else
  if (_cat==NGC)      return _active_dsoCat.Obj_type; else
  if (_cat==IC)       return _active_dsoCat.Obj_type; else
  if (_cat==HERSCHEL) return _active_dsoCat.Obj_type; else return -1;
}

const char* CatMgr::objectTypeStr() {
  if (_cat==STAR)     return "Star"; else
  if (_cat==MESSIER)  return Txt_Object_Type[objectType()]; else
  if (_cat==NGC)      return Txt_Object_Type[objectType()]; else
  if (_cat==IC)       return Txt_Object_Type[objectType()]; else
  if (_cat==HERSCHEL) return Txt_Object_Type[objectType()]; else return "";
}

const char* CatMgr::objectName() {
  //if (_cat==STAR)     return Cat_Stars[_idx[_selected]].Name; else
  //if (_cat==MESSIER)  return "";
  //if (_cat==HERSCHEL) return "";
  return "";
}

int CatMgr::primaryId() {
  if (_cat==STAR)     return _active_starCat.Bayer + 1; else
  if (_cat==MESSIER)  return _active_dsoCat.Obj_id; else
  if (_cat==NGC)      return _active_dsoCat.Obj_id; else
  if (_cat==IC)       return _active_dsoCat.Obj_id; else
  if (_cat==HERSCHEL) return _active_dsoCat.Obj_id; else return -1;
}

// support functions
// convert equatorial coordinates to horizon, in degrees
void CatMgr::EquToHor(double RA, double Dec, double *Alt, double *Azm) {
  double lstH = getLst();
  double HA=(lstH*15.0-RA);

  while (HA<0.0)    HA=HA+360.0;
  while (HA>=360.0) HA=HA-360.0;
  HA =HA/Rad;
  Dec=Dec/Rad;
  double SinAlt = (sin(Dec) * _sinLat) + (cos(Dec) * _cosLat * cos(HA));  
  *Alt   = asin(SinAlt);
  double t1=sin(HA);
  double t2=cos(HA)*_sinLat-tan(Dec)*_cosLat;
  *Azm=atan2(t1,t2)*Rad;
  *Azm=*Azm+180.0;
  *Alt = *Alt*Rad;
}

// convert horizon coordinates to equatorial, in degrees
void CatMgr::HorToEqu(double Alt, double Azm, double *RA, double *Dec) { 
  double lstH = getLst();
  while (Azm<0)      Azm=Azm+360.0;
  while (Azm>=360.0) Azm=Azm-360.0;
  Alt  = Alt/Rad;
  Azm  = Azm/Rad;
  double SinDec = (sin(Alt) * _sinLat) + (cos(Alt) * _cosLat * cos(Azm));  
  *Dec = asin(SinDec); 
  double t1=sin(Azm);
  double t2=cos(Azm)*_sinLat-tan(Alt)*_cosLat;
  double HA=atan2(t1,t2)*Rad;
  HA=HA+180.0;
  *Dec = *Dec*Rad;

  while (HA<0.0)    HA=HA+360.0;
  while (HA>=360.0) HA=HA-360.0;
  *RA=(lstH*15.0-HA);
}

// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
double CatMgr::TrueRefrac(double Alt, double Pressure, double Temperature) {
  double TPC=(Pressure/1010.0) * (283.0/(273.0+Temperature));
  double r=( ( 1.02*cot( (Alt+(10.3/(Alt+5.11)))/Rad ) ) ) * TPC;  if (r<0.0) r=0.0;
  return r;
}

double CatMgr::cot(double n) {
  return 1.0/tan(n);
}

// angular distance from current Equ coords, in degrees
double CatMgr::DistFromEqu(double RA, double Dec) {
  RA=RA/Rad; Dec=Dec/Rad;
  return acos( sin(dec()/Rad)*sin(Dec) + cos(dec()/Rad)*cos(Dec)*cos(ra()/Rad - RA))*Rad;
}

// convert an HA to RA, in degrees
double CatMgr::HAToRA(double HA) {
  double lstH = getLst();
  return (lstH*15.0-HA);
}

CatMgr cat_mgr;
