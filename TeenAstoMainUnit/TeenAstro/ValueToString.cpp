#include <Arduino.h>
#include <TeenAstroMath.h>
#include "ValueToString.h"

bool dateToYYYYMMDD(int *y1, int *m1, int *d1, char *date)
{
  char    m[3], d[3], y[3];
  if (strlen(date) != 8) return false;
  m[0] = *date++;
  m[1] = *date++;
  m[2] = 0;
  atoi2(m, m1);
  if (*date++ != '/') return false;
  d[0] = *date++;
  d[1] = *date++;
  d[2] = 0;
  atoi2(d, d1);
  if (*date++ != '/') return false;
  y[0] = *date++;
  y[1] = *date++;
  y[2] = 0;
  atoi2(y, y1);
  if ((*m1 < 1) || (*m1 > 12) || (*d1 < 1) || (*d1 > 31) || (*y1 < 0) || (*y1 > 99))
    return false;
  if (*y1 > 11)
    *y1 = *y1 + 2000;
  else
    *y1 = *y1 + 2100;
  return true;
}
bool hmsToDouble(double *f, char *hms, bool hP)
{
  int h1, m1, m2, s1;
  if (hmsToHms(&h1, &m1, &m2, &s1, hms, hP))
  {
    *f = h1 + m1 / 60.0 + m2 / 600.0 + s1 / 3600.0;
    return true;
  }
  return false;
}
bool hmsToHms(int *h1, int *m1, int *m2, int *s1, char*hms, bool hP)
{
  char    h[3], m[5], s[3];
  *h1 = -1;
  *m1 = 0;
  *m2 = 0;
  *s1 = 0;

  while (*hms == ' ') hms++;  // strip prefix white-space
  if (hP)
  {
    if (strlen(hms) != 8) return false;
  }
  else if (strlen(hms) != 7)
    return false;

  h[0] = *hms++;
  h[1] = *hms++;
  h[2] = 0;
  atoi2(h, h1);
  if (hP)
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

  if ((*h1 < 0) || (*h1 > 23) || (*m1 < 0) || (*m1 > 59) || (*m2 < 0) || (*m2 > 599) ||
    (*s1 < 0) || (*s1 > 59))
    return false;
  return true;
}
bool doubleToHms(char *reply, double *f, bool hP)
{
    double  h1,
            m1,
            f1,
            s1;

    f1 = fabs(*f) + 0.000139;   // round to 1/2 arc-sec
    h1 = floor(f1);
    m1 = (f1 - h1) * 60;
    s1 = (m1 - floor(m1));

    char    s[] = "%s%02d:%02d:%02d";
    if (hP)
    {
        s1 = s1 * 60.0;
    }
    else
    {
        s1 = s1 * 10.0;
        s[11] = '.';
        s[14] = '1';
    }

    char    sign[2] = "";
    if (((s1 != 0) || (m1 != 0) || (h1 != 0)) && (*f < 0.0))
        strcpy(sign, "-");
    sprintf(reply, s, sign, (int) h1, (int) m1, (int) s1);

    return true;
}
// convert string in format sDD:MM:SS to floating point
// (also handles)           DDD:MM:SS
//                          sDD*MM'SS
//                          sDD:MM
//                          DDD:MM
//                          sDD*MM
//                          DDD*MM
bool dmsToDouble(double *f, char *dms, boolean sign_present, bool hP)
{
    char    d[4],
            m[5],
            s[3];
    int     d1,
            m1,
            s1 = 0;
    int     lowLimit = 0,
            highLimit = 360;
    int     checkLen,
            checkLen1;
    double  sign = 1.0;
    boolean secondsOff = false;

    while (*dms == ' ') dms++;  // strip prefix white-space
    checkLen1 = strlen(dms);

    // determine if the seconds field was used and accept it if so
    if (hP)
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
            else
                return false;
        }
        else
            secondsOff = true;
    }

    // determine if the sign was used and accept it if so
    if (sign_present)
    {
        if (*dms == '-')
            sign = -1.0;
        else if (*dms == '+')
            sign = 1.0;
        else
            return false;
        dms++;
        d[0] = *dms++;
        d[1] = *dms++;
        d[2] = 0;
        if (!atoi2(d, &d1)) return false;
    }
    else
    {
        d[0] = *dms++;
        d[1] = *dms++;
        d[2] = *dms++;
        d[3] = 0;
        if (!atoi2(d, &d1)) return false;
    }

    // make sure the seperator is an allowed character
    if ((*dms != ':') && (*dms != '*') && (*dms != char(223)))
        return false;
    else
        dms++;

    m[0] = *dms++;
    m[1] = *dms++;
    m[2] = 0;
    if (!atoi2(m, &m1)) return false;
    if ((hP) && (!secondsOff))
    {
        // make sure the seperator is an allowed character
        if ((*dms != ':') && (*dms != '\''))
          return false;
        else
          dms++;
        s[0] = *dms++;
        s[1] = *dms++;
        s[2] = 0;
        atoi2(s, &s1);
    }

    if (sign_present)
    {
        lowLimit = -90;
        highLimit = 90;
    }

    if ((d1 < lowLimit) || (d1 > highLimit) || (m1 < 0) || (m1 > 59) || (s1 < 0) ||
        (s1 > 59))
        return false;

    *f = sign * (d1 + m1 / 60.0 + s1 / 3600.0);
    return true;
}
bool doubleToDms(char *reply, const double *f, bool fullRange, bool signPresent, bool hP)
{
    char    sign[] = "+";
    int     o = 0,
            d1,
            s1 = 0;
    double  m1,
            f1;
    f1 = *f;

    // setup formatting, handle adding the sign
    if (f1 < 0)
    {
        f1 = -f1;
        sign[0] = '-';
    }

    f1 = f1 + 0.000139;         // round to 1/2 arc-second
    d1 = floor(f1);
    m1 = (f1 - d1) * 60.0;
    s1 = (m1 - floor(m1)) * 60.0;

    char    s[] = "+%02d*%02d:%02d";
    if (signPresent)
    {
        if (sign[0] == '-')
        {
            s[0] = '-';
        }

        o = 1;
    }
    else
    {
        strcpy(s, "%02d*%02d:%02d");
    }

    if (fullRange) s[2 + o] = '3';

    if (hP)
    {
        sprintf(reply, s, d1, (int) m1, s1);
    }
    else
    {
        s[9 + o] = 0;
        sprintf(reply, s, d1, (int) m1);
    }

    return true;
}
