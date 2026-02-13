#pragma once
/**
 * Global.h - Main firmware include and global state.
 *
 * Layout:
 * - Types and constants live in MountTypes.h (enums, typedefs, BAUD, Tracking*, DefaultR*, masterClockSpeed, HzCf).
 * - Mount runtime state is in a single MountState instance `mount` (park/home, slew, limits, target/current,
 *   errors, tracking, guiding, axis state, motors, encoders, feature flags).
 * - Command/serial state is in `commandState` (reply/command buffers, highPrecision, baudRate, S_SHC, S_USB).
 * - Environment/refraction is in `environment` (temperature, pressure, doesRefraction).
 * - EEPROM mount index and getMountAddress() are in the EEPROM module (EEPROM_address.h + EEPROM.cpp).
 *
 * Backward compatibility: Legacy names are macros expanding to struct members (e.g. parkStatus -> mount.parkStatus).
 * reply, command, and highPrecision remain global (no macros) to avoid clashes with parameter names.
 * CommandState uses internal names baudRate_, S_SHC_, S_USB_; macros baudRate, S_SHC, S_USB map to them.
 * Migration path: Update call sites to use mount.* / commandState.* / environment.* and then remove the macros.
 */
#include "MountTypes.h"
#include <Arduino.h>
#include <math.h>
#include <TeenAstroCoordConv.hpp>
#include <TeenAstroMath.h>
#include <TinyGPS++.h>
#include "Config.TeenAstro.h"
#include "timerLoop.hpp"
#include "TelTimer.hpp"
#include "Site.hpp"
#include "Command.h"
#include "EEPROM_address.h"
#include "XEEPROM.hpp"
#include "Refraction.hpp"
#include "Axis.hpp"
#include "AxisEncoder.hpp"
#include "TeenAstroLA3.hpp"
#include "TeenAstroCoord_LO.hpp"
#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"

// -----------------------------------------------------------------------------
// Alignment / GNSS (shared; not part of mount state)
// -----------------------------------------------------------------------------
extern TinyGPSPlus gps;
extern CoordConv alignment;
extern bool hasStarAlignment;

// -----------------------------------------------------------------------------
// Environment and refraction
// -----------------------------------------------------------------------------
struct Environment {
  double temperature;
  double pressure;
  RefractionFlags doesRefraction;
};
extern Environment environment;

inline LA3::RefrOpt RefrOptForPole()
{
  return { environment.doesRefraction.forPole, environment.temperature, environment.pressure };
}
inline LA3::RefrOpt RefrOptForGoto()
{
  return { environment.doesRefraction.forGoto, environment.temperature, environment.pressure };
}
inline LA3::RefrOpt RefrOptForTracking()
{
  return { environment.doesRefraction.forTracking, environment.temperature, environment.pressure };
}

// -----------------------------------------------------------------------------
// Mount state (park/home, slew, identity, peripherals, sidereal, motors, encoders,
// axis, target/current, limits, errors, tracking, guiding, reticule)
// -----------------------------------------------------------------------------
struct MountState {
  MountState();
  // Park / home
  ParkState parkStatus;
  bool parkSaved;
  bool homeSaved;
  bool atHome;
  bool homeMount;
  unsigned int slewSettleDuration;
  unsigned long lastSettleTime;
  bool settling;
  BacklashPhase backlashStatus;
  // Mount identity and behaviour
  bool DecayModeTrack;
  MeridianFlip meridianFlip;
  Mount mountType;
  char mountName[maxNumMount][MountNameLen];
  bool isMountTypeFix;
  byte maxAlignNumStar;
  bool autoAlignmentBySync;
  // Peripherals
  Pushto PushtoStatus;
  bool hasFocuser;
  bool hasGNSS;
  // Sidereal / timing
  TrackingCompensation trackComp;
  double siderealClockSpeed;
  bool reboot_unit;
  interval minInterval1;
  interval maxInterval1;
  interval minInterval2;
  interval maxInterval2;
  float pulseGuideRate;
  double DegreesForAcceleration;
  // Motors
  bool enableMotor;
  MotorAxis motorA1;
  MotorAxis motorA2;
  // Encoders
  bool enableEncoder;
  EncoderSync EncodeSyncMode;
  EncoderAxis encoderA1;
  EncoderAxis encoderA2;
  // Axis (geo, status)
  GeoAxis geoA1;
  GeoAxis geoA2;
  StatusAxis staA1;
  StatusAxis staA2;
  // Target / current
  PoleSide newTargetPoleSide;
  double newTargetAlt;
  double newTargetAzm;
  double newTargetDec;
  double newTargetRA;
  double currentAzm;
  double currentAlt;
  // Limits
  int minAlt;
  int maxAlt;
  long minutesPastMeridianGOTOE;
  long minutesPastMeridianGOTOW;
  double underPoleLimitGOTO;
  int distanceFromPoleToKeepTrackingOn;
  // Errors
  ErrorsTraking lastError;
  // Slew / spiral
  volatile bool movingTo;
  bool doSpiral;
  double SpiralFOV;
  // Tracking
  bool lastSideralTracking;
  volatile bool sideralTracking;
  volatile SID_Mode sideralMode;
  double RequestedTrackingRateHA;
  double RequestedTrackingRateDEC;
  long storedTrakingRateRA;
  long storedTrakingRateDEC;
  volatile Guiding GuidingState;
  Guiding lastGuidingState;
  unsigned long lastSetTrakingEnable;
  unsigned long lastSecurityCheck;
  bool abortSlew;
  // Guiding
  double guideRates[5];
  volatile byte activeGuideRate;
  volatile byte recenterGuideRate;
  GuideAxis guideA1;
  GuideAxis guideA2;
#ifdef RETICULE_LED_PINS
  int reticuleBrightness;
#endif
};
extern MountState mount;

// -----------------------------------------------------------------------------
// Command / serial (CommandState and S_SHC/S_USB in Command.h; reply/command/highPrecision global to avoid macro clashes)
// -----------------------------------------------------------------------------
#include "CommandConstants.h"
extern char reply[REPLY_BUFFER_LEN];
extern char command[CMD_BUFFER_LEN];
extern bool highPrecision;

// -----------------------------------------------------------------------------
// Backward compatibility: legacy names -> struct members (to be removed after migration)
// -----------------------------------------------------------------------------
#define parkStatus           (mount.parkStatus)
#define parkSaved            (mount.parkSaved)
#define homeSaved            (mount.homeSaved)
#define atHome               (mount.atHome)
#define homeMount            (mount.homeMount)
#define slewSettleDuration   (mount.slewSettleDuration)
#define lastSettleTime       (mount.lastSettleTime)
#define settling             (mount.settling)
#define backlashStatus       (mount.backlashStatus)
#define DecayModeTrack       (mount.DecayModeTrack)
#define meridianFlip         (mount.meridianFlip)
#define mountType            (mount.mountType)
#define mountName            (mount.mountName)
#define isMountTypeFix       (mount.isMountTypeFix)
#define maxAlignNumStar      (mount.maxAlignNumStar)
#define autoAlignmentBySync  (mount.autoAlignmentBySync)
#define PushtoStatus         (mount.PushtoStatus)
#define hasFocuser           (mount.hasFocuser)
#define hasGNSS              (mount.hasGNSS)
#define trackComp            (mount.trackComp)
#define siderealClockSpeed   (mount.siderealClockSpeed)
#define reboot_unit          (mount.reboot_unit)
#define minInterval1         (mount.minInterval1)
#define maxInterval1         (mount.maxInterval1)
#define minInterval2         (mount.minInterval2)
#define maxInterval2         (mount.maxInterval2)
#define pulseGuideRate       (mount.pulseGuideRate)
#define DegreesForAcceleration (mount.DegreesForAcceleration)
#define enableMotor          (mount.enableMotor)
#define motorA1              (mount.motorA1)
#define motorA2              (mount.motorA2)
#define enableEncoder        (mount.enableEncoder)
#define EncodeSyncMode       (mount.EncodeSyncMode)
#define encoderA1            (mount.encoderA1)
#define encoderA2            (mount.encoderA2)
#define geoA1                (mount.geoA1)
#define geoA2                (mount.geoA2)
#define staA1                (mount.staA1)
#define staA2                (mount.staA2)
#define newTargetPoleSide    (mount.newTargetPoleSide)
#define newTargetAlt         (mount.newTargetAlt)
#define newTargetAzm         (mount.newTargetAzm)
#define newTargetDec         (mount.newTargetDec)
#define newTargetRA          (mount.newTargetRA)
#define currentAzm           (mount.currentAzm)
#define currentAlt           (mount.currentAlt)
#define minAlt                (mount.minAlt)
#define maxAlt                (mount.maxAlt)
#define minutesPastMeridianGOTOE  (mount.minutesPastMeridianGOTOE)
#define minutesPastMeridianGOTOW (mount.minutesPastMeridianGOTOW)
#define underPoleLimitGOTO   (mount.underPoleLimitGOTO)
#define distanceFromPoleToKeepTrackingOn (mount.distanceFromPoleToKeepTrackingOn)
#define lastError            (mount.lastError)
#define movingTo             (mount.movingTo)
#define doSpiral             (mount.doSpiral)
#define SpiralFOV            (mount.SpiralFOV)
#define lastSideralTracking  (mount.lastSideralTracking)
#define sideralTracking      (mount.sideralTracking)
#define sideralMode          (mount.sideralMode)
#define RequestedTrackingRateHA   (mount.RequestedTrackingRateHA)
#define RequestedTrackingRateDEC  (mount.RequestedTrackingRateDEC)
#define storedTrakingRateRA (mount.storedTrakingRateRA)
#define storedTrakingRateDEC (mount.storedTrakingRateDEC)
#define GuidingState         (mount.GuidingState)
#define lastGuidingState     (mount.lastGuidingState)
#define lastSetTrakingEnable (mount.lastSetTrakingEnable)
#define lastSecurityCheck    (mount.lastSecurityCheck)
#define abortSlew            (mount.abortSlew)
#define guideRates           (mount.guideRates)
#define activeGuideRate      (mount.activeGuideRate)
#define recenterGuideRate    (mount.recenterGuideRate)
#define guideA1              (mount.guideA1)
#define guideA2              (mount.guideA2)
#ifdef RETICULE_LED_PINS
#define reticuleBrightness   (mount.reticuleBrightness)
#endif

#define baudRate             (commandState.baudRate_)
#define S_SHC                (commandState.S_SHC_)
#define S_USB                (commandState.S_USB_)

#define temperature          (environment.temperature)
#define pressure             (environment.pressure)
#define doesRefraction       (environment.doesRefraction)

#include "MainUnitDecl.h"
