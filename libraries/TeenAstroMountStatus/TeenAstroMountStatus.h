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
//  StepperDriver — MainUnit :GVb# returns a single digit; this enum makes
//  the mapping explicit (same values as MainUnit AxisDriver / Command_G).
// ---------------------------------------------------------------------------
enum StepperDriver
{
  StepperDriver_StepDir  = 0,
  StepperDriver_TOS100   = 1,
  StepperDriver_TMC2130  = 2,
  StepperDriver_TMC5160  = 3,
  StepperDriver_TMC2660  = 4,
  StepperDriver_Unknown  = -1
};

/// Human-readable name for a stepper driver (for display on SHC, WiFi, etc.).
const char* stepperDriverName(StepperDriver d);

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
  /// Raw driver string from mount (e.g. "3" for TMC5160). Prefer getDriverType() / getDriverName().
  const char* getVb()   { return m_vbb; }
  /// Parsed driver type from :GVb# (StepDir=0, TOS100=1, TMC2130=2, TMC5160=3, TMC2660=4).
  StepperDriver getDriverType();
  /// Human-readable driver name; uses getDriverType() and stepperDriverName().
  bool getDriverName(char* name);
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

  // Numeric accessors populated by updateAllState()
  uint8_t  getUtcHour()   const { return m_utcH; }
  uint8_t  getUtcMin()    const { return m_utcM; }
  uint8_t  getUtcSec()    const { return m_utcS; }
  uint8_t  getUtcMonth()  const { return m_utcMonth; }
  uint8_t  getUtcDay()    const { return m_utcDay; }
  uint8_t  getUtcYear()   const { return m_utcYear; }
  uint32_t getFocuserPos()   const { return m_focuserPosN; }
  uint16_t getFocuserSpeed() const { return m_focuserSpeedN; }

  /// True when the all-state cache is older than 500 ms (stale).
  bool allStateCacheStale() const { return m_timerAllState.needsUpdate(500); }

  /// Returns the cached base64 string from the last successful :GXAS# call,
  /// with the '#' terminator included, ready to forward to TCP clients.
  const char* getAllStateB64Cached() const { return m_allStateB64; }

  long getTrackingRateRa()        { return m_trackRateRa; }
  long getTrackingRateDec()       { return m_trackRateDec; }
  long getStoredTrackingRateRa()  { return m_storedTrackRateRa; }
  long getStoredTrackingRateDec() { return m_storedTrackRateDec; }

  // -----------------------------------------------------------------------
  //  Config accessors (populated by updateAllConfig())
  // -----------------------------------------------------------------------
  bool hasConfig() const { return m_configValid; }

  // Per-axis motor params: axis 0 = RA/Az (Axis1), axis 1 = Dec/Alt (Axis2)
  uint32_t getCfgGear(int ax)         const { return m_cfgGear[ax]; }
  uint16_t getCfgStepRot(int ax)      const { return m_cfgStepRot[ax]; }
  uint16_t getCfgBacklash(int ax)     const { return m_cfgBacklash[ax]; }
  uint16_t getCfgBacklashRate(int ax) const { return m_cfgBacklashRate[ax]; }
  uint16_t getCfgLowCurr(int ax)      const { return m_cfgLowCurr[ax]; }
  uint16_t getCfgHighCurr(int ax)     const { return m_cfgHighCurr[ax]; }
  uint8_t  getCfgMicro(int ax)        const { return m_cfgMicro[ax]; }
  bool     getCfgReverse(int ax)      const { return (m_cfgFlags[ax] >> 0) & 1; }
  bool     getCfgSilent(int ax)       const { return (m_cfgFlags[ax] >> 1) & 1; }

  // Rates / Speed
  float    getCfgGuideRate()    const { return m_cfgGuideRate; }
  float    getCfgSlowRate()     const { return m_cfgSlowRate; }
  float    getCfgMediumRate()   const { return m_cfgMediumRate; }
  float    getCfgFastRate()     const { return m_cfgFastRate; }
  float    getCfgAcceleration() const { return m_cfgAcceleration; }
  uint16_t getCfgMaxRate()      const { return m_cfgMaxRate; }
  uint8_t  getCfgDefaultRate()  const { return m_cfgDefaultRate; }
  uint8_t  getCfgSettleTime()   const { return m_cfgSettleTime; }

  // Limits
  int16_t  getCfgMeridianE()    const { return m_cfgMeridianE; }
  int16_t  getCfgMeridianW()    const { return m_cfgMeridianW; }
  int16_t  getCfgAxis1Min()     const { return m_cfgAxis1Min; }
  int16_t  getCfgAxis1Max()     const { return m_cfgAxis1Max; }
  int16_t  getCfgAxis2Min()     const { return m_cfgAxis2Min; }
  int16_t  getCfgAxis2Max()     const { return m_cfgAxis2Max; }
  uint16_t getCfgUnderPole10()  const { return m_cfgUnderPole10; }
  int8_t   getCfgMinAlt()       const { return m_cfgMinAlt; }
  int8_t   getCfgMaxAlt()       const { return m_cfgMaxAlt; }
  uint8_t  getCfgMinDistPole()  const { return m_cfgMinDistPole; }
  bool     getCfgRefrTracking() const { return (m_cfgRefrFlags >> 0) & 1; }
  bool     getCfgRefrGoto()     const { return (m_cfgRefrFlags >> 1) & 1; }
  bool     getCfgRefrPole()     const { return (m_cfgRefrFlags >> 2) & 1; }

  // Encoders
  uint32_t getCfgPPD1()         const { return m_cfgPPD1; }
  uint32_t getCfgPPD2()         const { return m_cfgPPD2; }
  uint8_t  getCfgEncSyncMode()  const { return m_cfgEncSyncMode; }
  bool     getCfgEncReverse(int ax) const { return (m_cfgEncFlags >> ax) & 1; }

  // Options
  uint8_t  getCfgMountIdx()     const { return m_cfgMountIdx; }

  /// True when the config cache is stale (> 30 s old — config rarely changes).
  bool configCacheStale() const { return m_timerConfig.needsUpdate(30000); }

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

  /// Single command (:GXAS#) that refreshes ALL cached state at once:
  /// mount status, positions (RA/Dec/Alt/Az/LST/targetRA/targetDec),
  /// UTC date/time, and focuser position/speed.
  void updateAllState(bool force = false);

  /// Single command (:GXCS#) that refreshes ALL mount configuration:
  /// motor params (both axes), rates/speed, limits, encoders, and refraction.
  /// Configuration changes infrequently; call on startup or after a setting
  /// change rather than every poll cycle.
  void updateAllConfig(bool force = false);

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
  bool checkConnection(const char* major, const char* minor);
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

  // --- All-state bulk cache (:GXAS#) ---
  // Stores the 88-char base64 string + '#' + NUL (90 bytes total).
  char          m_allStateB64[96] = "";
  CacheTimer    m_timerAllState;
  // Unpacked UTC components
  uint8_t       m_utcH = 0, m_utcM = 0, m_utcS = 0;
  uint8_t       m_utcMonth = 1, m_utcDay = 1, m_utcYear = 0;
  // Unpacked focuser numerics
  uint32_t      m_focuserPosN   = 0;
  uint16_t      m_focuserSpeedN = 0;

  // --- All-config bulk cache (:GXCS#) ---
  CacheTimer    m_timerConfig;
  bool          m_configValid = false;
  // Per-axis motor params [0]=Axis1, [1]=Axis2
  uint32_t  m_cfgGear[2]         = {};
  uint16_t  m_cfgStepRot[2]      = {};
  uint16_t  m_cfgBacklash[2]     = {};
  uint16_t  m_cfgBacklashRate[2] = {};
  uint16_t  m_cfgLowCurr[2]      = {};
  uint16_t  m_cfgHighCurr[2]     = {};
  uint8_t   m_cfgMicro[2]        = {};
  uint8_t   m_cfgFlags[2]        = {};  // [bit0=reverse, bit1=silent]
  // Rates
  float     m_cfgGuideRate    = 0;
  float     m_cfgSlowRate     = 0;
  float     m_cfgMediumRate   = 0;
  float     m_cfgFastRate     = 0;
  float     m_cfgAcceleration = 0;
  uint16_t  m_cfgMaxRate      = 0;
  uint8_t   m_cfgDefaultRate  = 0;
  uint8_t   m_cfgSettleTime   = 0;
  // Limits
  int16_t   m_cfgMeridianE    = 0;
  int16_t   m_cfgMeridianW    = 0;
  int16_t   m_cfgAxis1Min     = 0;
  int16_t   m_cfgAxis1Max     = 0;
  int16_t   m_cfgAxis2Min     = 0;
  int16_t   m_cfgAxis2Max     = 0;
  uint16_t  m_cfgUnderPole10  = 0;
  int8_t    m_cfgMinAlt       = 0;
  int8_t    m_cfgMaxAlt       = 90;
  uint8_t   m_cfgMinDistPole  = 0;
  uint8_t   m_cfgRefrFlags    = 0;
  // Encoders
  uint32_t  m_cfgPPD1         = 0;
  uint32_t  m_cfgPPD2         = 0;
  uint8_t   m_cfgEncSyncMode  = 0;
  uint8_t   m_cfgEncFlags     = 0;
  // Options
  uint8_t   m_cfgMountIdx     = 0;

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
