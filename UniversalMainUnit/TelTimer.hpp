#pragma once
#ifdef __arm__
#include <Arduino.h>
#endif
#include <TimeLib.h>
#include <TeenAstroMath.h>
time_t getFreeRTOSTimer(void);


class DateTimeTimers
{
public:
  volatile long           m_lst = 0;                    // this is the local (apparent) sidereal time in 1/100 seconds (23h 56m 4.1s per day = 86400 clock seconds/
private:

  //timers members
  unsigned long           m_clockTimer = 0;                 // wall time base, one second counter
  long                    m_siderealTimer = 0;          // counter to issue steps during tracking
  long                    m_guideSiderealTimer = 0;     // counter to issue steps during guiding
//tmp variable
  time_t                  m_RTClock;
  double                  m_UT = 0.0;//tmp variable  
  double                  m_LT = 0.0;//tmp variable  
  double                  m_JD = 0.0;//tmp variable                  
  double                  m_Sitelongitude;
public:

  double* getUT()
  {
    m_RTClock = getTimeStamp();
    m_UT = hour(m_RTClock) + minute(m_RTClock) / 60.0 + second(m_RTClock) / 3600.0;
    return &m_UT;
  }

  long GetDeltaUTC(const int& y,const  int& m,const  int& d,const int& h,const int& mi,const int& s)
  {
    tmElements_t t1;
    t1.Year = y - 1970;
    t1.Month = m;
    t1.Day = d;
    t1.Hour = h;
    t1.Minute = mi;
    t1.Second = s;
    time_t delta = makeTime(t1);
    m_RTClock = getTime();
    return  m_RTClock - delta;
  }

  double* getLT(const float*toff)
  {
    m_RTClock = getTime();
    m_RTClock -= (long)(*toff * 3600.0);
    m_LT = hour(m_RTClock) + minute(m_RTClock) / 60.0 + second(m_RTClock) / 3600.0;
    return &m_LT;
  }

  double* getJD()
  {
    m_RTClock = getTime();
    m_JD = julian(year(m_RTClock), month(m_RTClock), day(m_RTClock));
    return &m_JD;
  }

  void getUTDate(int& y, int& m, int& d, int&h, int&mi, int&s)
  {
    m_RTClock = getTime();
    y = year(m_RTClock);
    m = month(m_RTClock);
    d = day(m_RTClock);
    h = hour(m_RTClock);
    mi = minute(m_RTClock);
    s = second(m_RTClock);
  }

  void getULDate(int& y, int& m, int& d, int&h, int&mi, int&s, const float* toff)
  {
    m_RTClock = getTimeStamp();
    m_RTClock -= (long)(*toff * 3600.0);
    y = year(m_RTClock);
    m = month(m_RTClock);
    d = day(m_RTClock);
    h = hour(m_RTClock);
    mi = minute(m_RTClock);
    s = second(m_RTClock);
  }

  unsigned long getTimeStamp()
  {
    return getTime();
  }
  static time_t getTime()
  {
    return getFreeRTOSTimer();
  }
  void SetFromTimeStamp(unsigned long t)
  {
//    Teensy3Clock.set(t);
    setTime(t);
    syncClock();
  }

  // Set RTC with give input
  void setClock(int y1, int m1, int d1, int h1, int mi1, int s1, double Sitelongitude, float Sitetoff)
  {
    tmElements_t t1;
    t1.Year = y1 - 1970;
    t1.Month = m1;
    t1.Day = d1;
    t1.Hour = h1;
    t1.Minute = mi1;
    t1.Second = s1;
    m_Sitelongitude = Sitelongitude;
    time_t t = makeTime(t1);
    t += (long)(Sitetoff * 3600.0);
//    Teensy3Clock.set(t);
    setTime(t);
    syncClock();
  }

  void resetLongitude(double Sitelongitude)
  {
    m_Sitelongitude = Sitelongitude;
    syncClock();
  }

  // convert the lst (in 1/100 second units) into floating point hours
  double LST()
  {
    cli();
    long    tempLst = m_lst;
    sei();
    while (tempLst > 8640000) tempLst -= 8640000;
    return (tempLst / 8640000.0) * 24.0;
  }

  //TIMERS
  bool updateclockTimer(unsigned long m)
  {
    if (m - m_clockTimer < 1000UL)
    {
      return false;
    }
    m_clockTimer = m;
    return true;
  }
  bool updatesiderealTimer()
  {
    cli();
    long    tempLst = m_lst;
    sei();
    if (tempLst != m_siderealTimer)
    {
      m_siderealTimer = tempLst;
      return true;
    }
    return false;
  }
  bool updateguideSiderealTimer()
  {
    cli();
    long    tempLst = m_lst;
    sei();
    if (tempLst != m_guideSiderealTimer)
    {
      m_guideSiderealTimer = tempLst;
      return true;
    }
    return false;
  }
  void updateTimers()
  {
    cli();
    m_siderealTimer = m_lst;
    m_guideSiderealTimer = m_lst;
    sei();
    m_clockTimer = millis();
  }

  //End TIMERS
private:
  double timeRange(double t)
  {
    while (t >= 24.0) t -= 24.0;
    while (t < 0.0) t += 24.0;
    return t;
  }
  double julian(int Year, int Month, int Day)
  {
    if ((Month == 1) || (Month == 2))
    {
      Year--;
      Month = Month + 12;
    }

    double  B = 2.0 - floor(Year / 100.0) + floor(Year / 400.0);
    return
      (
        B +
        floor(365.25 * Year) +
        floor(30.6001 * (Month + 1.0)) +
        Day +
        1720994.5
        );  //+(Time/24.0);
  }
  void greg(double JulianDay, int *Year, int *Month, int *Day)
  {
    double  A,
      B,
      C,
      D,
      D1,
      E,
      F,
      G,
      I;

    JulianDay = JulianDay + 0.5;
    I = floor(JulianDay);

    F = 0.0;    //  JD-I;
    if (I > 2299160.0)
    {
      A = int((I - 1867216.25) / 36524.25);
      B = I + 1.0 + A - floor(A / 4.0);
    }
    else
      B = I;

    C = B + 1524.0;
    D = floor((C - 122.1) / 365.25);
    E = floor(365.25 * D);
    G = floor((C - E) / 30.6001);

    D1 = C - E + F - floor(30.6001 * G);
    *Day = floor(D1);
    if (G < 13.5)
      *Month = floor(G - 1.0);
    else
      *Month = floor(G - 13.0);
    if (*Month > 2.5)
      *Year = floor(D - 4716.0);
    else
      *Year = floor(D - 4715.0);
  }
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
  bool dateToJD(double *JulianDay, char *date)
  {
    int     m1, d1, y1;
    if (dateToYYYYMMDD(&y1, &m1, &d1, date))
    {
      *JulianDay = julian(y1, m1, d1);
      return true;
    }
    return false;
  }


  void syncClock()
  {
    update_lst(last());
  }

  // passes Local Apparent Sidereal Time to stepper timer
  void update_lst(double t)
  {
    long    lst1 = (t / 24.0) * 8640000.0;

    // set the local sidereal time
    cli();
    m_lst = lst1;
    sei();
  }

  // -----------------------------------------------------------------------------------------------------------------------------
  // Date Time conversion

  // converts Gregorian date (Y,M,D) to Julian day number


  // convert date/time to Greenwich Apparent Sidereal time
  double green_ast()
  {
    unsigned long RTClock = getTimeStamp();
    double JulianDay = julian(year(RTClock), month(RTClock), day(RTClock));
    double ut1 = hour(RTClock) + minute(RTClock) / 60.0 + second(RTClock) / 3600.0;
    int y,
      m,
      d;
    greg(JulianDay, &y, &m, &d);

    double  JulianDay0 = julian(y, m, d);
    double  D = (JulianDay - 2451545.0) + (ut1 / 24.0);
    double  D0 = (JulianDay0 - 2451545.0);
    double  H = ut1;
    double  T = D / 36525.0;
    double  gmst = 6.697374558 + 0.06570982441908 * D0;
    gmst = timeRange(gmst);
    gmst = gmst + 1.00273790935 * H + 0.000026 * T * T;
    gmst = timeRange(gmst);

    // equation of the equinoxes
    double  O = 125.04 - 0.052954 * D;
    double  L = 280.47 + 0.98565 * D;
    double  E = 23.4393 - 0.0000004 * D;
    double  W = -0.000319 * sin(O / Rad) - 0.000024 * sin((2 * L) / Rad);
    double  eqeq = W * cos(E / Rad);
    double  gast = gmst + eqeq;
    return timeRange(gast);
  }

  // convert date/time to Local Apparent Sidereal Time

  // uses longitude
  double last()
  {
    // JulianDay is the Local date, jd2gast requires a universal time
    // this is a hack that leaves the date alone and lets the UT1 cover
    // the difference in time to the next (or previous) day
    double  gast = green_ast();
    return timeRange(gast - (m_Sitelongitude / 15.0));
  }
  // converts Julian day number to Gregorian date (Y,M,D)


};


