#include <Arduino.h>
#include <math.h>
#include "Config.h"
#include "TeenAstroCatalog.h"
#include "CatalogTypes.h"
#include "CatalogConfig.h"

// --------------------------------------------------------------------------------
// Catalog Manager Helper

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
// --------------------------------------------------------------------------------
// Catalog Manager

// initialization
void CatMgr::setLat(double lat) {
  _lat=lat;
  if (lat<9999) {
    _cosLat=cos(lat/Rad);
    _sinLat=sin(lat/Rad);
  }
}

// Set Local Sidereal Time, and number of milliseconds
void CatMgr::setLstT0(double lstT0) {
  _lstT0=lstT0;
  _lstMillisT0=millis();
}

// Set last Tele RA/Dec
void CatMgr::setLastTeleEqu(double RA, double Dec) {
  _lastTeleRA=RA;
  _lastTeleDec=Dec;
}

bool CatMgr::isInitialized() {
  return ((_lat<9999) && (_lstT0!=0));
}

// Get Local Sidereal Time, converted from hours to degrees
double CatMgr::lstDegs() {
  return (lstHours()*15.0);
}

// Get Local Sidereal Time, in hours, and adjust for time inside the menus
double CatMgr::lstHours() {
  double msSinceT0=(unsigned long)(millis()-_lstMillisT0);
  // Convert from Solar to Sidereal
  double siderealSecondsSinceT0=(msSinceT0/1000.0)*1.00277778;
  return _lstT0+siderealSecondsSinceT0/3600.0;
}

gen_star_t*       _genStarCatalog      = NULL;
gen_star_vcomp_t* _genStarVCompCatalog = NULL;
dbl_star_t*       _dblStarCatalog      = NULL;
dbl_star_comp_t*  _dblStarCompCatalog  = NULL;
var_star_t*       _varStarCatalog      = NULL;
var_star_comp_t*  _varStarCompCatalog  = NULL;
dso_t*            _dsoCatalog          = NULL;
dso_comp_t*       _dsoCompCatalog      = NULL;
dso_vcomp_t*      _dsoVCompCatalog     = NULL;

gen_star_t       _genStarObject      = { 0,0,0,0,0,0,0,0 };
gen_star_vcomp_t _genStarVCompObject = { 0,0,0,0,0,0,0 };
dbl_star_t       _dblStarObject      = { 0,0,0,0,0,0,0,0,0,0,0 };
dbl_star_comp_t  _dblStarCompObject  = { 0,0,0,0,0,0,0,0,0,0,0 };
var_star_t       _varStarObject      = { 0,0,0,0,0,0,0,0,0,0 };
var_star_comp_t  _varStarCompObject  = { 0,0,0,0,0,0,0,0,0,0 };
dso_t            _dsoObject          = { 0,0,0,0,0,0,0,0 };
dso_comp_t       _dsoCompObject      = { 0,0,0,0,0,0,0,0 };
dso_vcomp_t      _dsoVCompObject     = { 0,0,0,0,0,0,0 };

// handle catalog selection (0..n)
void CatMgr::select(int number) {
  _genStarCatalog      =NULL;
  _genStarVCompCatalog =NULL;
  _dblStarCatalog      =NULL;
  _dsoCatalog          =NULL;
  _dsoCompCatalog      =NULL;
  _dsoVCompCatalog     =NULL;
  if ((number<0) || (number>=NUM_CAT)) number=-1; // invalid catalog?
  _selected=number;
  if (_selected>=0) {
    if (catalog[_selected].CatalogType==CAT_GEN_STAR)       _genStarCatalog     =(gen_star_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_GEN_STAR_VCOMP) _genStarVCompCatalog=(gen_star_vcomp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DBL_STAR)       _dblStarCatalog     =(dbl_star_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DBL_STAR_COMP)  _dblStarCompCatalog =(dbl_star_comp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_VAR_STAR)       _varStarCatalog     =(var_star_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_VAR_STAR_COMP)  _varStarCompCatalog =(var_star_comp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DSO)            _dsoCatalog         =(dso_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DSO_COMP)       _dsoCompCatalog     =(dso_comp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DSO_VCOMP)      _dsoVCompCatalog    =(dso_vcomp_t*)catalog[_selected].Objects; else _selected=-1;
  }
}


void CatMgr::read()
{

  switch (catalogType())
  {
  case CAT_GEN_STAR:
    PROGMEM_readAnything(&_genStarCatalog[catalog[_selected].Index], _genStarObject);    
    break;
  case CAT_GEN_STAR_VCOMP:
    PROGMEM_readAnything(&_genStarVCompCatalog[catalog[_selected].Index], _genStarVCompObject);
    break;
  case CAT_DBL_STAR:
    PROGMEM_readAnything(&_dblStarCatalog[catalog[_selected].Index], _dblStarObject);
    break;
  case CAT_DBL_STAR_COMP:
    PROGMEM_readAnything(&_dblStarCompCatalog[catalog[_selected].Index], _dblStarCompObject);
    break;
  case CAT_VAR_STAR:
    PROGMEM_readAnything(&_varStarCatalog[catalog[_selected].Index], _varStarObject);
    break;
  case CAT_VAR_STAR_COMP:
    PROGMEM_readAnything(&_varStarCompCatalog[catalog[_selected].Index], _varStarCompObject);
    break;
  case CAT_DSO:
    PROGMEM_readAnything(&_dsoCatalog[catalog[_selected].Index], _dsoObject);
    break;
  case CAT_DSO_COMP:
    PROGMEM_readAnything(&_dsoCompCatalog[catalog[_selected].Index], _dsoCompObject);
    break;
  case CAT_DSO_VCOMP:
    PROGMEM_readAnything(&_dsoVCompCatalog[catalog[_selected].Index], _dsoVCompObject);
    break;
  }


}

//  Get active catalog type
CAT_TYPES CatMgr::catalogType()  {
  return catalog[_selected].CatalogType;
}

// check so see if there's a double star catalog present
bool CatMgr::hasDblStarCatalog() {
  for (int i=0; i<MaxCatalogs; i++) {
    if (catalog[i].CatalogType==CAT_DBL_STAR) return true;
    if (catalog[i].CatalogType==CAT_DBL_STAR_COMP) return true;
    if (catalog[i].CatalogType==CAT_NONE) break;
  }
  return false;
}

// check so see if theres a variable star catalog present
bool CatMgr::hasVarStarCatalog() {
  for (int i=0; i<MaxCatalogs; i++) {
    if (catalog[i].CatalogType==CAT_VAR_STAR) return true;
    if (catalog[i].CatalogType==CAT_NONE) break;
  }
  return false;
}

// check to see if the primaryId() was moved to the prefix
bool CatMgr::hasPrimaryIdInPrefix() {
  const char *s=catalog[_selected].Prefix;
  return strstr(s,";");
}

bool CatMgr::isStarCatalog() {
  return (catalogType()==CAT_GEN_STAR) || (catalogType()==CAT_GEN_STAR_VCOMP) || 
         (catalogType()==CAT_DBL_STAR) || (catalogType()==CAT_DBL_STAR_COMP) ||
         (catalogType()==CAT_VAR_STAR) || (catalogType()==CAT_VAR_STAR_COMP);
}

bool CatMgr::isDblStarCatalog() {
  if (catalogType()==CAT_DBL_STAR) return true;
  if (catalogType()==CAT_DBL_STAR_COMP) return true;
  return false;
}

bool CatMgr::isVarStarCatalog() {
  if (catalogType()==CAT_VAR_STAR) return true;
  if (catalogType()==CAT_VAR_STAR_COMP) return true;
  return false;
}

bool CatMgr::isDsoCatalog()  {
  return (catalogType()==CAT_DSO) || (catalogType()==CAT_DSO_COMP) || (catalogType()==CAT_DSO_VCOMP);
}

// Get active catalog title
const char* CatMgr::catalogTitle() {
  if (_selected<0) return "";

  char *subMenu=strstr(catalog[_selected].Title,">");
  if (subMenu) {
    return &subMenu[1];
  } else return catalog[_selected].Title;

  return catalog[_selected].Title;
}

// Get active catalog submenu
const char* CatMgr::catalogSubMenu() {
  if (_selected<0) return "";
  static char thisSubMenu[16];
  
  strcpy(thisSubMenu,catalog[_selected].Title);
  char *subMenu=strstr(thisSubMenu,">");
  if (subMenu) {
    subMenu[0]=0;
    return thisSubMenu;
  } else return "";
}

// Get active catalog prefix
const char* CatMgr::catalogPrefix() {
  if (_selected<0) return "";
  const char *s=catalog[_selected].Prefix;
  
  // array type prefix?
  if (strstr(s,";")) {
    const char *s1;
    long p=primaryId();
    if (p>=0) {
      s1=getElementFromString(s,p);
      if (strlen(s1)>0) return s1; else {
        s1=getElementFromString(s,0);
        static char s2[24];
        sprintf(s2,"%s%ld",s1,p);
        return s2;
      }
    } else return "?";
  } else return s;
}

// catalog filtering
void CatMgr::filtersClear() {
  _fm=FM_NONE;
}

void CatMgr::filterAdd(int fm) {
  _fm|=fm;
}

void CatMgr::filterAdd(int fm, int param) {
  _fm|=fm;
  if (fm&FM_CONSTELLATION) _fm_con=param;
  if (fm&FM_BY_MAG) {
    if (param==0) _fm_mag_limit=10.0; else
    if (param==1) _fm_mag_limit=11.0; else
    if (param==2) _fm_mag_limit=12.0; else
    if (param==3) _fm_mag_limit=13.0; else
    if (param==4) _fm_mag_limit=14.0; else
    if (param==5) _fm_mag_limit=15.0; else
    if (param==6) _fm_mag_limit=16.0; else _fm_mag_limit=100.0;
  }
  if (fm&FM_ABOVE_HORIZON) {
    if (param==0) _fm_horizon_limit=0.0; else
    if (param==1) _fm_horizon_limit=10.0; else
    if (param==2) _fm_horizon_limit=20.0; else
    if (param==3) _fm_horizon_limit=30.0; else
    if (param==4) _fm_horizon_limit=40.0; else
    if (param==5) _fm_horizon_limit=50.0; else
    if (param==6) _fm_horizon_limit=60.0; else
    if (param==7) _fm_horizon_limit=70.0; else  _fm_horizon_limit=0;
  }
  if (fm&FM_NEARBY) {
    if (param==0) _fm_nearby_dist=1.0; else
    if (param==1) _fm_nearby_dist=5.0; else
    if (param==2) _fm_nearby_dist=10.0; else
    if (param==3) _fm_nearby_dist=15.0; else _fm_nearby_dist=9999.0;
  }
  if (fm&FM_OBJ_TYPE) _fm_obj_type=param;
  if (fm&FM_DBL_MIN_SEP) {
    if (param==0) _fm_dbl_min=0.2; else
    if (param==1) _fm_dbl_min=0.5; else
    if (param==2) _fm_dbl_min=1.0; else
    if (param==3) _fm_dbl_min=1.5; else
    if (param==4) _fm_dbl_min=2.0; else
    if (param==5) _fm_dbl_min=3.0; else
    if (param==6) _fm_dbl_min=5.0; else
    if (param==7) _fm_dbl_min=10.0; else
    if (param==8) _fm_dbl_min=20.0; else
    if (param==9) _fm_dbl_min=50.0; else _fm_dbl_min=0.0;
  }
  if (fm&FM_DBL_MAX_SEP) {
    if (param==0) _fm_dbl_max=0.5; else
    if (param==1) _fm_dbl_max=1.0; else
    if (param==2) _fm_dbl_max=1.5; else
    if (param==3) _fm_dbl_max=2.0; else
    if (param==4) _fm_dbl_max=3.0; else
    if (param==5) _fm_dbl_max=5.0; else
    if (param==6) _fm_dbl_max=10.0; else
    if (param==7) _fm_dbl_max=20.0; else
    if (param==8) _fm_dbl_max=50.0; else
    if (param==9) _fm_dbl_max=100.0; else _fm_dbl_max=0.0;
  }
  if (fm&FM_VAR_MAX_PER) {
    if (param==0) _fm_var_max=0.5; else
    if (param==1) _fm_var_max=1.0; else
    if (param==2) _fm_var_max=2.0; else
    if (param==3) _fm_var_max=5.0; else
    if (param==4) _fm_var_max=10.0; else
    if (param==5) _fm_var_max=20.0; else
    if (param==6) _fm_var_max=50.0; else
    if (param==7) _fm_var_max=100.0; else _fm_var_max=0.0;  }
}

// checks to see if the currently selected catalog has an active filter
bool CatMgr::hasActiveFilter() {
  if (!isInitialized()) return false;
  if (_fm==FM_NONE) return false;
  if (_fm_horizon_limit != 0) return true; // doesn't apply to this indication
  if (_fm & FM_CONSTELLATION) return true;
  if (isDsoCatalog() && (_fm & FM_OBJ_TYPE)) return true;
  if (_fm & FM_BY_MAG) return true;
  if (_fm & FM_NEARBY) return true;
  if (isDblStarCatalog() && (_fm & FM_DBL_MAX_SEP)) return true;
  if (isDblStarCatalog() && (_fm & FM_DBL_MIN_SEP)) return true;
  if (isVarStarCatalog() && (_fm & FM_VAR_MAX_PER)) return true;
  return false;
}  

// checks to see if the currently selected object is filtered (returns true if filtered out)
bool CatMgr::isFiltered() {
  double current_alt = 0;
  if (!isInitialized()) return false;

  if (_fm & FM_CONSTELLATION) {
    if (constellation()!=_fm_con) return true;
  }
  if (_fm & FM_OBJ_TYPE) {
    if (isDsoCatalog() && (objectType()!=_fm_obj_type)) return true;
  }
  if (_fm & FM_BY_MAG) {
    if (magnitude()>=_fm_mag_limit) return true;
  }
  if (_fm & FM_NEARBY) {
    if (DistFromEqu(_lastTeleRA,_lastTeleDec)>=_fm_nearby_dist) return true;
  }
  if (_fm & FM_DBL_MAX_SEP) {
    if (isDblStarCatalog() && ((separation()>_fm_dbl_max) || (separation()<0))) return true;
  }
  if (_fm & FM_DBL_MIN_SEP) {
    if (isDblStarCatalog() && ((separation()<_fm_dbl_min) || (separation()<0))) return true;
  }
  if (_fm & FM_VAR_MAX_PER) {
    if (isVarStarCatalog() && ((period()>_fm_var_max) || (period()<0))) return true;
  }
  current_alt = alt();
  if (current_alt < _fm_horizon_limit) return true;
  if (_fm == FM_NONE) return false;
  if (_fm & FM_ALIGN_ALL_SKY) {
    if (current_alt < 10.0) return true;      // minimum 10 degrees altitude
    if (abs(dec()) > 85.0) return true; // minimum 5 degrees from the pole (for accuracy)
  }

  return false;
}  

// select catalog record
bool CatMgr::setIndex(long index) {
  if (_selected<0) return false;
  catalog[_selected].Index=index;
  read();
  decIndex();
  return incIndex();
}

long CatMgr::getIndex() {
  return catalog[_selected].Index;
}

long CatMgr::getMaxIndex() {
  return catalog[_selected].NumObjects-1;
}

bool CatMgr::incIndex() {
  long i=getMaxIndex()+1;
  do {
    i--;
    catalog[_selected].Index++;
    if (catalog[_selected].Index>getMaxIndex()) catalog[_selected].Index=0;
    read();
  } while (isFiltered() && (i>0));
  if (isFiltered()) return false; else return true;
}

bool CatMgr::decIndex() {
  long i=getMaxIndex()+1;
  do {
    i--;
    catalog[_selected].Index--;
    if (catalog[_selected].Index<0) catalog[_selected].Index=getMaxIndex();
    read();
  } while (isFiltered() && (i>0));
  if (isFiltered()) return false; else return true;
}

// get catalog contents

// RA, converted from hours to degrees
double CatMgr::ra() {
  return rah()*15.0;
}

// RA in hours
double CatMgr::rah() {
  if (_selected<0) return 0;
  if (catalogType()==CAT_GEN_STAR)       return _genStarObject.RA; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return _genStarVCompObject.RA/2730.6666666666666; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarObject.RA; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompObject.RA/2730.6666666666666; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarObject.RA; else
  if (catalogType()==CAT_VAR_STAR_COMP)  return _varStarCompObject.RA/2730.6666666666666; else
  if (catalogType()==CAT_DSO)            return _dsoObject.RA; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompObject.RA/2730.6666666666666; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompObject.RA/2730.6666666666666; else return 0;
}

// HA in degrees
double CatMgr::ha() {
  if (!isInitialized()) return 0;
  double h=(lstDegs()-ra());
  while (h>180.0) h-=360.0;
  while (h<-180.0) h+=360.0;
  return h;
}

// Get RA of an object, in hours, minutes, seconds
void CatMgr::raHMS(uint8_t &h, uint8_t &m, uint8_t &s) {
  double f=rah();
  double h1,m1,s1;

  h1=floor(f);
  m1=(f-h1)*60;
  s1=(m1-floor(m1))*60.0;

  h = (int)h1;
  m = (int)m1;
  s = (int)s1;
}

// Dec in degrees
double CatMgr::dec() {
  if (catalogType()==CAT_GEN_STAR)       return _genStarObject.DE; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return _genStarVCompObject.DE/364.07777777777777; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarObject.DE; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompObject.DE/364.07777777777777; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarObject.DE; else
  if (catalogType()==CAT_VAR_STAR_COMP)  return _varStarCompObject.DE/364.07777777777777; else
  if (catalogType()==CAT_DSO)            return _dsoObject.DE; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompObject.DE/364.07777777777777; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompObject.DE/364.07777777777777; else return 0;
}

// Declination as degrees, minutes, seconds
void CatMgr::decDMS(short& d, uint8_t& m, uint8_t& s) {
  double f=dec();
  double d1, m1, s1;
  int sign=1; if (f<0) sign=-1;

  f=f*sign;
  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1*sign;
  m = (int)m1;
  s = (int)s1;
}

// Epoch for catalog
int CatMgr::epoch() {
  if (_selected<0) return -1;
  return catalog[_selected].Epoch;
}

// Alt in degrees
double CatMgr::alt() {
  double HA = lstDegs() - ra();
  HA = HA / Rad;
  double Dec = dec() / Rad;
  double SinAlt = (sin(Dec) * _sinLat) + (cos(Dec) * _cosLat * cos(HA));
  return asin(SinAlt) * Rad;
}

// Declination as degrees, minutes, seconds
void CatMgr::altDMS(short &d, uint8_t &m, uint8_t &s) {
  double f=alt();
  double d1, m1, s1;
  int sign=1; if (f<0) sign=-1;

  f=f*sign;
  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1*sign;
  m = (int)m1;
  s = (int)s1;
}

// Azm in degrees
double CatMgr::azm() {
  double a,z;
  EquToHor(ra(),dec(),&a,&z);
  return z;
}

// Get Azm of an object, in degrees, minutes, seconds
void CatMgr::azmDMS(short &d, uint8_t &m, uint8_t &s) {
  double f=azm();
  double d1,m1,s1;

  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1;
  m = (int)m1;
  s = (int)s1;
}

// apply refraction, this converts from the "Topocentric" to "Observed" place for higher accuracy, RA in hours Dec in degrees
void CatMgr::topocentricToObservedPlace(float *RA, float *Dec) {
  if (isInitialized()) {
    double Alt,Azm;
    double r=*RA*15.0;
    double d=*Dec;
    EquToHor(r,d,&Alt,&Azm);
    Alt = Alt+TrueRefrac(Alt) / 60.0;
    HorToEqu(Alt,Azm,&r,&d);
    *RA=r/15.0; *Dec=d;
  }
}

// Period of variable star, in days
// -2 = irregular, -1 = unknown
float CatMgr::period() {
  if (_selected<0) return -1;
  float p = -1;
  if (catalogType()==CAT_VAR_STAR) p= _varStarObject.Period; else
  if (catalogType()==CAT_VAR_STAR_COMP) p= _varStarCompObject.Period; else return -1;
  // Period 0.00 to 9.99 days (0 to 999) period 10.0 to 3186.6 days (1000 to 32766), 32766 = Irregular, 32767 = Unknown
  if ((p>=0)  && (p<=999)) return p/100.0; else
  if ((p>999) && (p<=32765)) return (p-900)/10.0; else
  if (p==32766) return -2; else return -1;
}

// Position angle of double star, in degrees
// -1 = Unknown
int CatMgr::positionAngle() {
  if (_selected<0) return -1;
  int p;
  if (catalogType()==CAT_DBL_STAR) p=_dblStarObject.PA; else
  if (catalogType()==CAT_DBL_STAR_COMP) p=_dblStarCompObject.PA; else return -1;
  // Position angle 0 to 360 degrees, 361 = Unknown
  if (p==361) return -1; else return p;
}

// Separation of double star, in arc-seconds
// -1 = Unknown
float CatMgr::separation() {
  if (_selected<0) return -1;
  float s;
  if (catalogType()==CAT_DBL_STAR) s=_dblStarObject.Sep/10.0; else
  if (catalogType()==CAT_DBL_STAR_COMP) s=_dblStarCompObject.Sep/10.0; else return -1;
  // Seperation 0 to 999.8 (0 to 9998) arc-seconds, 9999=unknown
  if (abs(s-999.9)<0.01) return -1; else return s;
}

// Magnitude of an object
// 99.9 = Unknown
float CatMgr::magnitude() {
  if (_selected<0) return 99.9;
  if (catalogType()==CAT_GEN_STAR)       return _genStarObject.Mag/100.0; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { int m=_genStarVCompObject.Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarObject.Mag/100.0; else
  if (catalogType()==CAT_DBL_STAR_COMP)  { int m=_dblStarCompObject.Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_VAR_STAR)       return _varStarObject.Mag/100.0; else
  if (catalogType()==CAT_VAR_STAR_COMP)  { int m=_varStarCompObject.Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_DSO)            return _dsoObject.Mag/100.0; else
  if (catalogType()==CAT_DSO_COMP)       { int m=_dsoCompObject.Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_DSO_VCOMP)      { int m=_dsoVCompObject.Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else return 99.9;
}

// Secondary magnitude of an star.  For double stars this is the magnitude of the secondary.  For variables this is the minimum brightness.
// 99.9 = Unknown
float CatMgr::magnitude2() {
  if (_selected<0) return 99.9;
  if (catalogType()==CAT_DBL_STAR)       return _dblStarObject.Mag2/100.0; else
  if (catalogType()==CAT_DBL_STAR_COMP)  { int m=_dblStarCompObject.Mag2; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_VAR_STAR)       return _varStarObject.Mag2/100.0; else
  if (catalogType()==CAT_VAR_STAR_COMP)  { int m=_varStarCompObject.Mag2; if (m==255) return 99.9; else return (m/10.0)-2.5; } return 99.9;
}

// Constellation number
// 89 = Unknown
byte CatMgr::constellation() {
  if (_selected<0) return 89;
  if (catalogType()==CAT_GEN_STAR)       return _genStarObject.Cons; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return _genStarVCompObject.Cons; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarObject.Cons; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompObject.Cons; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarObject.Cons; else
  if (catalogType()==CAT_VAR_STAR_COMP)  return _varStarCompObject.Cons; else
  if (catalogType()==CAT_DSO)            return _dsoObject.Cons; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompObject.Cons; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompObject.Cons; else return 89;
}

// Constellation string
const char* CatMgr::constellationStr() {
  return Txt_Constellations[constellation()];
}

// Constellation string, from constellation number
const char* CatMgr::constellationCodeToStr(int code) {
  if ((code>=0) && (code<=87)) return Txt_Constellations[code]; else return "";
}

// Object type code
byte CatMgr::objectType() {
  if (_selected<0) return -1;
  if (catalogType()==CAT_GEN_STAR)       return 2; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return 2; else
  if (catalogType()==CAT_DBL_STAR)       return 2; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return 2; else
  if (catalogType()==CAT_VAR_STAR)       return 2; else
  if (catalogType()==CAT_VAR_STAR_COMP)  return 2; else
  if (catalogType()==CAT_DSO)            return _dsoObject.Obj_type; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompObject.Obj_type; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompObject.Obj_type; else return -1;
}

// Object type string
const char* CatMgr::objectTypeStr() {
  int t=objectType();
  if ((t>=0) && (t<=20)) return Txt_Object_Type[t]; else return "";
}

// Object Type string, from code number
const char* CatMgr::objectTypeCodeToStr(int code) {
  if ((code>=0) && (code<=20)) return Txt_Object_Type[code]; else return "";
}

// Object name code (encoded by Has_name.)  Returns -1 if the object doesn't have a name code.
long CatMgr::objectName() {
  if (_selected<0) return -1;
  long val = 0;
  // does it have a name? if not just return
  if (catalogType()==CAT_GEN_STAR)       { val = _genStarObject.It_name; } else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { val = _genStarVCompObject.It_name; } else
  if (catalogType()==CAT_DBL_STAR)       { val = _dblStarObject.It_name; } else
  if (catalogType()==CAT_DBL_STAR_COMP)  { val = _dblStarCompObject.It_name; } else
  if (catalogType()==CAT_VAR_STAR)       { val = _varStarObject.It_name; } else
  if (catalogType()==CAT_VAR_STAR_COMP)  { val = _varStarCompObject.It_name; } else
  if (catalogType()==CAT_DSO)            { val = _dsoObject.It_name; } else
  if (catalogType()==CAT_DSO_COMP)       { val = _dsoCompObject.It_name; } else
  if (catalogType()==CAT_DSO_VCOMP)      { val = _dsoVCompObject.It_name; }
  return val - 1;
}

// Object name type string
const char* CatMgr::objectNameStr() {
  if (_selected<0) return "";
  long elementNum=objectName();
  if (elementNum>=0) return getElementFromString(catalog[_selected].ObjectNames,elementNum); else return "";
}

// Object Id
long CatMgr::primaryId() {
  long id=-1;
  if (_selected<0) return -1;
  if (catalogType()==CAT_GEN_STAR)       id=_genStarObject.Obj_id; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) id= catalog[_selected].Index+1; else
  if (catalogType()==CAT_DBL_STAR)       id=_dblStarObject.Obj_id; else
  if (catalogType()==CAT_DBL_STAR_COMP)  id=_dblStarCompObject.Obj_id; else
  if (catalogType()==CAT_VAR_STAR)       id=_varStarObject.Obj_id; else
  if (catalogType()==CAT_VAR_STAR_COMP)  id=_varStarCompObject.Obj_id; else
  if (catalogType()==CAT_DSO)            id=_dsoObject.Obj_id; else
  if (catalogType()==CAT_DSO_COMP)       id=_dsoCompObject.Obj_id; else
  if (catalogType()==CAT_DSO_VCOMP)      id=catalog[_selected].Index+1; else return -1;
  if (id<1) return -1;
  return id;
}

// Object note code (encoded by Has_note.)  Returns -1 if the object doesn't have a note code.
long CatMgr::subId() {
  if (_selected<0) return -1;
  long val = 0;
  // does it have a name? if not just return
  if (catalogType()==CAT_GEN_STAR)       { val = _genStarObject.It_subId; } else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { val = _genStarVCompObject.It_subId; } else
  if (catalogType()==CAT_DBL_STAR)       { val = _dblStarObject.It_subId; } else
  if (catalogType()==CAT_DBL_STAR_COMP)  { val = _dblStarCompObject.It_subId; } else
  if (catalogType()==CAT_VAR_STAR)       { val = _varStarObject.It_subId; } else
  if (catalogType()==CAT_VAR_STAR_COMP)  { val = _varStarCompObject.It_subId; } else
  if (catalogType()==CAT_DSO)            { val = _dsoObject.It_subId; } else
  if (catalogType()==CAT_DSO_COMP)       { val = _dsoCompObject.It_subId; } else
  if (catalogType()==CAT_DSO_VCOMP)      { val = _dsoVCompObject.It_subId; }
  return val -1;
}

// Object note string
const char* CatMgr::subIdStr() {
  if (_selected<0) return "";
  long elementNum=subId();
  if (elementNum>=0) return getElementFromString(catalog[_selected].ObjectSubIds,elementNum); else return "";
}

// For Bayer designated Stars 0 = Alp, etc. to 23. For Fleemstead designated Stars 25 = '1', etc.
int CatMgr::bayerFlam() {
  if (_selected<0) return -1;
  int bf=24;
  if (catalogType()==CAT_GEN_STAR)       bf=_genStarObject.BayerFlam; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) bf=_genStarVCompObject.BayerFlam; else
  if (catalogType()==CAT_DBL_STAR)       bf=_dblStarObject.BayerFlam; else
  if (catalogType()==CAT_DBL_STAR_COMP)  bf=_dblStarCompObject.BayerFlam; else
  if (catalogType()==CAT_VAR_STAR)       bf=_varStarObject.BayerFlam; else
  if (catalogType()==CAT_VAR_STAR_COMP)  bf=_varStarCompObject.BayerFlam; else return -1;
  if (bf==24) return -1; else return bf;
}

// For Bayer designated Stars return greek letter or Flamsteed designated stars return number
const char* CatMgr::bayerFlamStr() {
  if (_selected<0) return "";
  static char bfStr[3]="";
  int bfNum=bayerFlam();
  if ((bfNum>=0) && (bfNum<24)) {
    sprintf(bfStr,"%d",bfNum);
    return bfStr;
  } else
  if (bfNum>24) {
    sprintf(bfStr,"%d",bfNum-24);
    return bfStr;
  } else
  return "";
}

// support functions

// returns elementNum 'th element from the comma delimited string where the 0th element is the first etc.
const char* CatMgr::getElementFromString(const char *data, long elementNum) {
  static char result[40] = "";

  // find the string start index
  bool found=false;
  long j=0;
  long n=elementNum;
  long len=strlen(data);
  for (long i=0; i<len; i++) {
    if (n==0) { j=i; found=true; break; }
    if (data[i]==';') { n--; }
  }

  // return the string
  if (found) {
    long k=0;
    for (long i=j; i<len; i++) {
      result[k++]=data[i];
      if (result[k-1]==';') { result[k-1]=0; break; }
      if (i==len-1) { result[k]=0; break; }
    }
    return result;
  } else return "";
}

// angular distance from current Equ coords, in degrees
double CatMgr::DistFromEqu(double RA, double Dec) {
  RA=RA/Rad; Dec=Dec/Rad;
  return acos( sin(dec()/Rad)*sin(Dec) + cos(dec()/Rad)*cos(Dec)*cos(ra()/Rad - RA))*Rad;
}

// convert an HA to RA, in degrees
double CatMgr::HAToRA(double HA) {
  return (lstDegs()-HA);
}

// convert equatorial coordinates to horizon, in degrees
void CatMgr::EquToHor(double RA, double Dec, double *Alt, double *Azm) {
  double HA=lstDegs()-RA;
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
  *RA=(lstDegs()-HA);
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

CatMgr cat_mgr;
