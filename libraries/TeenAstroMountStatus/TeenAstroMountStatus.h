#pragma once
#include <Arduino.h>
#include <LX200Client.h>
#include "TeenAstoCustomizations.h"

// ---------------------------------------------------------------------------
//  CacheTimer — rate-limits repeated queries
// ---------------------------------------------------------------------------
struct CacheTimer
{
  unsigned long lastUpdate = 0;
  bool needsUpdate(unsigned long rateMs = 200) const
  {
    return (millis() - lastUpdate) > rateMs;
  }
  void markUpdated() { lastUpdate = millis(); }
};

// ---------------------------------------------------------------------------
//  CachedStr<N> — a small fixed-size string with a validity flag
// ---------------------------------------------------------------------------
template<int N>
struct CachedStr
{
  char  data[N] = "?";
  bool  valid    = false;

  /// Fetch from client using a named method (pointer-to-member).
  bool fetch(LX200Client& c, LX200RETURN (LX200Client::*method)(char*, int))
  {
    valid = ((c.*method)(data, N) == LX200_VALUEGET);
    return valid;
  }

  /// Fetch using a raw get command (for axis-parameterised queries).
  bool fetchRaw(LX200Client& c, const char* cmd)
  {
    valid = (c.get(cmd, data, N) == LX200_VALUEGET);
    return valid;
  }

  operator const char*() const { return data; }
};

// ---------------------------------------------------------------------------
//  MountState — parsed snapshot of the :GXI# status string
// ---------------------------------------------------------------------------
class TeenAstroMountStatus;   // forward

struct MountState
{
  // --- Enums (shared with TeenAstroMountStatus for backward compat) ---
  // These are aliases; the canonical definitions live in TeenAstroMountStatus.

  enum TrackState    { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
  enum SiderealMode  { SID_UNKNOWN = -1, SID_STAR, SID_SUN, SID_MOON, SID_TARGET };
  enum ParkState     { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
  enum PierState     { PIER_E, PIER_W, PIER_UNKNOW };
  enum GuidingRate   { UNKNOW = -1, GUIDING, SLOW, MEDIUM, FAST, MAX };
  enum RateCompensation { RC_UNKNOWN = -1, RC_RA = 1, RC_BOTH };
  enum Mount         { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK,
                       MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
  enum Errors        { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE,
                       ERR_LIMIT_A1, ERR_LIMIT_A2, ERR_UNDER_POLE,
                       ERR_MERIDIAN, ERR_SYNC };

  // --- Parsed fields ---
  TrackState        tracking      = TRK_UNKNOW;
  SiderealMode      sidereal      = SID_UNKNOWN;
  ParkState         parkState     = PRK_UNKNOW;
  bool              atHome        = false;
  GuidingRate       guidingRate   = UNKNOW;
  bool              spiralRunning = false;
  bool              pulseGuiding  = false;
  char              guidingEW     = ' ';        // '>' east, '<' west, ' ' none
  char              guidingNS     = ' ';        // '^' north, '_' south, ' ' none
  bool              trackCorrected = false;
  RateCompensation  rateComp      = RC_UNKNOWN;
  bool              aligned       = false;
  Mount             mountType     = MOUNT_UNDEFINED;
  PierState         pierSide      = PIER_UNKNOW;
  uint8_t           gnssFlags     = 0;          // bitfield from char[14]
  Errors            error         = ERR_NONE;
  uint8_t           enableFlags   = 0;          // bitfield from char[16]
  bool              valid         = false;

  // --- Parse from raw :GXI# string ---
  void parseFrom(const char* raw);

  // --- GNSS convenience ---
  bool hasGNSSBoard()        const { return gnssFlags & 0x01; }
  bool isGNSSValid()         const { return gnssFlags & 0x02; }
  bool isGNSSTimeSync()      const { return gnssFlags & 0x04; }
  bool isGNSSLocationSync()  const { return gnssFlags & 0x08; }
  bool isHdopSmall()         const { return gnssFlags & 0x10; }

  // --- Enable convenience ---
  bool encodersEnabled()     const { return enableFlags & 0x01; }
  bool calibratingEncoder()  const { return enableFlags & 0x02; }
  bool isPushingTo()         const { return enableFlags & 0x04; }
  bool motorsEnabled()       const { return enableFlags & 0x08; }
  bool isPushTo()            const { return !motorsEnabled() && encodersEnabled(); }

  // --- Mount type convenience ---
  bool isAltAz() const
  {
    return mountType != MOUNT_TYPE_GEM && mountType != MOUNT_TYPE_FORK;
  }
};

// ---------------------------------------------------------------------------
//  TeenAstroMountStatus
// ---------------------------------------------------------------------------
class TeenAstroMountStatus
{
public:
  // Re-export MountState enums for backward compatibility
  typedef MountState::Errors          Errors;
  typedef MountState::Mount           Mount;
  typedef MountState::TrackState      TrackState;
  typedef MountState::SiderealMode    SiderealMode;
  typedef MountState::ParkState       ParkState;
  typedef MountState::PierState       PierState;
  typedef MountState::GuidingRate     GuidingRate;
  typedef MountState::RateCompensation RateCompensation;

  // Backward-compat enum value aliases
  static constexpr Errors ERR_NONE        = MountState::ERR_NONE;
  static constexpr Errors ERR_MOTOR_FAULT = MountState::ERR_MOTOR_FAULT;
  static constexpr Errors ERR_ALT         = MountState::ERR_ALT;
  static constexpr Errors ERR_LIMIT_SENSE = MountState::ERR_LIMIT_SENSE;
  static constexpr Errors ERR_LIMIT_A1    = MountState::ERR_LIMIT_A1;
  static constexpr Errors ERR_LIMIT_A2    = MountState::ERR_LIMIT_A2;
  static constexpr Errors ERR_UNDER_POLE  = MountState::ERR_UNDER_POLE;
  static constexpr Errors ERR_MERIDIAN    = MountState::ERR_MERIDIAN;
  static constexpr Errors ERR_SYNC        = MountState::ERR_SYNC;

  static constexpr Mount MOUNT_UNDEFINED   = MountState::MOUNT_UNDEFINED;
  static constexpr Mount MOUNT_TYPE_GEM    = MountState::MOUNT_TYPE_GEM;
  static constexpr Mount MOUNT_TYPE_FORK   = MountState::MOUNT_TYPE_FORK;
  static constexpr Mount MOUNT_TYPE_ALTAZM = MountState::MOUNT_TYPE_ALTAZM;
  static constexpr Mount MOUNT_TYPE_FORK_ALT = MountState::MOUNT_TYPE_FORK_ALT;

  static constexpr TrackState TRK_OFF     = MountState::TRK_OFF;
  static constexpr TrackState TRK_ON      = MountState::TRK_ON;
  static constexpr TrackState TRK_SLEWING = MountState::TRK_SLEWING;
  static constexpr TrackState TRK_UNKNOW  = MountState::TRK_UNKNOW;

  static constexpr ParkState PRK_UNPARKED = MountState::PRK_UNPARKED;
  static constexpr ParkState PRK_PARKED   = MountState::PRK_PARKED;
  static constexpr ParkState PRK_FAILED   = MountState::PRK_FAILED;
  static constexpr ParkState PRK_PARKING  = MountState::PRK_PARKING;
  static constexpr ParkState PRK_UNKNOW   = MountState::PRK_UNKNOW;

  static constexpr PierState PIER_E       = MountState::PIER_E;
  static constexpr PierState PIER_W       = MountState::PIER_W;
  static constexpr PierState PIER_UNKNOW  = MountState::PIER_UNKNOW;

  // Flat aliases for TeenAstroMountStatus::GUIDING etc.
  static constexpr GuidingRate UNKNOW     = MountState::UNKNOW;
  static constexpr GuidingRate GUIDING    = MountState::GUIDING;
  static constexpr GuidingRate SLOW       = MountState::SLOW;
  static constexpr GuidingRate MEDIUM     = MountState::MEDIUM;
  static constexpr GuidingRate FAST       = MountState::FAST;
  static constexpr GuidingRate MAX        = MountState::MAX;

  static constexpr SiderealMode SID_UNKNOWN = MountState::SID_UNKNOWN;
  static constexpr SiderealMode SID_STAR    = MountState::SID_STAR;
  static constexpr SiderealMode SID_SUN     = MountState::SID_SUN;
  static constexpr SiderealMode SID_MOON    = MountState::SID_MOON;
  static constexpr SiderealMode SID_TARGET  = MountState::SID_TARGET;

  static constexpr RateCompensation RC_UNKNOWN = MountState::RC_UNKNOWN;
  static constexpr RateCompensation RC_RA      = MountState::RC_RA;
  static constexpr RateCompensation RC_BOTH    = MountState::RC_BOTH;

  enum AlignMode  { ALIM_OFF, ALIM_ONE, ALIM_TWO, ALIM_THREE };
  enum AlignState { ALI_OFF, ALI_SELECT, ALI_SLEW, ALI_RECENTER };
  enum AlignReply { ALIR_FAILED1, ALIR_FAILED2, ALIR_DONE, ALIR_ADDED };

  // -----------------------------------------------------------------------
  //  Client binding
  // -----------------------------------------------------------------------
  void setClient(LX200Client& client) { m_client = &client; }
  LX200Client& client() { return *m_client; }

  // -----------------------------------------------------------------------
  //  Alignment state machine
  // -----------------------------------------------------------------------
  bool      isAligning()      { return m_align != ALI_OFF; }
  bool      isAlignSlew()     { return m_align == ALI_SLEW; }
  bool      isAlignSelect()   { return m_align == ALI_SELECT; }
  bool      isAlignRecenter() { return m_align == ALI_RECENTER; }
  void      stopAlign()       { m_align = ALI_OFF; m_alignStar = 0; }
  void      startAlign(AlignMode in)           { m_aliMode = in; m_align = ALI_SELECT; m_alignStar = 1; }
  void      startAlignSecondStar(AlignMode in) { m_aliMode = in; m_align = ALI_SELECT; m_alignStar = 2; }
  void      nextStepAlign();
  void      backStepAlign();
  bool      isLastStarAlign() { return (int)m_aliMode == m_alignStar; }
  AlignMode getAlignMode()    { return m_aliMode; }
  int       getAlignStar()    { return m_alignStar; }
  void      setAlignMode(AlignMode mode) { m_aliMode = mode; }
  AlignReply addStar();

  unsigned short alignSelectedStar = 1;
  int            alignMaxNumStars  = -1;

  // -----------------------------------------------------------------------
  //  Cached values — validity checks
  // -----------------------------------------------------------------------
  bool hasInfoV()         { return m_version.valid; }
  bool hasInfoRa()        { return m_ra.valid; }
  bool hasInfoHa()        { return m_ha.valid; }
  bool hasInfoDec()       { return m_dec.valid; }
  bool hasInfoAz()        { return m_az.valid; }
  bool hasInfoAlt()       { return m_alt.valid; }
  bool hasInfoPush()      { return m_push.valid; }
  bool hasInfoUTC()       { return m_utc.valid; }
  bool hasInfoLHA()       { return m_lha.valid; }
  bool hasInfoSidereal()  { return m_sidereal.valid; }
  bool hasInfoMount()     { return m_mount.valid; }
  bool hasInfoFocuser()   { return m_focuser.valid; }
  bool hasFocuser();
  bool hasInfoAxis1Step() { return m_axis1Step.valid; }
  bool hasInfoAxis2Step() { return m_axis2Step.valid; }
  bool hasInfoAxis1Deg()  { return m_axis1Deg.valid; }
  bool hasInfoAxis2Deg()  { return m_axis2Deg.valid; }
  bool hasInfoAxis1EDeg() { return m_axis1EDeg.valid; }
  bool hasInfoAxis2EDeg() { return m_axis2EDeg.valid; }
  bool hasInfoTrackingRate() { return m_hasInfoTrackingRate; }

  // -----------------------------------------------------------------------
  //  Cached values — accessors
  // -----------------------------------------------------------------------
  const char* getVP()   { return m_vp; }
  const char* getVN()   { return m_vn; }
  const char* getVB()   { return m_vb; }
  const char* getVb()   { return m_vbb; }
  const char* getVD()   { return m_vd; }
  const char* getRa()   { return m_ra; }
  const char* getHa()   { return m_ha; }
  const char* getDec()  { return m_dec; }
  const char* getRaT()  { return m_raT; }
  const char* getDecT() { return m_decT; }
  const char* getAz()   { return m_az; }
  const char* getAlt()  { return m_alt; }

  const char* GetPushA1() { return &m_push.data[2]; }
  const char* GetPushA2() { return &m_push.data[9]; }
  const char* GetPushE()  { return &m_push.data[0]; }

  const char* getAxis1Step() { return m_axis1Step; }
  const char* getAxis2Step() { return m_axis2Step; }
  const char* getAxis1Deg()  { return m_axis1Deg; }
  const char* getAxis2Deg()  { return m_axis2Deg; }
  const char* getAxis1Degc() { return m_axis1Degc; }
  const char* getAxis2Degc() { return m_axis2Degc; }
  const char* getAxis1EDeg() { return m_axis1EDeg; }
  const char* getAxis2EDeg() { return m_axis2EDeg; }

  const char* getUTC()      { return m_utc; }
  const char* getLHA()       { return m_lha; }
  const char* getUTCdate()  { return m_utcDate; }
  const char* getSidereal() { return m_sidereal; }
  const char* getMState()   { return m_focuser; }   // kept for compat but rarely used
  const char* getFocuser()  { return m_focuser; }

  long getTrackingRateRa()        { return m_trackRateRa; }
  long getTrackingRateDec()       { return m_trackRateDec; }
  long getStoredTrackingRateRa()  { return m_storedTrackRateRa; }
  long getStoredTrackingRateDec() { return m_storedTrackRateDec; }

  // -----------------------------------------------------------------------
  //  Update methods (poll mount, rate-limited)
  // -----------------------------------------------------------------------
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
  void updateMount(bool force = false);

  // -----------------------------------------------------------------------
  //  Mount state queries (from cached MountState)
  // -----------------------------------------------------------------------
  const MountState& mountState() const { return m_mount; }

  Mount             getMount()           { return m_mount.mountType; }
  bool              isAltAz()            { return m_mount.isAltAz(); }
  ParkState         getParkState()       { return m_mount.parkState; }
  TrackState        getTrackingState()   { return m_mount.tracking; }
  SiderealMode      getSiderealMode()    { return m_mount.sidereal; }
  bool              isTrackingCorrected(){ return m_mount.trackCorrected; }
  PierState         getPierState()       { return m_mount.pierSide; }
  Errors            getError()           { return m_mount.error; }
  bool              getLastErrorMessage(char message[]);
  GuidingRate       getGuidingRate()     { return m_mount.guidingRate; }
  RateCompensation  getRateCompensation(){ return m_mount.rateComp; }
  bool              atHome()             { return m_mount.atHome; }
  bool              Parking()            { return m_mount.parkState == PRK_PARKING; }
  bool              Parked()             { return m_mount.parkState == PRK_PARKED; }
  bool              isSpiralRunning()    { return m_mount.spiralRunning; }
  bool              isPulseGuiding()     { return m_mount.pulseGuiding; }
  bool              isGuidingE()         { return m_mount.guidingEW == '>'; }
  bool              isGuidingW()         { return m_mount.guidingEW == '<'; }
  bool              isGuidingN()         { return m_mount.guidingNS == '^'; }
  bool              isGuidingS()         { return m_mount.guidingNS == '_'; }
  bool              isAligned()          { return m_mount.aligned; }
  bool              hasGNSSBoard()       { return m_mount.hasGNSSBoard(); }
  bool              isGNSSValid()        { return m_mount.isGNSSValid(); }
  bool              isGNSSTimeSync()     { return m_mount.isGNSSTimeSync(); }
  bool              isGNSSLocationSync() { return m_mount.isGNSSLocationSync(); }
  bool              isHdopSmall()        { return m_mount.isHdopSmall(); }
  bool              encodersEnable()     { updateMount(); return m_mount.encodersEnabled(); }
  bool              motorsEnable()       { updateMount(); return m_mount.motorsEnabled(); }
  bool              CalibratingEncoder() { return m_mount.calibratingEncoder(); }
  bool              isPushingto()        { return m_mount.isPushingTo(); }
  bool              isPushTo()           { return m_mount.isPushTo(); }

  // -----------------------------------------------------------------------
  //  Location (delegated to client)
  // -----------------------------------------------------------------------
  bool getLstT0(double& T0);
  bool getLat(double& lat);
  bool getLong(double& longi);
  bool getTrackingRate(double& r);

  // -----------------------------------------------------------------------
  //  Connection management
  // -----------------------------------------------------------------------
  bool checkConnection(char* major, char* minor);
  bool getDriverName(char* name);
  bool isConnectionValid() { return m_isValid; }
  bool findFocuser();
  bool connected();
  bool notResponding();
  void removeLastConnectionFailure() { m_connectionFailure = max(m_connectionFailure - 1, 0); }

private:
  LX200Client* m_client = nullptr;

  // --- Alignment state ---
  AlignState  m_align     = ALI_OFF;
  AlignMode   m_aliMode   = ALIM_ONE;
  int         m_alignStar = 0;

  // --- Version (fetched once) ---
  struct { bool valid = false; } m_version;
  CachedStr<20> m_vp;       // product name
  CachedStr<20> m_vn;       // version number
  CachedStr<10> m_vb;       // board version
  CachedStr<10> m_vbb;      // driver type
  CachedStr<20> m_vd;       // version date

  // --- Position caches ---
  CachedStr<15> m_ra, m_ha, m_dec;
  CachedStr<15> m_raT, m_decT;
  CachedStr<15> m_az, m_alt;
  CachedStr<20> m_push;
  CachedStr<15> m_axis1Step, m_axis2Step;
  CachedStr<15> m_axis1Deg, m_axis2Deg;
  CachedStr<15> m_axis1Degc, m_axis2Degc;
  CachedStr<15> m_axis1EDeg, m_axis2EDeg;

  // --- Time caches ---
  CachedStr<15> m_utc, m_lha, m_utcDate, m_sidereal;

  // --- Mount state ---
  MountState    m_mount;
  CacheTimer    m_timerMount;

  // --- Focuser ---
  CachedStr<45> m_focuser;
  CacheTimer    m_timerFocuser;
  bool          m_hasFocuser = false;

  // --- Tracking rates ---
  long          m_trackRateRa  = 0;
  long          m_trackRateDec = 0;
  long          m_storedTrackRateRa  = 0;
  long          m_storedTrackRateDec = 0;
  bool          m_hasInfoTrackingRate = false;
  CacheTimer    m_timerTrackRate;

  // --- Rate timers ---
  CacheTimer    m_timerRaDec, m_timerHaDec, m_timerRaDecT;
  CacheTimer    m_timerAzAlt, m_timerPush;
  CacheTimer    m_timerAxisStep, m_timerAxisDeg;
  CacheTimer    m_timerTime;

  // --- Connection ---
  int           m_connectionFailure = 0;
  bool          m_isValid = false;
};

extern TeenAstroMountStatus ta_MountStatus;
