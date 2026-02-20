/*
 * CommandTypes.h - Shared parameter types for TeenAstro LX200 protocol
 *
 * Typed structs for command parameters and responses, replacing raw
 * multi-argument function signatures with self-documenting types.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
//  Time / Date
// ---------------------------------------------------------------------------
struct TimeHMS {
  unsigned int hour;
  unsigned int minute;
  unsigned int second;
};

struct DateDMY {
  unsigned int day;
  unsigned int month;
  unsigned int year;
};

// ---------------------------------------------------------------------------
//  Coordinates
// ---------------------------------------------------------------------------
struct CoordRA {
  uint8_t  hours;
  uint8_t  minutes;
  uint8_t  seconds;
};

struct CoordDec {
  bool     positive;
  uint16_t degrees;
  uint8_t  minutes;
  uint8_t  seconds;
};

struct CoordAz {
  uint16_t degrees;
  uint8_t  minutes;
  uint8_t  seconds;
};

struct CoordAlt {
  bool     positive;
  uint16_t degrees;
  uint8_t  minutes;
  uint8_t  seconds;
};

// ---------------------------------------------------------------------------
//  Motor configuration (per axis)
// ---------------------------------------------------------------------------
struct MotorConfig {
  bool         reverse;
  float        backlash;
  float        backlashRate;
  float        totGear;
  float        stepPerRot;
  uint8_t      microStep;
  uint8_t      silentStep;
  unsigned int lowCurr;
  unsigned int highCurr;
};

// ---------------------------------------------------------------------------
//  Encoder configuration (per axis)
// ---------------------------------------------------------------------------
struct EncoderConfig {
  bool    reverse;
  float   pulsePerDegree;
  uint8_t autoSyncMode;
};

// ---------------------------------------------------------------------------
//  Focuser configuration
// ---------------------------------------------------------------------------
struct FocuserConfig {
  unsigned int startPosition;
  unsigned int maxPosition;
  unsigned int minSpeed;
  unsigned int maxSpeed;
  unsigned int cmdAcc;
  unsigned int manAcc;
  unsigned int manDec;
};

struct FocuserMotor {
  bool         reverse;
  unsigned int micro;
  unsigned int incr;
  unsigned int curr;
  unsigned int steprot;
};
