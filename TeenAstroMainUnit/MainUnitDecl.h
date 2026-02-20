#pragma once
/**
 * Cross-module function declarations for TeenAstro Main Unit (C++ build).
 * Included from MainUnit.h. For command-specific or mount-specific APIs,
 * prefer including the owning header where possible.
 */

// Lifecycle
void reboot();

// Timer
speed interval2speed(interval i);
interval speed2interval(speed V, interval maxInterval);
void SetsiderealClockSpeed(double cs);
void beginTimers();

// Park / alignment
void saveAlignModel();

// EEPROM / mount init
void AutoinitEEPROM();
void writeDefaultMounts();
void writeDefaultMount();
void writeDefaultMountName(int i);
void initMount();
void initTransformation(bool reset);
void initCelestialPole();
void initmotor(bool deleteAlignment);
void ReadEEPROMEncoderMotorMode();
void WriteEEPROMEncoderMotorMode();
void initencoder();
void readEEPROMmotorCurrent();
void readEEPROMmotor();
void writeDefaultEEPROMmotor();
void readEEPROMencoder();
void writeDefaultEEPROMencoder();

// Command reply helpers
void replyShortTrue();
void replyLongTrue();
void replyShortFalse();
void replyLongFalse();
void replyLongUnknow();
void replyValueSetShort(bool set);
void replyNothing();
void clearReply();

// PushTo
byte PushToEqu(Coord_EQ EQ_T, PoleSide preferedPoleSide, double Lat, float* deltaA1, float* deltaA2);
byte PushToHor(Coord_HO HO_T, PoleSide preferedPoleSide, float* deltaA1, float* deltaA2);

// GNSS
void UpdateGnss();
bool GNSSTimeIsValid();
bool GNSSLocationIsValid();
bool isHdopSmall();
bool isTimeSyncWithGNSS();
bool isLocationSyncWithGNSS();

// Command_others
void Command_E();
