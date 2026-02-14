#pragma once
/**
 * Mount.h - Mount class and sub-objects (ParkHome, Identity, Limits, etc.).
 * See MountClassDesign.md. Include this for Mount type and mount instance.
 */
#include "MountTypes.h"
#include "Config.TeenAstro.h"
#include "EEPROM_address.h"
#include "Axis.hpp"
#include "AxisEncoder.hpp"
#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"
#include "MountParkHome.h"
#include "MountIdentity.h"
#include "MountLimits.h"
#include "MountTargetCurrent.h"
#include "MountErrors.h"
#include "MountTracking.h"
#include "MountGuiding.h"
#include "MountAxes.h"
#include "MountAxisLimitManager.h"
#include "MountST4.h"
#include "MountParkHomeController.h"
#include "MountAlignment.h"
#include "Refraction.hpp"
#include "TeenAstroLA3.hpp"
#include <TinyGPS++.h>
struct MountPeripherals {
  Pushto PushtoStatus;
  bool hasFocuser;
  bool hasGNSS;
};

/** Config group: identity (mount type, name, meridian flip) and peripherals (push-to, focuser, GNSS). */
struct MountConfig {
  MountIdentity identity;
  MountPeripherals peripherals;
};

struct MountMotorsEncoders {
  bool enableMotor;
  MotorAxis motorA1;
  MotorAxis motorA2;
  bool enableEncoder;
  EncoderSync EncodeSyncMode;
  EncoderAxis encoderA1;
  EncoderAxis encoderA2;
  interval minInterval1;
  interval maxInterval1;
  interval minInterval2;
  interval maxInterval2;
  bool reboot_unit;
};

#ifdef RETICULE_LED_PINS
struct MountReticule {
  int reticuleBrightness;
};
#endif

// -----------------------------------------------------------------------------
// Mount class
// -----------------------------------------------------------------------------
class Mount {
public:
  Mount();
  // Phase 2a: query methods
  bool isParked() const;
  bool isAtHome() const;
  bool isSlewing() const;
  PoleSide getPoleSide() const;
  PoleSide getTargetPoleSide() const;
  bool hasTracking() const;
  bool hasGuiding() const;
  ErrorsTraking lastError() const;
  void setError(ErrorsTraking e);
  void clearError();
  bool isAltAZ() const;
  // Phase 3: accessors (alternative to direct member access)
  ParkState getParkStatus() const;
  bool getParkSaved() const;
  MountParkHome& getParkHome();
  const MountParkHome& getParkHome() const;
  MountIdentity& getIdentity();
  const MountIdentity& getIdentity() const;
  MountPeripherals& getPeripherals();
  const MountPeripherals& getPeripherals() const;
  MountConfig& getConfig();
  const MountConfig& getConfig() const;
  MountLimits& getLimits();
  const MountLimits& getLimits() const;
  MountAlignment& getAlignment();
  const MountAlignment& getAlignment() const;
  MountTargetCurrent& getTargetCurrent();
  const MountTargetCurrent& getTargetCurrent() const;
  MountErrors& getErrors();
  const MountErrors& getErrors() const;
  MountTracking& getTracking();
  const MountTracking& getTracking() const;
  MountGuiding& getGuiding();
  const MountGuiding& getGuiding() const;
  MountMotorsEncoders& getMotorsEncoders();
  const MountMotorsEncoders& getMotorsEncoders() const;
  MountAxes& getAxes();
  const MountAxes& getAxes() const;
#ifdef RETICULE_LED_PINS
  MountReticule& getReticule();
  const MountReticule& getReticule() const;
#endif
  // Phase 2b: setters and limit helpers
  void setParkStatus(ParkState s);
  void setTracking(bool on);
  void setSiderealMode(SID_Mode m);
  void setGuidingState(Guiding g);
  void setTargetRaDec(double ra, double dec);
  void setTargetAltAz(double alt, double azm);
  void setTargetPoleSide(PoleSide s);
  void abortSlew();
  void setMeridianFlip(MeridianFlip m);
  // Phase 2c: limit and axis API via mount.limits.* (MountLimits); axis EEPROM via limits.initLimit etc.
  void setupST4();
  void checkST4();
  // Phase 2d: tracking and guiding
  void applyTrackingRate();
  void setTrackingRate(double rHA, double rDEC);
  void computeTrackingRate(bool apply);
  void enableGuideRate(int g, bool force = false);
  void enableST4GuideRate();
  void enableRecenterGuideRate();
  void resetGuideRate();
  void initMaxRate();
  void setRates(double maxslewrate);
  void setAcceleration();
  void updateSideral();
  void safetyCheck(bool forceTracking);
  // Phase 2e: Park, Home, MoveTo, Move, Guide
  bool setPark();
  void unsetPark();
  void parkClearBacklash();
  void finalizePark();
  byte park();
  bool syncAtPark();
  bool iniAtPark();
  void unpark();
  void moveTo();
  void decayModeTracking();
  void decayModeGoto();
  bool setHome();
  void unsetHome();
  bool goHome();
  void finalizeHome();
  bool syncAtHome();
  void initHome();
  void guide();
  void stopGuiding();
  bool isGuidingStar() const;
  void startSideralTracking();
  void checkEndOfMoveAxisAtRate();
  void checkSpiral();
  // Phase 2f: Goto and sync
  void stepToAngle(long Axis1, long Axis2, double* AngleAxis1, double* AngleAxis2, PoleSide* Side) const;
  void angle2Step(double AngleAxis1, double AngleAxis2, PoleSide Side, long* Axis1, long* Axis2) const;
  void syncAxis(const long* axis1, const long* axis2);
  void gotoAxis(const long* axis1Target, const long* axis2Target);
  bool syncInstr(Coord_IN* instr, PoleSide Side);
  bool syncEqu(Coord_EQ* EQ_T, PoleSide Side, double Lat);
  bool syncAzAlt(Coord_HO* HO_T, PoleSide Side);
  void syncTwithE();
  void syncEwithT();
  bool autoSyncWithEncoder(EncoderSync mode);
  void getInstrDeg(double* A1, double* A2, double* A3) const;
  Coord_IN getInstr() const;
  Coord_IN getInstrE() const;
  Coord_IN getInstrTarget() const;
  Coord_EQ getEqu(double Lat) const;
  Coord_EQ getEquE(double Lat) const;
  Coord_EQ getEquTarget(double Lat) const;
  Coord_HO getHorTopo() const;
  Coord_HO getHorETopo() const;
  Coord_HO getHorAppTarget() const;
  bool predictTarget(const double& Axis1_in, const double& Axis2_in, const PoleSide& inputSide,
    long& Axis1_out, long& Axis2_out, PoleSide& outputSide) const;
  byte goToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat);
  byte goToHor(Coord_HO HO_T, PoleSide preferedPoleSide);
  ErrorsGoTo goTo(long thisTargetAxis1, long thisTargetAxis2);
  ErrorsGoTo flip();
  // Refraction (options for pole, goto, tracking; uses temperature, pressure)
  LA3::RefrOpt refrOptForPole() const;
  LA3::RefrOpt refrOptForGoto() const;
  LA3::RefrOpt refrOptForTracking() const;
  // Phase 2g: Init and application lifecycle
  void init();
  void updateRatios(bool deleteAlignment, bool deleteHP);
  void loadGuideRatesFromEEPROM();
  void loadSiderealFromEEPROMAndStartTimers();
  void configureFaultPins();
  void onSiderealTick(long phase, bool forceTracking);
  void updateEncoderSync(bool runThisLoop);
  void updateStatusLed(int ledPin) const;

private:
  void rateFromMovingTarget(Coord_EQ& EQprev, Coord_EQ& EQnext, double TimeRange, PoleSide side, bool refr, double& A1_trackingRate, double& A2_trackingRate);
  void doCompensationCalc();

public:
  // Guide (delegates to guiding)
  bool stopIfMountError();
  void performPulseGuiding();
  void performST4Guiding();
  void performGuidingRecenter();
  void performGuidingAtRate();

  // Move (axis motion)
  void moveAxis1(bool BW, Guiding Mode);
  void moveAxisAtRate1(double newrate);
  void stopAxis1();
  void moveAxis2(bool BW, Guiding Mode);
  void moveAxisAtRate2(double newrate);
  void stopAxis2();

  MountParkHome parkHome;
  MountConfig config;
  TinyGPSPlus gnss;
  RefractionFlags refraction;
  double temperature = 10.0;
  double pressure = 110.0;
  MountLimits limits;
  MountAlignment alignment;
  MountTargetCurrent targetCurrent;
  MountErrors errors;
  MountTracking tracking;
  MountGuiding guiding;
  MountMotorsEncoders motorsEncoders;
  MountAxes axes;
  MountAxisLimitManager limitManager;
  MountST4 st4;
  MountParkHomeController parkHomeController;
#ifdef RETICULE_LED_PINS
  MountReticule reticule;
#endif
};

extern Mount mount;
