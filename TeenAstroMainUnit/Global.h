#pragma once
/**
 * Global.h - Main firmware include and global state.
 *
 * Layout:
 * - Types and constants live in MountTypes.h (enums, typedefs, BAUD, Tracking*, DefaultR*, masterClockSpeed, HzCf).
 * - Mount runtime state is in a single MountState instance `mount` (park/home, slew, limits, target/current,
 *   errors, tracking, guiding, axis state, motors, encoders, feature flags). Use mount.* directly.
 * - Command/serial state is in `commandState` (baudRate_, S_SHC_, S_USB_). Use commandState.* directly.
 *   reply, command, and highPrecision remain global variables.
 * - Environment/refraction is in `environment` (temperature, pressure, doesRefraction). Use environment.* directly.
 * - EEPROM mount index and getMountAddress() are in the EEPROM module (EEPROM_address.h + EEPROM.cpp).
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

#include "MainUnitDecl.h"
