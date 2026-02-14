#pragma once
/**
 * Star alignment state and config: transformation matrix (CoordConv), valid flag, and alignment options.
 * Owned by Mount as mount.alignment.
 */
#include <Arduino.h>
#include <TeenAstroCoordConv.hpp>

struct MountAlignment {
  CoordConv conv;
  bool hasValid = false;
  byte maxAlignNumStar = 0;
  bool autoAlignmentBySync = false;
};
