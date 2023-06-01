#pragma once
#include <Arduino.h>
#include "TeenAstoCustomizations.h"

class TeenAstroMountStatus
{
public:

  enum Errors { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE, ERR_LIMIT_A1, ERR_LIMIT_A2, ERR_UNDER_POLE, ERR_MERIDIAN, ERR_SYNC };
  enum AlignMode { ALIM_OFF, ALIM_ONE, ALIM_TWO, ALIM_THREE };
  enum AlignState { ALI_OFF, ALI_SELECT, ALI_SLEW, ALI_RECENTER };
  enum AlignReply { ALIR_FAILED1, ALIR_FAILED2, ALIR_DONE, ALIR_ADDED};
  enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
  enum TrackState { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
  enum RateCompensation { RC_UNKNOWN = -1, RC_NONE, RC_ALIGN_RA, RC_ALIGN_BOTH, RC_FULL_RA, RC_FULL_BOTH };
  enum SiderealMode { SID_UNKNOWN = -1, SID_STAR, SID_SUN, SID_MOON, SID_TARGET };
  enum ParkState { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
  enum PierState { PIER_E, PIER_W, PIER_UNKNOW };
  enum GuidingRate { UNKNOW = -1, GUIDING, SLOW, MEDIUM, FAST, MAX };
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
  char            m_TempHa[15] = "?";
  char            m_TempDec[15] = "?";
  char            m_TempRaT[15] = "?";
  char            m_TempDecT[15] = "?";
  unsigned long   m_lastStateRaDec;
  unsigned long   m_lastStateHaDec;
  unsigned long   m_lastStateRaDecT;
  char            m_TempAz[15] = "?";
  char            m_TempAlt[15] = "?";
  unsigned long   m_lastStateAzAlt;
  char            m_TempPush[20] = "?";
  unsigned long   m_lastStatePush;
  char            m_TempAxis1Step[15] = "?";
  char            m_TempAxis2Step[15] = "?";
  unsigned long   m_lastStateAxisStep;
  char            m_TempAxis1Deg[15] = "?";
  char            m_TempAxis2Deg[15] = "?";
  char            m_TempAxis1Degc[15] = "?";
  char            m_TempAxis2Degc[15] = "?";
  char            m_TempAxis1EDeg[15] = "?";
  char            m_TempAxis2EDeg[15] = "?";
  unsigned long   m_lastStateAxisDeg;
  char            m_TempUTC[15] = "?";
  char            m_TempLHA[15] = "?";
  char            m_TempUTCdate[15] = "?";
  char            m_TempSidereal[15] = "?";
  unsigned long   m_lastStateTime;
  char            m_TempMount[20] = "?";
  unsigned long   m_lastStateMount;
  unsigned long   m_lastStateTrackingRate;
  long            m_TempTrackingRateRa = 0;
  long            m_TempTrackingRateDec = 0;
  long            m_TempStoredTrackingRateRa = 0;
  long            m_TempStoredTrackingRateDec = 0;
  char            m_TempFocuser[45] = "?";
  unsigned long   m_lastStateFocuser;
  int             m_connectionFailure = 0;
  bool            m_isValid = false;
  bool            m_hasInfoV = false;
  bool            m_hasInfoRa = false;
  bool            m_hasInfoHa = false;
  bool            m_hasInfoDec = false;
  bool            m_hasInfoRaT = false;
  bool            m_hasInfoDecT = false;
  bool            m_hasInfoAz = false;
  bool            m_hasInfoAlt = false;
  bool            m_hasInfoPush = false;
  bool            m_hasInfoAxis1Step = false;
  bool            m_hasInfoAxis2Step = false;
  bool            m_hasInfoAxis1Deg = false;
  bool            m_hasInfoAxis2Deg = false;
  bool            m_hasInfoAxis1Degc = false;
  bool            m_hasInfoAxis2Degc = false;
  bool            m_hasInfoAxis1EDeg = false;
  bool            m_hasInfoAxis2EDeg = false;
  bool            m_hasInfoUTC = false;
  bool            m_hasInfoLHA = false;
  bool            m_hasInfoUTCdate = false;
  bool            m_hasInfoSidereal = false;
  bool            m_hasInfoTrackingRate = false;
  bool            m_hasInfoMount = false;
  bool            m_hasInfoFocuser = false;
  bool            m_hasFocuser = false;
  bool            m_hasEncoder = true;
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
  bool hasInfoHa() { return m_hasInfoHa; };
  bool hasInfoDec() { return m_hasInfoDec; };
  bool hasInfoAz() { return m_hasInfoAz; };
  bool hasInfoAlt() { return m_hasInfoAlt; };
  bool hasInfoPush() { return m_hasInfoPush; };
  bool hasInfoUTC() { return m_hasInfoUTC; };
  bool hasInfoLHA() { return m_hasInfoLHA; };
  bool hasInfoSidereal() { return m_hasInfoSidereal; };
  bool hasInfoMount() { return m_hasInfoMount; };
  bool hasInfoFocuser() { return m_hasInfoFocuser; };
  bool hasFocuser();
  bool hasInfoAxis1Step() { return m_hasInfoAxis1Step; };
  bool hasInfoAxis2Step() { return m_hasInfoAxis2Step; };
  bool hasInfoAxis1Deg() { return m_hasInfoAxis1Deg; };
  bool hasInfoAxis2Deg() { return m_hasInfoAxis2Deg; };
  bool hasInfoAxis1EDeg() { return m_hasInfoAxis1EDeg; };
  bool hasInfoAxis2EDeg() { return m_hasInfoAxis2EDeg; };
  bool hasInfoTrackingRate() { return m_hasInfoTrackingRate; };

  const char* getVP() { return  m_TempVP; };
  const char* getVN() { return  m_TempVN; };
  const char* getVB() { return  m_TempVB; };
  const char* getVb() { return  m_TempVb; };
  const char* getVD() { return  m_TempVD; };
  const char* getRa() { return  m_TempRa; };
  const char* getHa() { return  m_TempHa; };
  const char* getDec() { return  m_TempDec; };
  const char* getRaT() { return  m_TempRaT; };
  const char* getDecT() { return  m_TempDecT; };
  const char* getAz() { return  m_TempAz; };
  const char* getAlt() { return  m_TempAlt; };
  const char* GetPushA1() { return &m_TempPush[2]; };
  const char* GetPushA2() { return &m_TempPush[9]; };
  const char* GetPushE() { return &m_TempPush[0]; };
  const char* getAxis1Step() { return  m_TempAxis1Step; };
  const char* getAxis2Step() { return  m_TempAxis2Step; };
  const char* getAxis1Deg() { return  m_TempAxis1Deg; };
  const char* getAxis2Deg() { return  m_TempAxis2Deg; };
  const char* getAxis1Degc() { return  m_TempAxis1Degc; };
  const char* getAxis2Degc() { return  m_TempAxis2Degc; };
  const char* getAxis1EDeg() { return  m_TempAxis1EDeg; };
  const char* getAxis2EDeg() { return  m_TempAxis2EDeg; };

  const char* getUTC() { return m_TempUTC; };
  const char* getLHA() { return m_TempLHA; };
  const char* getUTCdate() { return m_TempUTCdate; };
  const char* getSidereal() { return m_TempSidereal; };
  const char* getMState() { return m_TempMount; };
  const char* getFocuser() { return m_TempFocuser; };
  long getTrackingRateRa() { return m_TempTrackingRateRa; };
  long getTrackingRateDec() { return m_TempTrackingRateDec; };
  long getStoredTrackingRateRa() { return m_TempStoredTrackingRateRa; };
  long getStoredTrackingRateDec() { return m_TempStoredTrackingRateDec; };

  void updateV();
  void updateRaDec();
  void updateHaDec();
  void updateRaDecT();
  void updateAzAlt();
  void updatePush();
  void updateAxisStep();
  void updateAxisDeg();
  void updateTime();
  void updateLHA();
  void updateFocuser();
  void updateTrackingRate();
  bool updateStoredTrackingRate();
  void updateMount();

  Mount             getMount();
  bool              isAltAz();
  ParkState         getParkState();
  TrackState        getTrackingState();
  SiderealMode      getSiderealMode();
  bool              isTrackingCorrected();
  PierState         getPierState();
  Errors            getError();
  bool              getLastErrorMessage(char message[]);
  bool              getLstT0(double &T0);
  bool              getLat(double &lat);
  bool              getLong(double &longi);
  bool              getTrackingRate(double &r);
  GuidingRate       getGuidingRate();
  RateCompensation  getRateCompensation();
  bool checkConnection(char* major, char* minor);
  bool getDriverName(char* name);
  bool isConnectionValid() { return m_isValid; };
  bool atHome();
  bool Parking();
  bool Parked();
  bool isPushingto();
  bool isSpiralRunning();
  bool isPulseGuiding();
  bool isGuidingN();
  bool isGuidingS();
  bool isGuidingE();
  bool isGuidingW();
  bool isAligned();
  bool hasGNSSBoard();
  bool isGNSSValid();
  bool isGNSSTimeSync();
  bool isGNSSLocationSync();
  bool isHdopSmall();
  bool findFocuser();
  bool hasEncoder();
  bool CalibratingEncoder();

  //Connection Errors
  bool connected();
  bool notResponding();
  void removeLastConnectionFailure() { m_connectionFailure = max(m_connectionFailure - 1, 0); };
};

extern TeenAstroMountStatus ta_MountStatus;
