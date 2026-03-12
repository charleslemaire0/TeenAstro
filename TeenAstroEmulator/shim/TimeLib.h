/*
 * TimeLib.h - Stub for the Arduino Time library (native tests).
 */
#pragma once
#include <cstdint>
#include <ctime>

/* Do not redefine time_t; use the system's definition */

typedef struct {
    uint8_t Second;
    uint8_t Minute;
    uint8_t Hour;
    uint8_t Wday;
    uint8_t Day;
    uint8_t Month;
    uint8_t Year;
} tmElements_t;

typedef time_t (*getExternalTime)();

static time_t _sim_time = 1709600000UL;

inline void     setTime(time_t t) { _sim_time = t; }
inline void     setTime(int hr, int mn, int sc, int dy, int mo, int yr) {
    (void)hr; (void)mn; (void)sc; (void)dy; (void)mo; (void)yr;
}
inline time_t   now()       { return _sim_time; }
inline int      hour()      { return (int)((_sim_time % 86400UL) / 3600); }
inline int      minute()    { return (int)((_sim_time % 3600) / 60); }
inline int      second()    { return (int)(_sim_time % 60); }
inline int      day()       { return 5; }
inline int      month()     { return 3; }
inline int      year()      { return 2026; }
inline int      hour(time_t t)   { return (int)((t % 86400UL) / 3600); }
inline int      minute(time_t t) { return (int)((t % 3600) / 60); }
inline int      second(time_t t) { return (int)(t % 60); }
inline int      day(time_t t)    { (void)t; return 5; }
inline int      month(time_t t)  { (void)t; return 3; }
inline int      year(time_t t)   { (void)t; return 2026; }

inline time_t   makeTime(tmElements_t& tm) {
    (void)tm; return _sim_time;
}
inline void     breakTime(time_t t, tmElements_t& tm) {
    (void)t;
    tm.Hour = hour(); tm.Minute = minute(); tm.Second = second();
    tm.Day = day(); tm.Month = month(); tm.Year = year() - 1970;
}
inline void     setSyncProvider(getExternalTime) {}
inline void     setSyncInterval(time_t) {}
inline int      timeStatus() { return 2; }
