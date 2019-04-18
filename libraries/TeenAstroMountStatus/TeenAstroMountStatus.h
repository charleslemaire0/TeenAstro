#pragma once
#include <Arduino.h>

class TeenAstroMountStatus
{
public:

  enum Errors { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE, ERR_DEC, ERR_AZM, ERR_UNDER_POLE, ERR_MERIDIAN, ERR_SYNC };
  enum AlignMode { ALIM_ONE, ALIM_TWO, ALIM_THREE };
  enum AlignState { ALI_OFF, ALI_SELECT, ALI_SLEW, ALI_RECENTER };
  enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
  enum TrackState { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
  enum RateCompensation { RC_NONE, RC_REFR_RA, RC_REFR_BOTH, RC_FULL_RA, RC_FULL_BOTH };
  enum SideralMode { SID_STAR, SID_SUN, SID_MOON };
  enum ParkState { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
  enum PierState { PIER_E, PIER_W, PIER_UNKNOW };
private:
  AlignState      m_align = ALI_OFF;
  AlignMode       m_aliMode = ALIM_ONE;
  int             m_alignStar = 0;
  char            m_TempRa[15] = "";
  char            m_TempDec[15] = "";
  unsigned long   m_lastStateRaDec;
  char            m_TempAz[15] = "";
  char            m_TempAlt[15] = "";
  unsigned long   m_lastStateAzAlt;
  char            m_TempUTC[15] = "";
  char            m_TempSideral[15] = "";
  unsigned long   m_lastStateTime;
  char            m_TempMount[17] = "";
  unsigned long   m_lastStateMount;
  char            m_TempFocuser[45] = "";
  unsigned long   m_lastStateFocuser;
public:
  //Alignment Stuff
  bool      isAligning()  { return m_align != ALI_OFF; }
  bool      isAlignSlew() { return m_align == ALI_SLEW; };
  bool      isAlignSelect() { return m_align == ALI_SELECT; };
  bool      isAlignRecenter() { return m_align == ALI_RECENTER; };
  void      stopAlign() { m_align = ALI_OFF; m_alignStar = 0; return; };
  void      startAlign(AlignMode in)
            { m_aliMode = in;  m_align = ALI_SELECT; m_alignStar; m_alignStar = 1; return; };
  void      nextStepAlign();
  void      backStepAlign();
  bool      isLastStarAlign() { return (int)m_aliMode == m_alignStar;  };
  AlignMode getAlignMode() { return m_aliMode; };
  unsigned short  alignSelectedStar = 1;
  int             alignMaxNumStars = -1;
  //
  const char* getRa() { return  m_TempRa; };
  const char* getDec() { return  m_TempDec; };
  const char* getAz() { return  m_TempAz; };
  const char* getAlt() { return  m_TempAlt; };
  const char* getUTC() { return m_TempUTC; };
  const char* getSideral() { return m_TempSideral; };
  const char* getFocuser() { return m_TempFocuser; };

  int connectionFailure = 0;
  bool hasInfoRa = false;
  bool hasInfoDec = false;
  bool hasInfoAz = false;
  bool hasInfoAlt = false;
  bool hasInfoUTC = false;
  bool hasInfoSideral = false;
  bool hasInfoMount = false;
  bool hasInfoFocuser = false;
public: 

public:

  unsigned long lastState;
  void updateRaDec();
  void updateAzAlt();
  void updateTime();
  void updateFocuser();
  void updateMount();
  Mount getMount();
  ParkState getParkState();
  TrackState getTrackingState();
  SideralMode getSideralMode();
  double getLstT0();
  double getLat();
  bool atHome();
  bool isPulseGuiding();
  bool isGuidingN();
  bool isGuidingS();
  bool isGuidingE();
  bool isGuidingW();
  bool isGNSSValid();
  bool connected();
  bool notResponding();
  PierState getPierState();
  Errors getError();
  void addStar();

  //***********************
//  bool update()
//  {
//    if (millis() - lastupdate< 500)
//      return _valid;
//    lastupdate = millis();
//    char s[20] = "";
//    if (!_valid) {
//      Ser.print(":GVP#");
//      s[Ser.readBytesUntil('#', s, 20)] = 0;
//      if ((s[0] == 0) || (!strstr(s, "TeenAstro"))) { _valid = false; return false; }
//      strcpy(_id, s);
//      Ser.print(":GVN#");
//      s[Ser.readBytesUntil('#', s, 20)] = 0;
//      if (s[0] == 0) { _valid = false; return false; }
//      strcpy(_ver, s);
//    }
//
//    Ser.print(":GU#");
//    s[Ser.readBytesUntil('#', s, 20)] = 0;
//    if (s[0] == 0) { _valid = false; return false; }
//    _tracking = false;
//    _slewing = false;
//    switch (s[0])
//    {
//    case '3':
//      _tracking = true;
//      _slewing = true;
//      break;
//    case '2':
//      _slewing = true;
//      break;
//    case '1':
//      _tracking = true;
//      break;
//    }
//    _sideralMode = s[1] - '0';
//    _atHome = (s[3] == 'H');
//    _parked = false;
//    _parking = false;
//    _parkFail = false;
//    switch (s[2])
//    {
//    case 'p':
//      _parked = false;
//      break;
//    case 'P':
//      _parked = true;
//      break;
//    case'I':
//      _parking = true;
//      break;
//    case 'F':
//      _parkFail = true;
//      break;
//    default:
//      break;
//    }
//    _ppsSync = (s[4] == 'S');
//    _guiding = (s[6] == '*');
//    _recenter = (s[6] == '+');
//    _axisFault = (s[9] == 'f');
//    //if (strstr(s,"r")) { if (strstr(s,"s")) _rateCompensation=RC_REFR_RA; else _rateCompensation=RC_REFR_BOTH; } else
//    //if (strstr(s,"t")) { if (strstr(s,"s")) _rateCompensation=RC_FULL_RA; else _rateCompensation=RC_FULL_BOTH; } else _rateCompensation=RC_NONE;
//    _rateCompensation = RC_NONE;
//    _waitingHome = false;
//    _pauseAtHome = false;
//    _buzzerEnabled = false;
//    switch (s[3])
//    {
//    case 'E':
//      _mountType = MT_GEM;
//      break;
//    case 'K':
//      _mountType = MT_FORK;
//      break;
//    case'k':
//      _mountType = MT_FORKALT;
//      break;
//    case 'A':
//      _mountType = MT_ALTAZM;
//      break;
//    default:
//      _mountType = MT_UNKNOWN;
//      break;
//    }
//    _autoMeridianFlips = false;
//    _pierSide = 1;
//    if (s[13] == 'W') _pierSide = 2;
//    _validGNSS = s[14] - '0';
//    _lastError = (Errors)(s[15] - '0');
//
//    /*   if (all) {
//    Ser.print(":GX94#"); s[Ser.readBytesUntil('#',s,20)]=0; if (s[0]==0) { _valid=false; return false; }
//    _meridianFlips=!strstr(s, "N");
//    _pierSide=strtol(&s[0],NULL,10);
//
//    if (_alignMaxStars==-1) {
//    Ser.print(":A?#");
//    s[Ser.readBytesUntil('#',s,20)]=0;
//    _alignMaxStars=3;
//    if (s[0]!=0) { if ((s[0]>'0') && (s[0]<='9')) _alignMaxStars=s[0]-'0'; }
//    }
//    }*/
//
//    _valid = true;
//    return true;
//  }
//  bool updateFocuserSettings()
//  {
//
//  }
//  bool getId(char id[]) { if (!_valid) return false; else { strcpy(id, _id); return true; } }
//  bool getVer(char ver[]) { if (!_valid) return false; else { strcpy(ver, _ver); return true; } }
//  bool valid() { return _valid; }
//  bool tracking() { return _tracking; }
//  byte sideralMode() { return _sideralMode; }
//  bool slewing() { return _slewing; }
//  bool parked() { return _parked; }
//  bool parking() { return _parking; }
//  bool parkFail() { return _parkFail; }
//  bool pecRecorded() { return _pecRecorded; }
//  bool pecRecording() { return _pecRecording; }
//  bool atHome() { return _atHome; }
//  bool ppsSync() { return _ppsSync; }
//  bool guiding() { return _guiding; }
//  bool axisFault() { return _axisFault; }
//  bool waitingHome() { return _waitingHome; }
//  bool pauseAtHome() { return _pauseAtHome; }
//  bool buzzerEnabled() { return _buzzerEnabled; }
//  MountTypes mountType() { return _mountType; }
//  RateCompensation rateCompensation() { return _rateCompensation; }
//  bool meridianFlips() { return _meridianFlips; }
//  bool autoMeridianFlips() { return _autoMeridianFlips; }
//  byte pierSide() { return _pierSide; }
//  int alignMaxStars() { return _alignMaxStars; }
//  Errors lastError() { return _lastError; }
//  bool getLastErrorMessage(char message[]) {
//    strcpy(message, "");
//    if (_lastError == ERR_NONE) strcpy(message, "None"); else
//      if (_lastError == ERR_MOTOR_FAULT) strcpy(message, "Motor or Driver Fault"); else
//        if (_lastError == ERR_ALT) strcpy(message, "Altitude Min/Max"); else
//          if (_lastError == ERR_LIMIT_SENSE) strcpy(message, "Limit Sense"); else
//            if (_lastError == ERR_DEC) strcpy(message, "Dec Limit Exceeded"); else
//              if (_lastError == ERR_AZM) strcpy(message, "Azm Limit Exceeded"); else
//                if (_lastError == ERR_UNDER_POLE) strcpy(message, "Under Pole Limit Exceeded"); else
//                  if (_lastError == ERR_MERIDIAN) strcpy(message, "Meridian Limit (W) Exceeded"); else
//                    if (_lastError == ERR_SYNC) strcpy(message, "Sync. ignored >30&deg;");
//    return message[0];
//  }
//private:
//  char _id[20] = "";
//  char _ver[20] = "";
//  int lastupdate = millis();
//  bool _valid = false;
//  bool _tracking = false;
//  byte  _sideralMode = 0;
//  bool _slewing = false;
//  bool _parked = false;
//  bool _parking = false;
//  bool _parkFail = false;
//  bool _pecRecorded = false;
//  bool _pecRecording = false;
//  bool _atHome = false;
//  bool _ppsSync = false;
//  bool _guiding = false;
//  bool _recenter = false;
//  bool _axisFault = false;
//  bool _waitingHome = false;
//  bool _pauseAtHome = false;
//  bool _buzzerEnabled = false;
//  MountTypes _mountType = MT_UNKNOWN;
//  RateCompensation _rateCompensation = RC_NONE;
//  bool _meridianFlips = true;
//  bool _autoMeridianFlips = false;
//  byte _pierSide = PierSideNone;
//  int _alignMaxStars = -1;
//  bool _validGNSS = false;
//  Errors _lastError = ERR_NONE;
  //----------------------
};

