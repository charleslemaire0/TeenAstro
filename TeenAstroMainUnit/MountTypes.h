#pragma once
/**
 * Mount and command protocol types and constants.
 * Shared enums, typedefs, and numeric constants used across Mount.h and the firmware.
 *
 * ErrorsGoTo is now defined in the shared TeenAstroCommandDef library
 * (CommandEnums.h) so that client and server stay in sync.
 */
#include <Arduino.h>
#include <CommandEnums.h>

// -----------------------------------------------------------------------------
// Type aliases
// -----------------------------------------------------------------------------
typedef double interval;
typedef double speed;

// -----------------------------------------------------------------------------
// Mount and behaviour enums
// -----------------------------------------------------------------------------
enum MountType { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
enum EncoderSync { ES_OFF, ES_60, ES_30, ES_15, ES_8, ES_4, ES_2, ES_ALWAYS };
enum Pushto { PT_OFF, PT_RADEC, PT_ALTAZ };
enum MeridianFlip { FLIP_NEVER, FLIP_ALIGN, FLIP_ALWAYS };
enum CheckMode { CHECKMODE_GOTO, CHECKMODE_TRACKING };
enum ParkState { PRK_UNPARKED, PRK_PARKING, PRK_PARKED };
enum TrackingCompensation { TC_UNKNOWN = -1, TC_RA = 1, TC_BOTH };
enum BacklashPhase { INIT, MOVE_IN, MOVE_OUT, DONE };

enum ErrorsTraking {
  ERRT_NONE,
  ERRT_MOTOR_FAULT,
  ERRT_ALT,
  ERRT_LIMIT_SENSE,
  ERRT_AXIS1,
  ERRT_AXIS2,
  ERRT_UNDER_POLE,
  ERRT_MERIDIAN
};

// When goTo() returns a tracking-style error, it uses lastError + 10 (ERRGOTO_* from ERRT_*).
// Do not reorder ErrorsTraking or ErrorsGoTo without updating Mount::goTo().
// ErrorsGoTo is now defined in CommandEnums.h (TeenAstroCommandDef library).

enum SID_Mode { SIDM_STAR, SIDM_SUN, SIDM_MOON, SIDM_TARGET };

enum Guiding { GuidingOFF, GuidingPulse, GuidingST4, GuidingRecenter, GuidingAtRate };

enum GuideRate { RG, RC, RM, RS, RX };

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------
#define BAUD 57600

#define TrackingStar  1
#define TrackingSolar 0.99726956632
#define TrackingLunar 0.96236513150

#define DefaultR0 1
#define DefaultR1 4
#define DefaultR2 16
#define DefaultR3 64
#ifndef DefaultR4
#define DefaultR4 600
#endif

const double mastersiderealClockSpeed = 997269.5625;
const double masterClockSpeed = 1000000.0;  // reference frequency for tick
const double HzCf = masterClockSpeed / 60.0;
