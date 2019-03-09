#pragma once
#include "config.h"

#define PierSideNone     0
#define PierSideEast     1
#define PierSideWest     2
#define PierSideBest     3
#define PierSideFlipWE1  10
#define PierSideFlipWE2  11
#define PierSideFlipWE3  12
#define PierSideFlipEW1  20
#define PierSideFlipEW2  21
#define PierSideFlipEW3  22

class MountStatus {
  public:
    enum RateCompensation { RC_NONE, RC_REFR_RA, RC_REFR_BOTH, RC_FULL_RA, RC_FULL_BOTH };
    enum MountTypes { MT_UNKNOWN, MT_GEM, MT_FORK, MT_FORKALT, MT_ALTAZM };
    enum Errors { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE, ERR_DEC, ERR_AZM, ERR_UNDER_POLE, ERR_MERIDIAN, ERR_SYNC };

    bool update()
    {
      if ( millis() - lastupdate< 500)
        return false;
      char s[20] = "";
      if (!_valid) {
        Ser.print(":GVP#");
        s[Ser.readBytesUntil('#',s,20)]=0;
        if ((s[0]==0) || (!strstr(s,"TeenAstro"))) { _valid=false; return false; }
        strcpy(_id,s);
        Ser.print(":GVN#");
        s[Ser.readBytesUntil('#',s,20)]=0;
        if (s[0]==0) { _valid=false; return false; }
        strcpy(_ver,s);
      }

      Ser.print(":GU#");
      s[Ser.readBytesUntil('#',s,20)]=0;
      if (s[0]==0) { _valid=false; return false; }
      _tracking=false;
      _slewing=false;
      switch (s[0])
      {
      case '3':
        _tracking = true;
        _slewing = true;
        break;
      case '2':
        _slewing = true;
        break;
      case '1':
        _tracking = true;
        break;
      }
      _sideralMode = s[1] - '0';      
      _atHome = (s[3] == 'H');
      _parked = false;
      _parking = false;
      _parkFail = false;
      switch (s[2])
      {
      case 'p':
        _parked = false;
        break;
      case 'P':
        _parked = true;
        break;
      case'I':
        _parking = true;
        break;
      case 'F':
        _parkFail = true;
        break;
      default:
        break;
      }
      _ppsSync = (s[4] == 'S');
      _guiding = (s[6] == '*');
      _recenter = (s[6] == '+');
      _axisFault = (s[9] == 'f');
      //if (strstr(s,"r")) { if (strstr(s,"s")) _rateCompensation=RC_REFR_RA; else _rateCompensation=RC_REFR_BOTH; } else
      //if (strstr(s,"t")) { if (strstr(s,"s")) _rateCompensation=RC_FULL_RA; else _rateCompensation=RC_FULL_BOTH; } else _rateCompensation=RC_NONE;
      _rateCompensation = RC_NONE;
      _waitingHome   = false;
      _pauseAtHome   = false;
      _buzzerEnabled = false;
      switch (s[3])
      {
      case 'E':
        _mountType = MT_GEM;
        break;
      case 'K':
        _mountType = MT_FORK;
        break;
      case'k':
        _mountType = MT_FORKALT;
        break;
      case 'A':
        _mountType = MT_ALTAZM;
        break;
      default:
        _mountType = MT_UNKNOWN;
        break;
      }
      _autoMeridianFlips = false;
      _pierSide = 1;
      if (s[13] == 'W') _pierSide = 2;
      _validGNSS = s[14] - '0';
      _lastError = (Errors)(s[15]-'0');

   /*   if (all) {
        Ser.print(":GX94#"); s[Ser.readBytesUntil('#',s,20)]=0; if (s[0]==0) { _valid=false; return false; }
        _meridianFlips=!strstr(s, "N");
        _pierSide=strtol(&s[0],NULL,10);

        if (_alignMaxStars==-1) {
          Ser.print(":A?#");
          s[Ser.readBytesUntil('#',s,20)]=0;
          _alignMaxStars=3;
          if (s[0]!=0) { if ((s[0]>'0') && (s[0]<='9')) _alignMaxStars=s[0]-'0'; }
        }
      }*/
      
      _valid=true;
      return true;
    }
    bool updateFocuserSettings()
    {

    }
    bool getId(char id[]) { if (!_valid) return false; else { strcpy(id,_id); return true; } }
    bool getVer(char ver[]) { if (!_valid) return false; else { strcpy(ver,_ver); return true; } }
    bool valid() { return _valid; }
    bool tracking() { return _tracking; }
    bool slewing() { return _slewing; }
    bool parked() { return _parked; }
    bool parking() { return _parking; }
    bool parkFail() { return _parkFail; }
    bool pecRecorded() { return _pecRecorded; }
    bool pecRecording() { return _pecRecording; }
    bool atHome() { return _atHome; }
    bool ppsSync() { return _ppsSync; }
    bool guiding() { return _guiding; }
    bool axisFault() { return _axisFault; }
    bool waitingHome() { return _waitingHome; }
    bool pauseAtHome() { return _pauseAtHome; }
    bool buzzerEnabled() { return _buzzerEnabled; }
    MountTypes mountType() { return _mountType; }
    RateCompensation rateCompensation() { return _rateCompensation; }
    bool meridianFlips() { return _meridianFlips; }
    bool autoMeridianFlips() { return _autoMeridianFlips; }
    byte pierSide() { return _pierSide; }
    int alignMaxStars() { return _alignMaxStars; }
    Errors lastError() { return _lastError; }
    bool getLastErrorMessage(char message[]) {
      strcpy(message,"");
      if (_lastError==ERR_NONE) strcpy(message,"None"); else
      if (_lastError==ERR_MOTOR_FAULT) strcpy(message,"Motor or Driver Fault"); else
      if (_lastError==ERR_ALT) strcpy(message,"Altitude Min/Max"); else
      if (_lastError==ERR_LIMIT_SENSE) strcpy(message,"Limit Sense"); else
      if (_lastError==ERR_DEC) strcpy(message,"Dec Limit Exceeded"); else
      if (_lastError==ERR_AZM) strcpy(message,"Azm Limit Exceeded"); else
      if (_lastError==ERR_UNDER_POLE) strcpy(message,"Under Pole Limit Exceeded"); else
      if (_lastError==ERR_MERIDIAN) strcpy(message,"Meridian Limit (W) Exceeded"); else
      if (_lastError==ERR_SYNC) strcpy(message,"Sync. ignored >30&deg;");
      return message[0];
    }
  private:
    char _id[20]="";
    char _ver[20]="";
    int lastupdate = millis();
    bool _valid=false;
    bool _tracking=false;
    byte  _sideralMode = 0;
    bool _slewing=false;
    bool _parked=false;
    bool _parking=false;
    bool _parkFail=false;
    bool _pecRecorded=false;
    bool _pecRecording=false;
    bool _atHome=false;
    bool _ppsSync=false;
    bool _guiding=false;
    bool _recenter = false;
    bool _axisFault=false;
    bool _waitingHome=false;
    bool _pauseAtHome=false;
    bool _buzzerEnabled=false;
    MountTypes _mountType=MT_UNKNOWN;
    RateCompensation _rateCompensation=RC_NONE;
    bool _meridianFlips=true;
    bool _autoMeridianFlips=false;
    byte _pierSide=PierSideNone;
    int _alignMaxStars = -1;
    bool _validGNSS = false;
    Errors _lastError=ERR_NONE;
};


