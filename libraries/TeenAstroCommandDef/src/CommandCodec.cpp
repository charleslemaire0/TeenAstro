/*
 * CommandCodec.cpp - Encoding / decoding helpers for TeenAstro LX200 protocol
 *
 * Unified implementation: this is the single source of truth for
 * value <-> string conversions used by both client and server.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include "CommandCodec.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ---------------------------------------------------------------------------
//  Internal helper -- integer parsing (static to avoid linker clash with
//  the copy in TeenAstroMath)
// ---------------------------------------------------------------------------
static bool atoi2(char* a, int* i)
{
  char* conv_end;
  long l = strtol(a, &conv_end, 10);
  if ((l < -32767) || (l > 32768) || (&a[0] == conv_end))
    return false;
  *i = (int)l;
  return true;
}

// ---------------------------------------------------------------------------
//  Response string parsers
// ---------------------------------------------------------------------------
void char2RA(char* txt, unsigned int& hour, unsigned int& minute, unsigned int& second)
{
  char* pEnd;
  hour   = (unsigned int)strtol(&txt[0], &pEnd, 10);
  minute = (unsigned int)strtol(&txt[3], &pEnd, 10);
  second = (unsigned int)strtol(&txt[6], &pEnd, 10);
}

void char2AZ(char* txt, unsigned int& deg, unsigned int& min, unsigned int& sec)
{
  char* pEnd;
  deg = (unsigned int)strtol(&txt[0], &pEnd, 10);
  min = (unsigned int)strtol(&txt[4], &pEnd, 10);
  sec = (unsigned int)strtol(&txt[6], &pEnd, 10);
}

void char2DEC(char* txt, int& deg, unsigned int& min, unsigned int& sec)
{
  char* pEnd;
  deg = (int)strtol(&txt[0], &pEnd, 10);
  min = (unsigned int)strtol(&txt[4], &pEnd, 10);
  sec = (unsigned int)strtol(&txt[7], &pEnd, 10);
}

// ---------------------------------------------------------------------------
//  String-to-component: "HH:MM:SS" or "HH:MM.M" -> int components
// ---------------------------------------------------------------------------
bool hmsToHms(int* h1, int* m1, int* m2, int* s1, char* hms, bool highPrecision)
{
  char h[3], m[5], s[3];
  *h1 = -1;
  *m1 = 0;
  *m2 = 0;
  *s1 = 0;

  while (*hms == ' ') hms++;  // strip prefix white-space

  if (highPrecision)
  {
    if (strlen(hms) != 8) return false;
  }
  else if (strlen(hms) != 7)
    return false;

  h[0] = *hms++;
  h[1] = *hms++;
  h[2] = 0;
  atoi2(h, h1);

  if (highPrecision)
  {
    if (*hms++ != ':') return false;
    m[0] = *hms++;
    m[1] = *hms++;
    m[2] = 0;
    atoi2(m, m1);
    if (*hms++ != ':') return false;
    s[0] = *hms++;
    s[1] = *hms++;
    s[2] = 0;
    atoi2(s, s1);
  }
  else
  {
    if (*hms++ != ':') return false;
    m[0] = *hms++;
    m[1] = *hms++;
    m[2] = 0;
    atoi2(m, m1);
    if (*hms++ != '.') return false;
    *m2 = (*hms++) - '0';
  }

  if ((*h1 < 0) || (*h1 > 23) || (*m1 < 0) || (*m1 > 59) ||
      (*m2 < 0) || (*m2 > 599) || (*s1 < 0) || (*s1 > 59))
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  String-to-double: "HH:MM:SS" or "HH:MM.M" -> fractional hours
// ---------------------------------------------------------------------------
bool hmsToDouble(double* f, char* hms, bool highPrecision)
{
  int h1, m1, m2, s1;
  if (hmsToHms(&h1, &m1, &m2, &s1, hms, highPrecision))
  {
    *f = h1 + m1 / 60.0 + m2 / 600.0 + s1 / 3600.0;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  String-to-double: DMS string -> fractional degrees
//  Accepts separators: ':', '*', degree-symbol (223), '\'' (tick)
//  Formats:  sDD:MM:SS  DDD:MM:SS  sDD*MM'SS  sDD:MM  DDD:MM  sDD*MM  DDD*MM
// ---------------------------------------------------------------------------
bool dmsToDouble(double* f, char* dms, bool sign_present, bool highPrecision)
{
  char d[4], m[5], s[3];
  int  d1, m1, s1 = 0;
  int  lowLimit = 0, highLimit = 360;
  int  checkLen, checkLen1;
  double sign = 1.0;
  bool secondsOff = false;

  while (*dms == ' ') dms++;  // strip prefix white-space
  checkLen1 = (int)strlen(dms);

  // determine if the seconds field was used and accept it if so
  if (highPrecision)
  {
    checkLen = 9;
    if (checkLen1 != checkLen) return false;
  }
  else
  {
    checkLen = 6;
    if (checkLen1 != checkLen)
    {
      if (checkLen1 == 9)
      {
        secondsOff = false;
        checkLen = 9;
      }
      else return false;
    }
    else secondsOff = true;
  }

  // determine if the sign was used and accept it if so
  if (sign_present)
  {
    if (*dms == '-')      sign = -1.0;
    else if (*dms == '+') sign =  1.0;
    else return false;
    dms++;
    d[0] = *dms++; d[1] = *dms++; d[2] = 0;
    if (!atoi2(d, &d1)) return false;
  }
  else
  {
    d[0] = *dms++; d[1] = *dms++; d[2] = *dms++; d[3] = 0;
    if (!atoi2(d, &d1)) return false;
  }

  // make sure the separator is an allowed character
  if ((*dms != ':') && (*dms != '*') && (*dms != (char)223))
    return false;
  else
    dms++;

  m[0] = *dms++; m[1] = *dms++; m[2] = 0;
  if (!atoi2(m, &m1)) return false;

  if ((highPrecision) && (!secondsOff))
  {
    // make sure the separator is an allowed character
    if ((*dms != ':') && (*dms != '\''))
      return false;
    else
      dms++;
    s[0] = *dms++; s[1] = *dms++; s[2] = 0;
    atoi2(s, &s1);
  }

  if (sign_present)
  {
    lowLimit  = -90;
    highLimit =  90;
  }

  if ((d1 < lowLimit) || (d1 > highLimit) ||
      (m1 < 0) || (m1 > 59) ||
      (s1 < 0) || (s1 > 59))
    return false;

  *f = sign * (d1 + m1 / 60.0 + s1 / 3600.0);
  return true;
}

// ---------------------------------------------------------------------------
//  Double-to-string: fractional hours -> "[-]HH:MM:SS" / "[-]HH:MM.M"
// ---------------------------------------------------------------------------
bool doubleToHms(char* reply, double* f, bool highPrecision)
{
  double h1, m1, f1, s1;

  f1 = fabs(*f) + 0.000139;   // round to 1/2 arc-sec
  h1 = floor(f1);
  m1 = (f1 - h1) * 60;
  s1 = (m1 - floor(m1));

  char fmt[] = "%s%02d:%02d:%02d";
  if (highPrecision)
  {
    s1 = s1 * 60.0;
  }
  else
  {
    s1 = s1 * 10.0;
    fmt[11] = '.';
    fmt[14] = '1';
  }

  char sign[2] = "";
  if (((s1 != 0) || (m1 != 0) || (h1 != 0)) && (*f < 0.0))
    strcpy(sign, "-");

  sprintf(reply, fmt, sign, (int)h1, (int)m1, (int)s1);
  return true;
}

// ---------------------------------------------------------------------------
//  Double-to-string: fractional degrees -> "[+/-]DD*MM:SS" / "DDD*MM:SS"
// ---------------------------------------------------------------------------
bool doubleToDms(char* reply, const double* f, bool fullRange, bool signPresent, bool highPrecision)
{
  char sign[] = "+";
  int  o = 0, d1, s1 = 0;
  double m1, f1;
  f1 = *f;

  // setup formatting, handle adding the sign
  if (f1 < 0)
  {
    f1 = -f1;
    sign[0] = '-';
  }

  f1 = f1 + 0.000139;         // round to 1/2 arc-second
  d1 = (int)floor(f1);
  m1 = (f1 - d1) * 60.0;
  s1 = (int)((m1 - floor(m1)) * 60.0);

  char fmt[] = "+%02d*%02d:%02d";
  if (signPresent)
  {
    if (sign[0] == '-')
      fmt[0] = '-';
    o = 1;
  }
  else
  {
    strcpy(fmt, "%02d*%02d:%02d");
  }

  if (fullRange) fmt[2 + o] = '3';

  if (highPrecision)
  {
    sprintf(reply, fmt, d1, (int)m1, s1);
  }
  else
  {
    fmt[9 + o] = 0;
    sprintf(reply, fmt, d1, (int)m1);
  }

  return true;
}

// ---------------------------------------------------------------------------
//  Date parsing: "MM/DD/YY" -> year (2000+), month, day
// ---------------------------------------------------------------------------
bool dateToYYYYMMDD(int* y1, int* m1, int* d1, char* date)
{
  char mm[3], dd[3], yy[3];
  if (strlen(date) != 8) return false;

  mm[0] = *date++; mm[1] = *date++; mm[2] = 0;
  atoi2(mm, m1);
  if (*date++ != '/') return false;

  dd[0] = *date++; dd[1] = *date++; dd[2] = 0;
  atoi2(dd, d1);
  if (*date++ != '/') return false;

  yy[0] = *date++; yy[1] = *date++; yy[2] = 0;
  atoi2(yy, y1);

  if ((*m1 < 1) || (*m1 > 12) || (*d1 < 1) || (*d1 > 31) ||
      (*y1 < 0) || (*y1 > 99))
    return false;

  if (*y1 > 11)
    *y1 = *y1 + 2000;
  else
    *y1 = *y1 + 2100;

  return true;
}
