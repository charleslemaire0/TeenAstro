#pragma once

// Gear-check star list. Named stars only, scored like the alignment list
// (* … ****). Equatorial and alt-azimuth mounts want a different move.
// On a German equatorial, after the first star, the other pier side is omitted.

struct GearStarGeom {
  int mountKind;     // 0 equatorial, 1 alt-azimuth
  int leg;           // 0 sync, 1 mostly axis 1, 2 mostly axis 2
  bool gemSamePier;  // German equatorial: stay on the sync star's pier side
  double ha;         // hour angle, degrees, signed
  double dec;
  double alt;
  double az;
  double mag;
  double refHa;
  double refDec;
  double refAlt;
  double refAz;
  bool hasRef;
  int sideSign;      // +1 / -1 from the sync star, 0 if that star is on the meridian
};

struct GearStarScore {
  bool keep;
  double score;
};

inline double gearWrapSigned(double deg) {
  while (deg > 180.0) deg -= 360.0;
  while (deg < -180.0) deg += 360.0;
  return deg;
}

inline double gearAzDelta(double a, double b) {
  double d = a - b;
  if (d < 0.0) d = -d;
  while (d >= 360.0) d -= 360.0;
  if (d > 180.0) d = 360.0 - d;
  return d;
}

// Hour angle past 12° of the meridian is one pier side. Inside that strip
// the side is not decided.
inline int gearHaSign(double haDeg) {
  if (haDeg > 12.0) return 1;
  if (haDeg < -12.0) return -1;
  return 0;
}

inline GearStarScore gearStarScore(const GearStarGeom& g) {
  GearStarScore out;
  out.keep = false;
  out.score = 0.0;
  if (g.alt < 10.0) return out;

  if (g.gemSamePier && g.hasRef && g.sideSign != 0) {
    if (gearHaSign(g.ha) != g.sideSign) return out;
  }

  double altScore = 36.0 - (g.alt > 42.0 ? g.alt - 42.0 : 42.0 - g.alt);
  if (altScore < 0.0) altScore = 0.0;
  double bright = (4.0 - g.mag) * 6.0;
  if (bright < 0.0) bright = 0.0;

  if (!g.hasRef || g.leg == 0) {
    out.keep = true;
    out.score = bright + altScore + 10.0;
    return out;
  }

  if (g.mountKind == 0) {
    const double dHa = gearAzDelta(g.ha, g.refHa);
    const double dDec = g.dec > g.refDec ? g.dec - g.refDec : g.refDec - g.dec;
    if (g.leg == 1) {
      if (dHa < 8.0) return out;
      out.keep = true;
      out.score = dHa * 1.4 - dDec * 1.6 + altScore + bright;
      if (dHa >= 2.0 * dDec) out.score += 30.0;
    } else {
      if (dDec < 8.0) return out;
      out.keep = true;
      out.score = dDec * 1.8 - dHa * 1.2 + altScore + bright;
      if (dDec >= 2.0 * dHa) out.score += 30.0;
    }
  } else {
    const double dAz = gearAzDelta(g.az, g.refAz);
    const double dAlt = g.alt > g.refAlt ? g.alt - g.refAlt : g.refAlt - g.alt;
    if (g.leg == 1) {
      if (dAz < 8.0) return out;
      out.keep = true;
      out.score = dAz * 1.2 - dAlt * 1.5 + altScore + bright;
      if (dAz >= 2.0 * dAlt) out.score += 30.0;
    } else {
      if (dAlt < 8.0) return out;
      out.keep = true;
      out.score = dAlt * 2.2 - dAz * 0.6 + altScore + bright;
      if (dAz < 40.0) out.score += 30.0;
    }
  }
  if (out.score < 1.0) out.score = 1.0;
  return out;
}
