import 'dart:math';
import 'package:flutter/material.dart';

/// Astronomical algorithms for Sun, Moon, and planet positions.
/// Based on Jean Meeus "Astronomical Algorithms" (simplified, visual accuracy).

// ---------------------------------------------------------------------------
// Julian Date
// ---------------------------------------------------------------------------

double julianDate(DateTime utc) {
  int y = utc.year;
  int m = utc.month;
  final d = utc.day +
      utc.hour / 24.0 +
      utc.minute / 1440.0 +
      utc.second / 86400.0;
  if (m <= 2) {
    y -= 1;
    m += 12;
  }
  final a = y ~/ 100;
  final b = 2 - a + a ~/ 4;
  return (365.25 * (y + 4716)).floor() +
      (30.6001 * (m + 1)).floor() +
      d +
      b -
      1524.5;
}

double _jdCenturies(double jd) => (jd - 2451545.0) / 36525.0;

// ---------------------------------------------------------------------------
// Angle utilities
// ---------------------------------------------------------------------------

const _deg2rad = pi / 180.0;
const _rad2deg = 180.0 / pi;

double _normDeg(double d) {
  d = d % 360.0;
  if (d < 0) d += 360.0;
  return d;
}

double _sinD(double deg) => sin(deg * _deg2rad);
double _cosD(double deg) => cos(deg * _deg2rad);

// ---------------------------------------------------------------------------
// Obliquity of the ecliptic (Meeus Ch. 22)
// ---------------------------------------------------------------------------

double meanObliquity(double t) {
  return 23.439291 - 0.0130042 * t - 1.64e-7 * t * t + 5.04e-7 * t * t * t;
}

// ---------------------------------------------------------------------------
// Ecliptic -> Equatorial conversion
// ---------------------------------------------------------------------------

/// Convert ecliptic longitude/latitude (degrees) to RA (hours) and Dec (degrees).
(double ra, double dec) eclipticToEquatorial(
    double lonDeg, double latDeg, double obliquity) {
  final lon = lonDeg * _deg2rad;
  final lat = latDeg * _deg2rad;
  final eps = obliquity * _deg2rad;

  final sinDec = sin(lat) * cos(eps) + cos(lat) * sin(eps) * sin(lon);
  final dec = asin(sinDec.clamp(-1.0, 1.0)) * _rad2deg;

  final y = sin(lon) * cos(eps) - tan(lat) * sin(eps);
  final x = cos(lon);
  var ra = atan2(y, x) * _rad2deg;
  ra = _normDeg(ra) / 15.0; // convert to hours

  return (ra, dec);
}

// ---------------------------------------------------------------------------
// Sun position (Meeus Ch. 25, low accuracy ~1')
// ---------------------------------------------------------------------------

CelestialBody sunPosition(double jd) {
  final t = _jdCenturies(jd);

  // Geometric mean longitude and anomaly
  final l0 = _normDeg(280.46646 + 36000.76983 * t + 0.0003032 * t * t);
  final m = _normDeg(357.52911 + 35999.05029 * t - 0.0001537 * t * t);

  // Equation of center
  final c = (1.914602 - 0.004817 * t - 0.000014 * t * t) * _sinD(m) +
      (0.019993 - 0.000101 * t) * _sinD(2 * m) +
      0.000289 * _sinD(3 * m);

  final sunLon = _normDeg(l0 + c);
  final omega = 125.04 - 1934.136 * t;
  final apparentLon = sunLon - 0.00569 - 0.00478 * _sinD(omega);

  final eps = meanObliquity(t) + 0.00256 * _cosD(omega);
  final (ra, dec) = eclipticToEquatorial(apparentLon, 0, eps);

  return CelestialBody(
    name: 'Sun',
    ra: ra,
    dec: dec,
    mag: -26.74,
    color: const Color(0xFFFFD700),
    radius: 8.0,
  );
}

// ---------------------------------------------------------------------------
// Moon position (Meeus Ch. 47, truncated ~20 terms, ~10' accuracy)
// ---------------------------------------------------------------------------

CelestialBody moonPosition(double jd) {
  final t = _jdCenturies(jd);
  final t2 = t * t;
  final t3 = t2 * t;

  // Fundamental arguments (degrees)
  final lp =
      _normDeg(218.3164477 + 481267.88123421 * t - 0.0015786 * t2 + t3 / 538841);
  final d =
      _normDeg(297.8501921 + 445267.1114034 * t - 0.0018819 * t2 + t3 / 545868);
  final m =
      _normDeg(357.5291092 + 35999.0502909 * t - 0.0001536 * t2 + t3 / 24490000);
  final mp =
      _normDeg(134.9633964 + 477198.8675055 * t + 0.0087414 * t2 + t3 / 69699);
  final f =
      _normDeg(93.2720950 + 483202.0175233 * t - 0.0036539 * t2 - t3 / 3526000);

  // Longitude terms (most significant)
  double sumL = 0;
  sumL += 6288774 * _sinD(mp);
  sumL += 1274027 * _sinD(2 * d - mp);
  sumL += 658314 * _sinD(2 * d);
  sumL += 213618 * _sinD(2 * mp);
  sumL += -185116 * _sinD(m);
  sumL += -114332 * _sinD(2 * f);
  sumL += 58793 * _sinD(2 * d - 2 * mp);
  sumL += 57066 * _sinD(2 * d - m - mp);
  sumL += 53322 * _sinD(2 * d + mp);
  sumL += 45758 * _sinD(2 * d - m);
  sumL += -40923 * _sinD(m - mp);
  sumL += -34720 * _sinD(d);
  sumL += -30383 * _sinD(m + mp);
  sumL += 15327 * _sinD(2 * d - 2 * f);
  sumL += -12528 * _sinD(mp + 2 * f);
  sumL += 10980 * _sinD(mp - 2 * f);
  sumL += 10675 * _sinD(4 * d - mp);
  sumL += 10034 * _sinD(3 * mp);
  sumL += 8548 * _sinD(4 * d - 2 * mp);

  // Latitude terms
  double sumB = 0;
  sumB += 5128122 * _sinD(f);
  sumB += 280602 * _sinD(mp + f);
  sumB += 277693 * _sinD(mp - f);
  sumB += 173237 * _sinD(2 * d - f);
  sumB += 55413 * _sinD(2 * d - mp + f);
  sumB += 46271 * _sinD(2 * d - mp - f);
  sumB += 32573 * _sinD(2 * d + f);
  sumB += 17198 * _sinD(2 * mp + f);
  sumB += 9266 * _sinD(2 * d + mp - f);
  sumB += 8822 * _sinD(2 * mp - f);
  sumB += 8216 * _sinD(2 * d - m - f);
  sumB += 4324 * _sinD(2 * d - 2 * mp - f);

  final lonMoon = lp + sumL / 1000000.0;
  final latMoon = sumB / 1000000.0;

  final eps = meanObliquity(t);
  final (ra, dec) = eclipticToEquatorial(lonMoon, latMoon, eps);

  return CelestialBody(
    name: 'Moon',
    ra: ra,
    dec: dec,
    mag: -12.7,
    color: const Color(0xFFE0E0E0),
    radius: 7.0,
  );
}

// ---------------------------------------------------------------------------
// Planet positions (Meeus Ch. 31-33, Keplerian + perturbation)
// ---------------------------------------------------------------------------

enum Planet { mercury, venus, mars, jupiter, saturn, uranus, neptune }

class _OrbitalElements {
  final double l0, l1; // mean longitude (deg, deg/century)
  final double a0, a1; // semi-major axis (AU, AU/century)
  final double e0, e1; // eccentricity
  final double i0, i1; // inclination (deg)
  final double om0, om1; // longitude of ascending node (deg)
  final double wp0, wp1; // longitude of perihelion (deg)

  const _OrbitalElements(this.l0, this.l1, this.a0, this.a1, this.e0, this.e1,
      this.i0, this.i1, this.om0, this.om1, this.wp0, this.wp1);
}

// J2000.0 orbital elements and rates (Standish 1992, from JPL)
const _elements = <Planet, _OrbitalElements>{
  Planet.mercury: _OrbitalElements(
      252.25032, 149472.67411, 0.38709927, 0.00000037, 0.20563593, 0.00001906,
      7.00497902, -0.00594749, 48.33076593, -0.12534081, 77.45779628, 0.16047689),
  Planet.venus: _OrbitalElements(
      181.97909, 58517.81539, 0.72333566, 0.00000390, 0.00677672, -0.00004107,
      3.39467605, -0.00078890, 76.67984255, -0.27769418, 131.60246718, 0.00268329),
  Planet.mars: _OrbitalElements(
      -4.55343, 19140.30268, 1.52371034, 0.00001847, 0.09339410, 0.00007882,
      1.84969142, -0.00813131, 49.55953891, -0.29257343, -23.94362959, 0.44441088),
  Planet.jupiter: _OrbitalElements(
      34.39644, 3034.74612, 5.20288700, -0.00011607, 0.04838624, -0.00013253,
      1.30439695, -0.00183714, 100.47390909, 0.20469106, 14.72847983, 0.21252668),
  Planet.saturn: _OrbitalElements(
      49.95424, 1222.49362, 9.53667594, -0.00125060, 0.05386179, -0.00050991,
      2.48599187, 0.00193609, 113.66242448, -0.28867794, 92.59887831, -0.41897216),
  Planet.uranus: _OrbitalElements(
      313.23810, 428.48202, 19.18916464, -0.00196176, 0.04725744, -0.00004397,
      0.77263783, -0.00242939, 74.01692503, 0.04240589, 170.95427630, 0.40805281),
  Planet.neptune: _OrbitalElements(
      -55.12002, 218.45945, 30.06992276, 0.00026291, 0.00859048, 0.00005105,
      1.77004347, 0.00035372, 131.78422574, -0.00508664, 44.96476227, -0.32241464),
};

const _planetNames = <Planet, String>{
  Planet.mercury: 'Mercury',
  Planet.venus: 'Venus',
  Planet.mars: 'Mars',
  Planet.jupiter: 'Jupiter',
  Planet.saturn: 'Saturn',
  Planet.uranus: 'Uranus',
  Planet.neptune: 'Neptune',
};

const _planetColors = <Planet, Color>{
  Planet.mercury: Color(0xFFB0B0B0),
  Planet.venus: Color(0xFFFFF8DC),
  Planet.mars: Color(0xFFCD5C5C),
  Planet.jupiter: Color(0xFFDEB887),
  Planet.saturn: Color(0xFFF5DEB3),
  Planet.uranus: Color(0xFF87CEEB),
  Planet.neptune: Color(0xFF4169E1),
};

/// Solve Kepler's equation M = E - e*sin(E) by iteration.
double _solveKepler(double mDeg, double e) {
  final mRad = mDeg * _deg2rad;
  var eAnom = mRad;
  for (int i = 0; i < 20; i++) {
    final dE = (mRad - eAnom + e * sin(eAnom)) / (1 - e * cos(eAnom));
    eAnom += dE;
    if (dE.abs() < 1e-10) break;
  }
  return eAnom * _rad2deg;
}

CelestialBody planetPosition(Planet planet, double jd) {
  final t = _jdCenturies(jd);
  final el = _elements[planet]!;

  // Current orbital elements
  final l = _normDeg(el.l0 + el.l1 * t);
  final a = el.a0 + el.a1 * t;
  final e = el.e0 + el.e1 * t;
  final inc = el.i0 + el.i1 * t;
  final om = el.om0 + el.om1 * t;
  final wp = el.wp0 + el.wp1 * t;

  // Mean anomaly and eccentric anomaly
  final mAnom = _normDeg(l - wp);
  final eAnomDeg = _solveKepler(mAnom, e);

  // Heliocentric position in orbital plane
  final xp = a * (_cosD(eAnomDeg) - e);
  final yp = a * sqrt(1 - e * e) * _sinD(eAnomDeg);

  // Rotate to ecliptic coordinates
  final argPeri = wp - om;
  final cosArg = _cosD(argPeri);
  final sinArg = _sinD(argPeri);
  final cosOm = _cosD(om);
  final sinOm = _sinD(om);
  final cosInc = _cosD(inc);
  final sinInc = _sinD(inc);

  final xEcl = (cosArg * cosOm - sinArg * sinOm * cosInc) * xp +
      (-sinArg * cosOm - cosArg * sinOm * cosInc) * yp;
  final yEcl = (cosArg * sinOm + sinArg * cosOm * cosInc) * xp +
      (-sinArg * sinOm + cosArg * cosOm * cosInc) * yp;
  final zEcl = (sinArg * sinInc) * xp + (cosArg * sinInc) * yp;

  // Earth position (use same method)
  const earthEl = _OrbitalElements(
      100.46457, 35999.37245, 1.00000261, 0.00000562, 0.01671123, -0.00004392,
      -0.00001531, -0.01294668, 0.0, 0.0, 102.93768193, 0.32327364);

  final lE = _normDeg(earthEl.l0 + earthEl.l1 * t);
  final aE = earthEl.a0 + earthEl.a1 * t;
  final eE = earthEl.e0 + earthEl.e1 * t;
  final wpE = earthEl.wp0 + earthEl.wp1 * t;
  final mE = _normDeg(lE - wpE);
  final eeAnomDeg = _solveKepler(mE, eE);

  final xEarth = aE * (_cosD(eeAnomDeg) - eE);
  final yEarth = aE * sqrt(1 - eE * eE) * _sinD(eeAnomDeg);

  final argPeriE = wpE;
  final xEarthEcl = _cosD(argPeriE) * xEarth - _sinD(argPeriE) * yEarth;
  final yEarthEcl = _sinD(argPeriE) * xEarth + _cosD(argPeriE) * yEarth;
  const zEarthEcl = 0.0;

  // Geocentric ecliptic
  final dx = xEcl - xEarthEcl;
  final dy = yEcl - yEarthEcl;
  final dz = zEcl - zEarthEcl;

  var lonGeo = atan2(dy, dx) * _rad2deg;
  lonGeo = _normDeg(lonGeo);
  final dist = sqrt(dx * dx + dy * dy + dz * dz);
  final latGeo = asin((dz / dist).clamp(-1.0, 1.0)) * _rad2deg;

  final eps = meanObliquity(t);
  final (ra, dec) = eclipticToEquatorial(lonGeo, latGeo, eps);

  // Approximate visual magnitude
  final mag = _planetMag(planet, dist, a, e, mAnom);

  return CelestialBody(
    name: _planetNames[planet]!,
    ra: ra,
    dec: dec,
    mag: mag,
    color: _planetColors[planet]!,
    radius: planet == Planet.venus || planet == Planet.jupiter ? 5.0 : 4.0,
  );
}

double _planetMag(Planet p, double delta, double r, double e, double mAnom) {
  // Very rough visual magnitudes (phase angle ignored for simplicity)
  switch (p) {
    case Planet.mercury:
      return -0.36 + 5 * log(delta * r) / ln10;
    case Planet.venus:
      return -4.34 + 5 * log(delta * r) / ln10;
    case Planet.mars:
      return -1.51 + 5 * log(delta * r) / ln10;
    case Planet.jupiter:
      return -9.40 + 5 * log(delta * r) / ln10;
    case Planet.saturn:
      return -8.88 + 5 * log(delta * r) / ln10;
    case Planet.uranus:
      return -7.19 + 5 * log(delta * r) / ln10;
    case Planet.neptune:
      return -6.87 + 5 * log(delta * r) / ln10;
  }
}

/// Compute all solar system body positions for a given Julian Date.
List<CelestialBody> allBodies(double jd) {
  return [
    sunPosition(jd),
    moonPosition(jd),
    for (final p in Planet.values) planetPosition(p, jd),
  ];
}

// ---------------------------------------------------------------------------
// CelestialBody
// ---------------------------------------------------------------------------

class CelestialBody {
  final String name;
  final double ra;  // hours
  final double dec; // degrees
  final double mag;
  final Color color;
  final double radius; // rendering radius in px

  const CelestialBody({
    required this.name,
    required this.ra,
    required this.dec,
    required this.mag,
    required this.color,
    required this.radius,
  });
}
