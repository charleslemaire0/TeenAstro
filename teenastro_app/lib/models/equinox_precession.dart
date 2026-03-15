import 'dart:math' as math;

/// J2000 (or other epoch) equatorial coordinates to JNow (apparent, equinox of date).
///
/// Logic matches TeenAstro Ephemeris::equatorialEquinoxToEquatorialJNowAtDateForT
/// (libraries/ephemeris-master/Ephemeris.cpp) and LX200Navigation.cpp SyncGotoLX200:
/// the SHC converts catalog (equinox) to JNow at UTC date before sending to the main unit.
/// The main unit is always in JNow. This module ensures the planetarium displays the same
/// positions as the mount would point to for a given sync/goto.
///
/// [raHours] and [decDeg] are in the given [equinox] (e.g. 2000 for J2000).
/// [jd] is the Julian Date for the moment of observation (UTC).
/// Returns (raJNowHours, decJNowDeg) for use with current LST.

double _limitDeg360(double value) {
  value = value % 360.0;
  if (value < 0) value += 360.0;
  return value;
}

double _limitHours24(double value) {
  value = value % 24.0;
  if (value < 0) value += 24.0;
  return value;
}

double _sinD(double deg) => math.sin(deg * math.pi / 180);
double _cosD(double deg) => math.cos(deg * math.pi / 180);
double _tanD(double deg) => math.tan(deg * math.pi / 180);

/// T from JD: (JD - 2451545.0 + time) / 36525, with time in [0,1) for fraction of day.
double _tFromJd(double jd) => (jd - 2451545.0) / 36525.0;

/// Obliquity and nutation for T. Matches Ephemeris::obliquityAndNutationForT.
/// Returns (obliquityDeg, deltaNutationArcsec, deltaObliquityArcsec).
(double obliquityDeg, double deltaNutation, double deltaObliquity)
    _obliquityAndNutationForT(double T) {
  final t2 = T * T;
  final t3 = t2 * T;

  double Ls = 280.4565 + T * 36000.7698 + t2 * 0.000303;
  Ls = _limitDeg360(Ls);
  double Lm = 218.3164 + T * 481267.8812 - t2 * 0.001599;
  Lm = _limitDeg360(Lm);
  double Ms = 357.5291 + T * 35999.0503 - t2 * 0.000154;
  Ms = _limitDeg360(Ms);
  double Mm = 134.9634 + T * 477198.8675 + t2 * 0.008721;
  Mm = _limitDeg360(Mm);
  double omega = 125.0443 - T * 1934.1363 + t2 * 0.008721;
  omega = _limitDeg360(omega);

  final dNutation = -(17.1996 + 0.01742 * T) * _sinD(omega) -
      (1.3187 + 0.00016 * T) * _sinD(2 * Ls) -
      0.2274 * _sinD(2 * Lm) +
      0.2062 * _sinD(2 * omega) +
      (0.1426 - 0.00034 * T) * _sinD(Ms) +
      0.0712 * _sinD(Mm) -
      (0.0517 - 0.00012 * T) * _sinD(2 * Ls + Ms) -
      0.0386 * _sinD(2 * Lm - omega) -
      0.0301 * _sinD(2 * Lm + Mm) +
      0.0217 * _sinD(2 * Ls - Ms) -
      0.0158 * _sinD(2 * Ls - 2 * Lm + Mm) +
      0.0129 * _sinD(2 * Ls - omega) +
      0.0123 * _sinD(2 * Lm - Mm);

  final dObliquity = (9.2025 + 0.00089 * T) * _cosD(omega) +
      (0.5736 - 0.00031 * T) * _cosD(2 * Ls) +
      0.0977 * _cosD(2 * Lm) -
      0.0895 * _cosD(2 * omega) +
      0.0224 * _cosD(2 * Ls + Ms) +
      0.0200 * _cosD(2 * Lm - omega) +
      0.0129 * _cosD(2 * Lm + Mm) -
      0.0095 * _cosD(2 * Ls - Ms) -
      0.0070 * _cosD(2 * Ls - omega);

  // eps0 in arcseconds: 23*3600 + 26*60 + 21.448
  const eps0Sec = 23 * 3600 + 26 * 60 + 21.448;
  final eps0 = eps0Sec - T * 46.8150 - t2 * 0.00059 + t3 * 0.001813;
  final obliquitySec = eps0 + dObliquity;
  final obliquityDeg = obliquitySec / 3600.0;

  return (obliquityDeg, dNutation, dObliquity);
}

// Precession: deltaRA is in seconds of time → hours
const _secTimeToHours = 1.0 / 3600.0;
// Arcseconds to decimal degrees
const _arcsecToDeg = 1.0 / 3600.0;
// Arcseconds to hours (for RA): arcsec/3600 = deg, deg/15 = hours
const _arcsecToHours = 1.0 / 54000.0;

/// Converts equatorial coordinates from a given equinox (e.g. J2000) to JNow (apparent)
/// for the given Julian Date. Matches Ephemeris::equatorialEquinoxToEquatorialJNowAtDateForT.
///
/// [raHours] RA in hours (equinox of [equinox]).
/// [decDeg] Dec in degrees (equinox of [equinox]).
/// [equinox] Epoch year (e.g. 2000 for J2000).
/// [jd] Julian Date (UTC) for the moment of observation.
///
/// Returns (raJNowHours, decJNowDeg).
(double raJNowHours, double decJNowDeg) equatorialEquinoxToJNow(
  double raHours,
  double decDeg,
  int equinox,
  double jd,
) {
  final T = _tFromJd(jd);
  final year = 2000 + T * 100; // approximate calendar year for (year - equinox)
  final yearDiff = (year - equinox).round();

  double ra = raHours;
  double dec = decDeg;

  // ----- Precession (same formulas as Ephemeris #if 1 block) -----
  final radDeg = ra * 15.0; // RA in degrees
  final Teq = equinox - 2000;
  final m = 3.07496 + 0.00186 * Teq;
  final nRA = 1.33621 - 0.00057 * Teq;
  final nDec = 20.0431 - 0.0085 * Teq;

  double deltaRA = m + nRA * _sinD(radDeg) * _tanD(dec);
  double deltaDec = nDec * _cosD(radDeg);

  ra += (deltaRA * yearDiff) * _secTimeToHours;
  dec += (deltaDec * yearDiff) * _arcsecToDeg;

  // ----- Nutation -----
  final (eps, deltaPhi, deltaEps) = _obliquityAndNutationForT(T);
  final radDeg2 = ra * 15.0;

  double deltaNutationRA = ( _cosD(eps) + _sinD(eps) * _sinD(radDeg2) * _tanD(dec)) * deltaPhi -
      (_cosD(radDeg2) * _tanD(dec) * deltaEps);
  deltaNutationRA = deltaNutationRA * _arcsecToHours;
  double deltaNutationDec = (_sinD(eps) * _cosD(radDeg2)) * deltaPhi + _sinD(radDeg2) * deltaEps;
  deltaNutationDec = deltaNutationDec * _arcsecToDeg;

  ra += deltaNutationRA;
  dec += deltaNutationDec;

  // ----- Aberration -----
  final t2 = T * T;
  double L0 = 280.46646 + T * 36000.76983 + t2 * 0.0003032;
  L0 = _limitDeg360(L0);
  double M = 357.5291092 + T * 35999.0502909 - t2 * 0.0001536;
  M = _limitDeg360(M);
  final e = 0.016708634 - T * 0.000042037 - t2 * 0.0000001267;
  double pi = 102.93735 + T * 1.71946 + t2 * 0.00046;

  final C = (1.914602 - T * 0.004817 - t2 * 0.000014) * _sinD(M) +
      (0.019993 - T * 0.000101) * _sinD(2 * M) +
      0.000289 * _sinD(3 * M);
  final O = L0 + C;

  const K = 20.49552;
  final radDeg3 = ra * 15.0;

  double deltaAberrationRA = -K * (_cosD(radDeg3) * _cosD(O) * _cosD(eps) + _sinD(radDeg3) * _sinD(O)) / _cosD(dec) +
      K * e * (_cosD(radDeg3) * _cosD(pi) * _cosD(eps) + _sinD(radDeg3) * _sinD(pi)) / _cosD(dec);
  deltaAberrationRA = deltaAberrationRA * _arcsecToHours;

  double deltaAberrationDec = -K * (_cosD(O) * _cosD(eps) * (_tanD(eps) * _cosD(dec) - _sinD(radDeg3) * _sinD(dec)) +
          _cosD(radDeg3) * _sinD(dec) * _sinD(O)) +
      K * e * (_cosD(pi) * _cosD(eps) * (_tanD(eps) * _cosD(dec) - _sinD(radDeg3) * _sinD(dec)) +
          _cosD(radDeg3) * _sinD(dec) * _sinD(pi));
  deltaAberrationDec = deltaAberrationDec * _arcsecToDeg;

  ra += deltaAberrationRA;
  dec += deltaAberrationDec;

  // ----- Avoid overflow (pole wrap) -----
  if (dec > 90) {
    dec = 180 - dec;
    ra += 12;
  } else if (dec < -90) {
    dec = -180 - dec;
    ra += 12;
  }

  ra = _limitHours24(ra);
  return (ra, dec);
}
