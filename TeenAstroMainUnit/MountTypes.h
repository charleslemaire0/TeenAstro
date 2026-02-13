#pragma once
/**
 * Mount and command protocol types and constants.
 * Shared enums, typedefs, and numeric constants used across Global.h and the firmware.
 */
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Type aliases
// -----------------------------------------------------------------------------
typedef double interval;
typedef double speed;

// -----------------------------------------------------------------------------
// Mount and behaviour enums
// -----------------------------------------------------------------------------
enum Mount { MOUNT_UNDEFINED, MOUNT_TYPE_GEM, MOUNT_TYPE_FORK, MOUNT_TYPE_ALTAZM, MOUNT_TYPE_FORK_ALT };
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

enum ErrorsGoTo {
  ERRGOTO_NONE,
  ERRGOTO_BELOWHORIZON,
  ERRGOTO_NOOBJECTSELECTED,
  ERRGOTO_SAMESIDE,
  ERRGOTO_PARKED,
  ERRGOTO_SLEWING,
  ERRGOTO_LIMITS,
  ERRGOTO_GUIDINGBUSY,
  ERRGOTO_ABOVEOVERHEAD,
  ERRGOTO_MOTOR,
  ERRGOTO____,
  ERRGOTO_MOTOR_FAULT,
  ERRGOTO_ALT,
  ERRGOTO_LIMIT_SENSE,
  ERRGOTO_AXIS1,
  ERRGOTO_AXIS2,
  ERRGOTO_UNDER_POLE,
  ERRGOTO_MERIDIAN
};

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
const double masterClockSpeed = 1000000;  // reference frequency for tick
const double HzCf = masterClockSpeed / 60.0;
