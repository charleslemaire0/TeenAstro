/**
 * @file  TeenAstroMath.h
 * @brief Math utilities for the TeenAstro telescope controller.
 *
 * Provides:
 *   - Angle unit conversion constants
 *   - Coordinate formatting (HMS / DMS)
 *   - String-to-integer parsing with range checking
 *   - Angle normalisation helpers
 *   - Angular distance
 *   - Atmospheric refraction  [DEPRECATED -- use LA3::Topocentric2Apparent]
 *   - Equatorial / Horizontal coordinate conversion  [LEGACY -- use TeenAstroCoord]
 *   - Step-distance helpers for mount axes
 *
 * This library merges the former TeenAstroFunction into TeenAstroMath so
 * that all math / formatting utilities live in a single place.
 *
 * @author  Charles Lemaire
 */

#pragma once
#ifndef __TeenAstroMath_h__
#define __TeenAstroMath_h__

#include <cstdint>

/* ======================================================================== */
/*  Constants                                                                */
/* ======================================================================== */

/** @name Unit-conversion constants */
/** @{ */
#ifndef DEG_TO_RAD
#define DEG_TO_RAD  0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG  57.295779513082320876798154814105
#endif
#ifndef HOUR_TO_RAD
#define HOUR_TO_RAD 0.26179938779914943653855361527329
#endif
#ifndef RAD_TO_HOUR
#define RAD_TO_HOUR 3.8197186342054880584532103209403
#endif
/** @} */

/**
 * Legacy alias for RAD_TO_DEG.
 * Kept for backward compatibility with Site.hpp, TelTimer.hpp, and TeenAstroCatalog.
 */
#define Rad 57.29577951308232

/* ======================================================================== */
/*  Types                                                                    */
/* ======================================================================== */

/** Side of the pole an axis is on (used for pier-side logic). */
enum PoleSide
{
  POLE_NOTVALID,  /**< Not yet determined. */
  POLE_UNDER,     /**< Normal (under-pole) side. */
  POLE_OVER       /**< Flipped (over-pole / meridian-flipped) side. */
};

/* ======================================================================== */
/*  Coordinate formatting  (merged from TeenAstroFunction)                   */
/* ======================================================================== */

/**
 * Split total arc-seconds into hours, minutes, seconds.
 *
 * @param[in]  v   Total arc-seconds (non-negative expected).
 * @param[out] h   Hours   (v / 3600).
 * @param[out] m   Minutes ((v / 60) % 60).
 * @param[out] s   Seconds (v % 60).
 */
void gethms(const long& v, uint8_t& h, uint8_t& m, uint8_t& s);

/**
 * Split a signed arc-second value into sign, degrees, arc-minutes, arc-seconds.
 *
 * @param[in]  v      Signed total arc-seconds.
 * @param[out] ispos  True if v >= 0.
 * @param[out] deg    Degrees  (|v| / 3600).
 * @param[out] min    Arc-minutes ((|v| / 60) % 60).
 * @param[out] sec    Arc-seconds (|v| % 60).
 */
void getdms(const long& v, bool& ispos, uint16_t& deg, uint8_t& min, uint8_t& sec);

/* ======================================================================== */
/*  String-to-integer parsing                                                */
/* ======================================================================== */

/**
 * Parse a decimal string to a signed 16-bit integer with bounds checking.
 *
 * @param[in]  a  Null-terminated decimal string.
 * @param[out] i  Parsed value (unchanged on failure).
 * @return True if the string was a valid integer in [INT16_MIN, INT16_MAX].
 */
bool atoi2(char* a, int* i);

/**
 * Parse a decimal string to an unsigned 16-bit integer with bounds checking.
 *
 * @param[in]  a  Null-terminated decimal string.
 * @param[out] i  Parsed value (unchanged on failure).
 * @return True if the string was a valid integer in [0, UINT16_MAX].
 */
bool atoui2(char* a, unsigned int* i);

/* ======================================================================== */
/*  Basic math helpers                                                       */
/* ======================================================================== */

/** Fractional part of @p v (v - floor(v)). */
double frac(double v);

/** Cotangent of @p n (1 / tan(n)). */
double cot(double n);

/* ======================================================================== */
/*  Angle normalisation                                                      */
/* ======================================================================== */

/** Normalise hour-angle to [-180, +180] degrees. */
double haRange(double d);

/** Normalise hour-angle to [-PI, +PI] radians. */
double haRangeRad(double d);

/** Normalise azimuth to [0, 360) degrees. */
double AzRange(double d);

/** Normalise arbitrary angle to [0, 360) degrees. */
double degRange(double d);

/* ======================================================================== */
/*  Angular distance                                                         */
/* ======================================================================== */

/**
 * Great-circle angular distance between two equatorial positions.
 *
 * All parameters and the return value are in **degrees**.
 *
 * @param h   Hour-angle of position 1.
 * @param d   Declination of position 1.
 * @param h1  Hour-angle of position 2.
 * @param d1  Declination of position 2.
 * @return    Angular separation in degrees.
 */
double angDist(double h, double d, double h1, double d1);

/* ======================================================================== */
/*  Atmospheric refraction  [DEPRECATED]                                     */
/*                                                                           */
/*  These degree-based functions are kept for UniversalMainUnit              */
/*  compatibility.  New code should use the radian-based Meeus formulas      */
/*  in TeenAstroLA3  (LA3::Topocentric2Apparent / LA3::Apparent2Topocentric).*/
/* ======================================================================== */

/**
 * [DEPRECATED] Refraction in arc-minutes at a given true (topocentric) altitude.
 *
 * @param Alt          True altitude in degrees.
 * @param Pressure     Atmospheric pressure in millibars (default 1010).
 * @param Temperature  Temperature in Celsius (default 10).
 * @return Refraction in arc-minutes (always >= 0).
 */
double trueRefrac(double Alt, double Pressure = 1010.0, double Temperature = 10.0);

/**
 * [DEPRECATED] Add refraction to a topocentric altitude (degrees).
 * Prefer LA3::Topocentric2Apparent (radian-based, Meeus Saemundsson).
 */
void Topocentric2Apparent(double* Alt, double Pressure = 1010.0, double Temperature = 10.0);

/**
 * [DEPRECATED] Remove refraction from an apparent altitude (degrees).
 * Prefer LA3::Apparent2Topocentric (radian-based, Meeus Bennett).
 */
void Apparent2Topocentric(double* Alt, double Pressure = 1010.0, double Temperature = 10.0);

/* ======================================================================== */
/*  Coordinate conversion  [LEGACY]                                          */
/*                                                                           */
/*  These degree-based free functions are kept for UniversalMainUnit.        */
/*  New code should use the TeenAstroCoord classes (Coord_EQ, Coord_HO).    */
/* ======================================================================== */

/**
 * [LEGACY] Equatorial -> Horizontal conversion (degrees).
 *
 * @param HA          Hour-angle in degrees.
 * @param Dec         Declination in degrees.
 * @param refraction  If true, applies Topocentric2Apparent to the result.
 * @param[out] Azm    Azimuth in degrees (0 = North, CW).
 * @param[out] Alt    Altitude in degrees.
 * @param cosLat      Pointer to cos(latitude).
 * @param sinLat      Pointer to sin(latitude).
 */
void EquToHor(double HA, double Dec, bool refraction,
              double* Azm, double* Alt,
              const double* cosLat, const double* sinLat);

/**
 * [LEGACY] Topocentric Horizontal -> Equatorial conversion (degrees).
 */
void HorTopoToEqu(double Azm, double Alt,
                   double* HA, double* Dec,
                   const double* cosLat, const double* sinLat);

/**
 * [LEGACY] Apparent Horizontal -> Equatorial conversion (degrees).
 * Removes refraction from Alt before converting.
 */
void HorAppToEqu(double Azm, double Alt,
                  double* HA, double* Dec,
                  const double* cosLat, const double* sinLat);

/* ======================================================================== */
/*  Step-distance helpers                                                    */
/* ======================================================================== */

/** Step difference for axis 1 (RA / Azimuth). */
long distStepAxis1(long* start, long* end);
/** @overload */
long distStepAxis1(volatile long* start, volatile long* end);
/** @overload */
long distStepAxis1(volatile long* start, volatile double* end);

/** Step difference for axis 2 (Dec / Altitude). */
long distStepAxis2(long* start, long* end);
/** @overload */
long distStepAxis2(volatile long* start, volatile long* end);
/** @overload */
long distStepAxis2(volatile long* start, volatile double* end);

#endif /* __TeenAstroMath_h__ */
