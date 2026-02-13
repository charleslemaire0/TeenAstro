#pragma once
// Cross-module function declarations for TeenAstro Main Unit (C++ build)

void reboot();

// Timer
speed interval2speed(interval i);
interval speed2interval(speed V, interval maxInterval);
void SetsiderealClockSpeed(double cs);
void beginTimers();

// ST4
void setupST4();
void checkST4();

// Park
bool setPark();
void unsetPark();
void saveAlignModel();
void parkClearBacklash();
void finalizePark();
byte park();
bool syncAtPark();
bool iniAtPark();
void unpark();

// MoveTo
void moveTo();
void DecayModeTracking();
void DecayModeGoto();

// Move
void MoveAxis1(const bool BW, const Guiding Mode);
void MoveAxisAtRate1(const double newrate);
void StopAxis1();
void MoveAxis2(const bool BW, const Guiding Mode);
void MoveAxisAtRate2(const double newrate);
void StopAxis2();
void CheckEndOfMoveAxisAtRate();
void CheckSpiral();
void StartSideralTracking();

// Limit
void setAtMount(long &axis1, long &axis2);
PoleSide getPoleSide(const long &axis2);
bool checkPole(const long &axis1, const long& axis2, CheckMode mode);
bool checkMeridian(const long &axis1, const long &axis2, CheckMode mode);
bool withinLimit(const long &axis1, const long &axis2);
void initLimit();
void reset_EE_Limit();
void force_reset_EE_Limit();
bool initLimitMinAxis1();
bool initLimitMaxAxis1();
bool initLimitMinAxis2();
bool initLimitMaxAxis2();

// Home
bool setHome();
void unsetHome();
bool goHome();
void finalizeHome();
bool syncAtHome();
void initHome();

// Guide
void apply_GuidingA1();
void apply_GuidingA2();
void StopGuiding();
bool StopIfMountError();
void PerformPulseGuiding();
void PerfomST4Guiding();
void PerfomGuidingRecenter();
void PerformGuidingAtRate();
bool isGuidingStar();
void Guide();

// Goto
void StepToAngle(long Axis1, long Axis2, double* AngleAxis1, double* AngleAxis2, PoleSide* Side);
void Angle2Step(double AngleAxis1, double AngleAxis2, PoleSide Side, long* Axis1, long* Axis2);
void syncAxis(const long* axis1, const long* axis2);
void GotoAxis(const long* axis1Target, const long* axis2Target);
bool SyncInstr(Coord_IN* instr, PoleSide Side);
bool syncEqu(Coord_EQ *EQ_T, PoleSide Side, double Lat);
bool syncAzAlt(Coord_HO *HO_T, PoleSide Side);
void syncTwithE();
void syncEwithT();
bool autoSyncWithEncoder(EncoderSync mode);
void getInstrDeg(double* A1, double* A2, double* A3);
Coord_HO getHorTopo();
Coord_HO getHorAppTarget();
Coord_IN getInstrTarget();
Coord_IN getInstr();
Coord_IN getInstrE();
Coord_EQ getEqu(double Lat);
Coord_EQ getEquE(double Lat);
Coord_EQ getEquTarget(double Lat);
Coord_HO getHorETopo();
bool predictTarget(const double& Axis1_in, const double& Axis2_in, const PoleSide& inputSide, long& Axis1_out, long& Axis2_out, PoleSide& outputSide);
byte goToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat);
byte goToHor(Coord_HO HO_T, PoleSide preferedPoleSide);
ErrorsGoTo goTo(long thisTargetAxis1, long thisTargetAxis2);
ErrorsGoTo Flip();

// EEPROM / Mount init
void AutoinitEEPROM();
void writeDefaultMounts();
void writeDefaultMount();
void writeDefaultMountName(int i);
void initMount();
void initTransformation(bool reset);
void initCelestialPole();
void initmotor(bool deleteAlignment);
void ReadEEPROMEncoderMotorMode();
void WriteEEPROMEncoderMotorMode();
void initencoder();
void readEEPROMmotorCurrent();
void readEEPROMmotor();
void writeDefaultEEPROMmotor();
void readEEPROMencoder();
void writeDefaultEEPROMencoder();

// Command reply helpers (Command.ino)
void replyShortTrue();
void replyLongTrue();
void replyShortFalse();
void replyLongFalse();
void replyLongUnknow();
void replyValueSetShort(bool set);
void replyNothing();
void clearReply();

// Astro
void updateDeltaTarget();
void updateDeltaStart();
PoleSide GetPoleSide();
PoleSide GetTargetPoleSide();
bool TelescopeBusy();
void ApplyTrackingRate();
void SetTrackingRate(double rHA, double rDEC);
void computeTrackingRate(bool apply);
void RateFromMovingTarget(Coord_EQ &EQprev, Coord_EQ &EQnext, const double &TimeRange, const PoleSide &side, const bool &refr, double &A1_trackingRate, double &A2_trackingRate);
void do_compensation_calc();
void initMaxRate();
void SetRates(double maxslewrate);
void SetAcceleration();
void enableGuideRate(int g, bool force = false);
void enableST4GuideRate();
void enableRecenterGuideRate();
void resetGuideRate();
bool isAltAZ();
void SafetyCheck(const bool forceTracking);
void enable_Axis(bool enable);
void updateRatios(bool deleteAlignment, bool deleteHP);
void updateSideral();

// PushTo
byte PushToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat, float* deltaA1, float* deltaA2);
byte PushToHor(Coord_HO HO_T, PoleSide preferedPoleSide, float* deltaA1, float* deltaA2);

// Command_GNSS (used from main loop)
void UpdateGnss();
bool GNSSTimeIsValid();
bool GNSSLocationIsValid();
bool isHdopSmall();
bool isTimeSyncWithGNSS();
bool isLocationSyncWithGNSS();

// Command_others
void Command_E();
