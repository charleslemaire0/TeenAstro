/*
 * CommandCodec.h - Encoding / decoding helpers for TeenAstro LX200 protocol
 *
 * Shared parsing and formatting utilities used by both the client
 * (LX200Client) and the server (MainUnit).  This is the single source
 * of truth for value ↔ string conversion on the LX200 wire format.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
//  Response string parsers (LX200 reply -> components)
// ---------------------------------------------------------------------------

/// Parse an RA reply string "HH:MM:SS" into components.
void char2RA(char* txt, unsigned int& hour, unsigned int& minute, unsigned int& second);

/// Parse an azimuth reply string "DDD:MM:SS" into components.
void char2AZ(char* txt, unsigned int& deg, unsigned int& min, unsigned int& sec);

/// Parse a declination reply string "+DD:MM:SS" into components.
void char2DEC(char* txt, int& deg, unsigned int& min, unsigned int& sec);

// ---------------------------------------------------------------------------
//  String-to-double conversions
// ---------------------------------------------------------------------------

/// Convert "HH:MM:SS" (high precision) or "HH:MM.M" (low precision) to
/// fractional hours.
bool hmsToDouble(double* f, char* hms, bool highPrecision = true);

/// Convert "sDD:MM:SS" / "DDD:MM:SS" / "sDD*MM'SS" / "DDD*MM" to
/// fractional degrees.  Accepts ':', '*', degree-symbol, and '\'' as
/// separators.
bool dmsToDouble(double* f, char* dms, bool sign_present, bool highPrecision);

// ---------------------------------------------------------------------------
//  String-to-component conversions
// ---------------------------------------------------------------------------

/// Parse "HH:MM:SS" or "HH:MM.M" into separate int components.
bool hmsToHms(int* h1, int* m1, int* m2, int* s1, char* hms, bool highPrecision = true);

// ---------------------------------------------------------------------------
//  Double-to-string conversions (encoding)
// ---------------------------------------------------------------------------

/// Format fractional hours as "[-]HH:MM:SS" (high precision) or
/// "[-]HH:MM.M" (low precision).
bool doubleToHms(char* reply, double* f, bool highPrecision = true);

/// Format fractional degrees as "[+/-]DD*MM:SS" or "[+/-]DDD*MM:SS".
/// @param fullRange  true → 3-digit degrees (DDD), false → 2-digit (DD)
/// @param signPresent true → prepend +/-, false → no sign
bool doubleToDms(char* reply, const double* f, bool fullRange, bool signPresent, bool highPrecision = true);

// ---------------------------------------------------------------------------
//  Date parsing
// ---------------------------------------------------------------------------

/// Parse "MM/DD/YY" date string into year (2000+), month, day.
bool dateToYYYYMMDD(int* y1, int* m1, int* d1, char* date);
