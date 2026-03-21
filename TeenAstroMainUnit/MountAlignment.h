#pragma once
/**
 * Star alignment state and config: transformation matrix (CoordConv), valid flag, and alignment options.
 * Owned by Mount as mount.alignment.
 */
#include <Arduino.h>
#include <TeenAstroCoordConv.hpp>

enum AlignPhase : uint8_t {
  ALIGN_IDLE     = 0,
  ALIGN_SELECT   = 1,
  ALIGN_SLEW     = 2,
  ALIGN_RECENTER = 3
};

struct MountAlignment {
  CoordConv conv;
  bool hasValid = false;
  byte maxAlignNumStar = 0;
  bool autoAlignmentBySync = false;
  AlignPhase alignPhase = ALIGN_IDLE;
  uint8_t alignStarNum  = 0;
  char alignStarName[16] = {0};  // name of current alignment star (set by app via :SXAs,name#)
};
