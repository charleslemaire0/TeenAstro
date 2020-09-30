#pragma once
#include <Arduino.h>

class TeenAstroMountStatus
{
public:

  enum Errors { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE, ERR_DEC, ERR_AZM, ERR_UNDER_POLE, ERR_MERIDIAN, ERR_SYNC };
  enum AlignMode { ALIM_OFF, ALIM_ONE, ALIM_TWO, ALIM_THREE };
  enum AlignState { ALI_OFF, ALI_SELECT, ALI_SLEW, ALI_RECENTER };
  enum AlignReply { ALIR_FAILED1, ALIR_FAILED2, ALIR_DONE, ALIR_ADDED};
  enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
  enum TrackState { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
  enum RateCompensation { RC_NONE, RC_REFR_RA, RC_REFR_BOTH, RC_FULL_RA, RC_FULL_BOTH };
  enum SiderealMode { SID_STAR, SID_SUN, SID_MOON };
  enum ParkState { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
  enum PierState { PIER_E, PIER_W, PIER_UNKNOW };
private:
  //Align
  AlignState      m_align = ALI_OFF;
  AlignMode       m_aliMode = ALIM_ONE;
  int             m_alignStar = 0;

  //Cache Answer

  char            m_TempVP[20] = "?";
  char            m_TempVN[20] = "?";
  char            m_TempVB[10] = "?";
  char            m_TempVb[10] = "?";
  char            m_TempVD[20] = "?";
  char            m_TempRa[15] = "?";
  char            m_TempDec[15] = "?";
  char            m_TempRaT[15] = "?";
  char            m_TempDecT[15] = "?";
  unsigned long   m_lastStateRaDec;
  unsigned long   m_lastStateRaDecT;
  char            m_TempAz[15] = "?";
  char            m_TempAlt[15] = "?";
  unsigned long   m_lastStateAzAlt;
  char            m_TempAxis1[15] = "?";
  char            m_TempAxis2[15] = "?";
  unsigned long   m_lastStateAxis;
  char            m_TempUTC[15] = "?";
  char            m_TempUTCdate[15] = "?";
  char            m_TempSidereal[15] = "?";
  unsigned long   m_lastStateTime;
  char            m_TempMount[17] = "?";
  unsigned long   m_lastStateMount;
  char            m_TempTrackingRate[15] = "?";
  unsigned long   m_lastTrackingRate;
  char            m_TempFocuser[45] = "?";
  unsigned long   m_lastStateFocuser;
  int             m_connectionFailure = 0;
  bool            m_isValid = false;
  bool            m_hasInfoV = false;
  bool            m_hasInfoRa = false;
  bool            m_hasInfoDec = false;
  bool            m_hasInfoRaT = false;
  bool            m_hasInfoDecT = false;
  bool            m_hasInfoAz = false;
  bool            m_hasInfoAlt = false;
  bool            m_hasInfoAxis1 = false;
  bool            m_hasInfoAxis2 = false;
  bool            m_hasInfoUTC = false;
  bool            m_hasInfoUTCdate = false;
  bool            m_hasInfoSidereal = false;
  bool            m_hasInfoTrackingRate = false;
  bool            m_hasInfoMount = false;
  bool            m_hasInfoFocuser = false;
  bool            m_hasFocuser = true;
public:
  //Alignment Stuff
  bool            isAligning()  { return m_align != ALI_OFF; }
  bool            isAlignSlew() { return m_align == ALI_SLEW; };
  bool            isAlignSelect() { return m_align == ALI_SELECT; };
  bool            isAlignRecenter() { return m_align == ALI_RECENTER; };
  void            stopAlign() { m_align = ALI_OFF; m_alignStar = 0; };
  void            startAlign(AlignMode in) { m_aliMode = in;  m_align = ALI_SELECT; m_alignStar; m_alignStar = 1; };
  void            nextStepAlign();
  void            backStepAlign();
  bool            isLastStarAlign() { return (int)m_aliMode == m_alignStar;  };
  AlignMode       getAlignMode() { return m_aliMode; };
  int             getAlignStar() { return m_alignStar; };
  void            setAlignMode(AlignMode mode){ m_aliMode = mode;};
  AlignReply      addStar();
  unsigned short  alignSelectedStar = 1;
  int             alignMaxNumStars = -1;

  bool hasInfoV() { return m_hasInfoV; };
  bool hasInfoRa() { return m_hasInfoRa; };
  bool hasInfoDec() { return m_hasInfoDec; };
  bool hasInfoAz() { return m_hasInfoAz; };
  bool hasInfoAlt() { return m_hasInfoAlt; };
  bool hasInfoUTC() { return m_hasInfoUTC; };
  bool hasInfoSidereal() { return m_hasInfoSidereal; };
  bool hasInfoMount() { return m_hasInfoMount; };
  bool hasInfoFocuser() { return m_hasInfoFocuser; };
  bool hasFocuser() { static bool firstime = m_hasFocuser; if (firstime){updateFocuser();} return m_hasFocuser; }
  bool hasInfoAxis1() { return m_hasInfoAxis1; };
  bool hasInfoAxis2() { return m_hasInfoAxis2; };
  bool hasInfoTrackingRate() { return m_hasInfoTrackingRate; };

  const char* getVP() { return  m_TempVP; };
  const char* getVN() { return  m_TempVN; };
  const char* getVB() { return  m_TempVB; };
  const char* getVb() { return  m_TempVb; };
  const char* getVD() { return  m_TempVD; };
  const char* getRa() { return  m_TempRa; };
  const char* getDec() { return  m_TempDec; };
  const char* getRaT() { return  m_TempRaT; };
  const char* getDecT() { return  m_TempDecT; };
  const char* getAz() { return  m_TempAz; };
  const char* getAlt() { return  m_TempAlt; };
  const char* getAxis1() { return  m_TempAxis1; };
  const char* getAxis2() { return  m_TempAxis2; };
  const char* getUTC() { return m_TempUTC; };
  const char* getUTCdate() { return m_TempUTCdate; };
  const char* getSidereal() { return m_TempSidereal; };
  const char* getMState() { return m_TempMount; };
  const char* getFocuser() { return m_TempFocuser; };
  const char* getTrackingRate() { return m_TempTrackingRate; };

  void updateV();
  void updateRaDec();
  void updateRaDecT();
  void updateAzAlt();
  void updateAxis();
  void updateTime();
  void updateFocuser();
  void updateTrackingRate();
  void updateMount();

  Mount       getMount();
  bool        isAltAz();
  ParkState   getParkState();
  TrackState  getTrackingState();
  SiderealMode getSiderealMode();
  bool        isTrackingCorrected();
  PierState   getPierState();
  Errors      getError();
  bool        getLastErrorMessage(char message[]);
  bool        getLstT0(double &T0);
  bool        getLat(double &lat);
  bool        getTrackingRate(double &r);
  bool        getGuidingRate(unsigned char &g);
  bool checkConnection(char* major, char* minor);
  bool getDriverName(char* name);
  bool isConnectionValid() { return m_isValid; };
  bool atHome();
  bool Parking();
  bool Parked();
  bool isSpiralRunning();
  bool isPulseGuiding();
  bool isGuidingN();
  bool isGuidingS();
  bool isGuidingE();
  bool isGuidingW();
  bool isAligned();
  bool isGNSSValid();
  bool isLowPower();
  //Connection Errors
  bool connected();
  bool notResponding();
  void removeLastConnectionFailure() { m_connectionFailure = max(m_connectionFailure - 1, 0); };
};

extern TeenAstroMountStatus ta_MountStatus;
