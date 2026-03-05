/*
 * TinyGPS++.h - Stub for TinyGPS++ library (native tests).
 */
#pragma once

struct TinyGPSLocation {
    double lat() const { return 0.0; }
    double lng() const { return 0.0; }
    bool isValid() const { return false; }
    bool isUpdated() const { return false; }
};

struct TinyGPSDate {
    uint16_t year() const { return 2026; }
    uint8_t month() const { return 3; }
    uint8_t day() const { return 5; }
    bool isValid() const { return false; }
    bool isUpdated() const { return false; }
};

struct TinyGPSTime {
    uint8_t hour() const { return 12; }
    uint8_t minute() const { return 0; }
    uint8_t second() const { return 0; }
    uint8_t centisecond() const { return 0; }
    bool isValid() const { return false; }
    bool isUpdated() const { return false; }
};

struct TinyGPSHDOP {
    double hdop() const { return 99.0; }
    bool isValid() const { return false; }
    bool isUpdated() const { return false; }
};

struct TinyGPSAltitude {
    double meters() const { return 0.0; }
    bool isValid() const { return false; }
};

class TinyGPSPlus {
public:
    TinyGPSLocation location;
    TinyGPSDate date;
    TinyGPSTime time;
    TinyGPSHDOP hdop;
    TinyGPSAltitude altitude;
    bool encode(char) { return false; }
    uint32_t charsProcessed() const { return 0; }
};
