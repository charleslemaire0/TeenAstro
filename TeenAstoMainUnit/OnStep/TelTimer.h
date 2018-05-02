#pragma once
#include "Helper_math.h"

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
// converts Julian day number to Gregorian date (Y,M,D)
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
// passes Local Apparent Sidereal Time to stepper timer
// Misc. numeric conversion
double timeRange(double t)
{
  while (t >= 24.0) t -= 24.0;
  while (t < 0.0) t += 24.0;
  return t;
}

class GNSSPPS
{
private:
  volatile unsigned long  m_lastMicroS = 1000000UL;
  volatile unsigned long  m_avgMicroS = 1000000UL;
  volatile double         m_lastRateRatio = 1.0;

public:
  // PPS (GPS)
  volatile boolean           m_synced = false;
  volatile double         m_rateRatio = 1.0;
  void clockSync()
  {
    unsigned long   t = micros();
    unsigned long   oneS = (t - m_lastMicroS);
    if ((oneS > 1000000 - 1000) && (oneS < 1000000 + 1000))
    {
      m_avgMicroS = (m_avgMicroS * 19 + oneS) / 20;
      m_synced = true;
    }
    m_lastMicroS = t;
  }
  void clockUpdate()
  {
    m_rateRatio = ((double) 1000000.0 / (double)(m_avgMicroS));
    if ((long)(micros() - (m_lastMicroS + 2000000UL)) > 0)
      m_synced = false;          // if more than two seconds has ellapsed without a pulse we've lost sync
  }
  bool hasRatioChanged()
  {
    return (m_lastRateRatio != m_rateRatio && m_synced);
  }
  void updateLastRatio()
  {
    m_lastRateRatio = m_rateRatio;
  }
};

class timerLoop
{
private:
  long                    m_this_micros = 0;
  long                    m_time = 0;
  long                    m_last_micros = 0;       // workload monitoring
  long                    m_worst_time = 0;
public:
  void monitor()
  {
    m_this_micros = micros();
    m_time = m_this_micros - m_last_micros;
    if (m_time > m_worst_time) m_worst_time = m_time;
    m_last_micros = m_this_micros;
  }
  void update()
  {
    m_last_micros = micros();
  }
  void resetWorstTime()
  {
    m_worst_time = 0;
  }
  long getWorstTime()
  {
    return m_worst_time;
  }
};

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
  double                  m_JD = 0.0;//tmp variable                  
  double                  m_Sitelongitude;

 

public:

  double* getUT()
  {
    m_RTClock = Teensy3Clock.get();
    m_UT = hour(m_RTClock) + minute(m_RTClock) / 60.0 + second(m_RTClock) / 3600.0;
    return &m_UT;
  }
  double* getJD()
  {
    m_RTClock = Teensy3Clock.get();
    m_JD = julian(year(m_RTClock), month(m_RTClock), day(m_RTClock));
    return &m_JD;
  }
  void getUTDate(int& y, int& m, int& d)
  {
    m_RTClock = Teensy3Clock.get();
    y = year(m_RTClock);
    m = month(m_RTClock);
    d = day(m_RTClock);
  }
  // Set RTC with give input
  void setClock(int y1, int m1, int d1, int h1, int mi1, int s1, double Sitelongitude)
  {
    tmElements_t t1;
    t1.Year = y1 - 1970;
    t1.Month = m1;
    t1.Day = d1;
    t1.Hour = h1;
    t1.Minute = mi1;
    t1.Second = s1;
    Sitelongitude = m_Sitelongitude;
    time_t t = makeTime(t1);
    Teensy3Clock.set(t);
    setTime(t);
    syncClock();
  }
  // convert string in format MM/DD/YY to julian date
  bool resetDate(char *date)
  {
    int     m1, d1, y1;
    if (!dateToYYYYMMDD(&y1, &m1, &d1, date))
    {
      return false;
    }
    m_RTClock = Teensy3Clock.get();
    setClock(y1, m1, d1, hour(m_RTClock), minute(m_RTClock), second(m_RTClock), m_Sitelongitude);
    return true;
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
  boolean dateToYYYYMMDD(int *y1, int *m1, int *d1, char *date)
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
  boolean dateToJD(double *JulianDay, char *date)
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
    unsigned long RTClock = Teensy3Clock.get();
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


};


