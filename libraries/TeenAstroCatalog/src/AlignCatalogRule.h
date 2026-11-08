#pragma once

// How the alignment star list treats the sky after at least one star is in.
//
// mountKind: 0 equatorial (German or fork), 1 alt-az (German or fork).
// session:   0 two-star, 1 four-star, 2 first half of 3+3, 3 the other half.
enum AlignCatalogRule {
  ALIGN_EQ_EITHER = 0,  // both pier sides; the other side is only preferred
  ALIGN_EQ_THIS_SIDE,   // only the pier side of the stars already taken
  ALIGN_EQ_OTHER_SIDE,  // only the opposite pier side
  ALIGN_AZ_SPREAD,      // no azimuth cut; prefer about 90 degrees away
  ALIGN_AZ_SECTOR,      // drop stars more than 100 degrees of azimuth away
  ALIGN_AZ_OPPOSITE,    // drop stars within 70 degrees; the other azimuth
  ALIGN_AZ_FULL         // no azimuth cut; prefer stars far from those taken
};

inline AlignCatalogRule alignCatalogRule(int mountKind, int session) {
  if (mountKind == 0) {
    if (session == 1 || session == 2) return ALIGN_EQ_THIS_SIDE;
    if (session == 3) return ALIGN_EQ_OTHER_SIDE;
    return ALIGN_EQ_EITHER;
  }
  // An alt-az mount never goes over the zenith, so a four-star session can
  // use the whole sky from one side of the mount.
  if (session == 0) return ALIGN_AZ_SPREAD;
  if (session == 1) return ALIGN_AZ_FULL;
  if (session == 3) return ALIGN_AZ_OPPOSITE;
  return ALIGN_AZ_SECTOR;
}
