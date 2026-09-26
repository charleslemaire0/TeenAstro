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
  /// Session star count from :A0# (default 2) or :A0,m# / :A*,m# for equatorial mechanical-pole path.
  uint8_t alignNumStarsSession = 2;
  /// After two stars, provisional model built; :AP# discards it (cold baseline,
  /// hasValid=false, EE_Tvalid cleared) and syncs at the recentered star.
  bool alignPolarThirdPending = false;
  bool autoAlignmentBySync = false;
  AlignPhase alignPhase = ALIGN_IDLE;
  uint8_t alignStarNum  = 0;
  char alignStarName[16] = {0};  // name of current alignment star (set by app via :SXAs,name#)

  /// Rigid six degree of freedom session from :A0,r<n># / :A*,r<n>#. Zero means
  /// a classic two star session, where only T is fitted and the head stays zero.
  uint8_t alignRigidStars = 0;
  /// True once a fit produced non-zero head terms (cone / perp / axis2 index).
  bool hasRigid = false;
  /// RMS pointing residual of the last rigid fit, arcseconds.
  float rigidRmsArcsec = 0.f;

  /// User-defined pole error (radians, equatorial azimuth and altitude), optical
  /// cone and axis2 non-perpendicularity. When \p knownGeom is set, a 2-star
  /// alignment holds the cone and the perpendicularity and still estimates the pole.
  bool knownGeom = false;
  double knownPoleAz = 0;
  double knownPoleAlt = 0;
  double knownCone = 0;
  double knownPerp = 0;

  bool isRigidSession() const { return alignRigidStars >= COORDCONV_MIN_RIGID_STARS; }
};
