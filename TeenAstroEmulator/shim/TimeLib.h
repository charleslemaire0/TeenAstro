/*
 * TimeLib.h - Arduino Time library compatibility for the MainUnit native build.
 *
 * Uses UTC semantics consistent with Paul Stoffregen's Time library
 * (see libraries/Time-master/Time.cpp: breakTime / makeTime).
 */
#pragma once
#include <cstdint>
#include <ctime>

#include "Teensy3Clock.h"

typedef struct {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;
  uint8_t Day;
  uint8_t Month;
  uint8_t Year; /* offset from 1970 */
} tmElements_t;

typedef time_t (*getExternalTime)();

#define tmYearToCalendar(Y) ((int)(Y) + 1970)
#define CalendarYrToTm(Y)   ((uint8_t)((Y) - 1970))

#define SECS_PER_MIN  ((time_t)(60UL))
#define SECS_PER_HOUR ((time_t)(3600UL))
#define SECS_PER_DAY  ((time_t)(SECS_PER_HOUR * 24UL))

// Arduino LEAP_YEAR: Y is years since 1970 (tmElements_t.Year).
#define LEAP_YEAR(Y) \
  (((1970 + (Y)) > 0) && !((1970 + (Y)) % 4) \
   && ((((1970 + (Y)) % 100)) || !(((1970 + (Y)) % 400))))

namespace {

static const uint8_t kMonthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

inline void breakTimeImpl(time_t timeInput, tmElements_t& tm)
{
  uint8_t              year;
  uint8_t              month, monthLength;
  uint32_t             time;
  unsigned long        days;

  time = (uint32_t)timeInput;
  tm.Second = (uint8_t)(time % 60);
  time /= 60;
  tm.Minute = (uint8_t)(time % 60);
  time /= 60;
  tm.Hour = (uint8_t)(time % 24);
  time /= 24;
  tm.Wday = (uint8_t)(((time + 4) % 7) + 1);

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366UL : 365UL)) <= time)
    year++;
  tm.Year = year;

  days -= LEAP_YEAR(year) ? 366UL : 365UL;
  time -= days;

  days       = 0;
  month      = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++)
  {
    if (month == 1)
      monthLength = LEAP_YEAR(year) ? (uint8_t)29 : (uint8_t)28;
    else
      monthLength = kMonthDays[month];

    if (time >= monthLength)
      time -= monthLength;
    else
      break;
  }
  tm.Month = (uint8_t)(month + 1);
  tm.Day   = (uint8_t)(time + 1);
}

inline time_t makeTimeImpl(const tmElements_t& tm)
{
  int      i;
  uint32_t seconds;

  seconds = (uint32_t)tm.Year * (uint32_t)SECS_PER_DAY * 365UL;
  for (i = 0; i < tm.Year; i++)
  {
    if (LEAP_YEAR((uint8_t)i))
      seconds += (uint32_t)SECS_PER_DAY;
  }

  for (i = 1; i < tm.Month; i++)
  {
    if ((i == 2) && LEAP_YEAR(tm.Year))
      seconds += (uint32_t)SECS_PER_DAY * 29U;
    else
      seconds += (uint32_t)SECS_PER_DAY * (uint32_t)kMonthDays[(unsigned)i - 1U];
  }

  seconds += (uint32_t)(tm.Day - 1) * (uint32_t)SECS_PER_DAY;
  seconds += (uint32_t)tm.Hour * (uint32_t)SECS_PER_HOUR;
  seconds += (uint32_t)tm.Minute * (uint32_t)SECS_PER_MIN;
  seconds += (uint32_t)tm.Second;
  return (time_t)seconds;
}

} // namespace

inline void setTime(time_t t)
{
  Teensy3Clock.set((unsigned long)t);
}

inline void setTime(int hr, int mn, int sc, int dy, int mo, int yr)
{
  tmElements_t tm{};
  tm.Hour   = (uint8_t)hr;
  tm.Minute = (uint8_t)mn;
  tm.Second = (uint8_t)sc;
  tm.Day    = (uint8_t)dy;
  tm.Month  = (uint8_t)mo;
  tm.Year   = CalendarYrToTm(yr);
  tm.Wday   = 0;
  setTime(makeTimeImpl(tm));
}

inline time_t now()
{
  return (time_t)Teensy3Clock.get();
}

inline int hour(time_t t)
{
  tmElements_t tm;
  breakTimeImpl(t, tm);
  return tm.Hour;
}
inline int minute(time_t t)
{
  tmElements_t tm;
  breakTimeImpl(t, tm);
  return tm.Minute;
}
inline int second(time_t t)
{
  tmElements_t tm;
  breakTimeImpl(t, tm);
  return tm.Second;
}
inline int day(time_t t)
{
  tmElements_t tm;
  breakTimeImpl(t, tm);
  return tm.Day;
}
inline int month(time_t t)
{
  tmElements_t tm;
  breakTimeImpl(t, tm);
  return tm.Month;
}
inline int year(time_t t)
{
  tmElements_t tm;
  breakTimeImpl(t, tm);
  return tmYearToCalendar(tm.Year);
}

inline int hour()
{
  return hour(now());
}
inline int minute()
{
  return minute(now());
}
inline int second()
{
  return second(now());
}
inline int day()
{
  return day(now());
}
inline int month()
{
  return month(now());
}
inline int year()
{
  return year(now());
}

inline time_t makeTime(const tmElements_t& tm)
{
  return makeTimeImpl(tm);
}

inline time_t makeTime(tmElements_t& tm)
{
  return makeTimeImpl(tm);
}

inline void breakTime(time_t t, tmElements_t& tm)
{
  breakTimeImpl(t, tm);
}

inline void setSyncProvider(getExternalTime)
{
}

inline void setSyncInterval(time_t)
{
}

inline int timeStatus()
{
  return 2;
}
