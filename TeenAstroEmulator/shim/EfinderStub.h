/*
 * EfinderStub.h -- In-process emulation of TeenAstroEfinder (Pi plate-solver).
 *
 * Speaks the :e…# UART protocol that TeenAstroMainUnit::EfinderManager expects
 * on the Focuser (56000) or GNSS (9600) port. Attach to Serial3 (GNSS) so the
 * focuser stub can stay on Serial2.
 *
 * Solves complete after a short delay; returned RA/Dec track a configurable
 * offset from the last site, or fixed JNow coordinates for desk tests.
 */
#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <Arduino.h>

class EfinderStub : public Stream {
  static constexpr int RXBUF = 96;
  static constexpr int TXBUF = 256;
  static constexpr unsigned long SOLVE_MS = 400;

  char rxBuf_[RXBUF];
  int  rxPos_ = 0;
  bool rxStarted_ = false;

  char txBuf_[TXBUF];
  int  txHead_ = 0;
  int  txTail_ = 0;

  bool busy_ = false;
  bool isOffset_ = false;
  unsigned long busyUntil_ = 0;
  bool failNext_ = false;

  double lat_ = 48.1;
  double lonEast_ = 2.3;
  int elev_ = 100;
  char timeStamp_[16] = "20260101120000";

  double pointingRaH_ = 5.5;
  double pointingDec_ = 22.0;
  bool pointingValid_ = false;

  double solvedRaH_ = 5.5;
  double solvedDec_ = 22.0;
  int stars_ = 42;
  int solveMs_ = 350;

  double offsetDx_ = 0.0;
  double offsetDy_ = 0.0;
  char offsetName_[24] = "Polaris";

  bool nmeaOn_ = false;
  bool gpsPresent_ = true;
  unsigned long lastNmea_ = 0;

  char exposure_[16] = "1.0";
  char gain_[16] = "50";

  void txPut(const char* s)
  {
    while (*s) {
      txBuf_[txHead_] = *s++;
      txHead_ = (txHead_ + 1) % TXBUF;
    }
  }

  void txPutChar(char c)
  {
    txBuf_[txHead_] = c;
    txHead_ = (txHead_ + 1) % TXBUF;
  }

  void replyResult()
  {
    if (busy_ && (long)(millis() - busyUntil_) < 0) {
      txPut("B#");
      return;
    }
    busy_ = false;
    if (failNext_) {
      failNext_ = false;
      txPut("FS#");
      return;
    }
    if (isOffset_) {
      char buf[64];
      snprintf(buf, sizeof(buf), "O%.2f,%.2f,%s#", offsetDx_, offsetDy_, offsetName_);
      txPut(buf);
      isOffset_ = false;
      return;
    }
    char buf[80];
    snprintf(buf, sizeof(buf), "S%.6f,%.5f,%d,%d#",
             solvedRaH_, solvedDec_, stars_, solveMs_);
    txPut(buf);
  }

  void processCommand()
  {
    // rxBuf_ is :e...#
    if (rxPos_ < 3 || rxBuf_[1] != 'e') return;
    const char* body = &rxBuf_[2]; // after ":e"

    if (body[0] == '?' && (body[1] == '#' || body[1] == 0)) {
      txPut("TeenAstroEfinder,emu-0.1#");
      return;
    }
    if (body[0] == 'T' && strlen(body) >= 15) {
      strncpy(timeStamp_, body + 1, 14);
      timeStamp_[14] = 0;
      txPut("1#");
      return;
    }
    if (body[0] == 'L') {
      double la = 0, lo = 0;
      int el = 0;
      if (sscanf(body + 1, "%lf,%lf,%d", &la, &lo, &el) >= 2) {
        lat_ = la;
        lonEast_ = lo;
        elev_ = el;
        txPut("1#");
      } else {
        txPut("0#");
      }
      return;
    }
    if (body[0] == 'C') {
      double ra = 0, dec = 0;
      if (sscanf(body + 1, "%lf,%lf", &ra, &dec) == 2) {
        pointingRaH_ = fmod(ra, 24.0);
        if (pointingRaH_ < 0) pointingRaH_ += 24.0;
        pointingDec_ = dec;
        pointingValid_ = true;
        // Star density proxy: more stars near galactic plane / mid declination
        double dens = 35.0 + 40.0 * cos(pointingDec_ * 0.017453292519943295);
        if (dens < 12.0) dens = 12.0;
        stars_ = (int)dens;
        txPut("1#");
      } else {
        txPut("0#");
      }
      return;
    }
    if (body[0] == 'S' && (body[1] == '#' || body[1] == 0)) {
      if (busy_) { txPut("0#"); return; }
      busy_ = true;
      isOffset_ = false;
      busyUntil_ = millis() + SOLVE_MS;
      // Synthetic field: report mount pointing (+ tiny noise), not a fixed sky patch
      if (pointingValid_) {
        solvedRaH_ = fmod(pointingRaH_ + 0.0002, 24.0);
        solvedDec_ = pointingDec_ + 0.001;
      } else {
        solvedRaH_ = fmod(solvedRaH_ + 0.001, 24.0);
      }
      txPut("1#");
      return;
    }
    if (body[0] == 'O' && (body[1] == '#' || body[1] == 0)) {
      if (busy_) { txPut("0#"); return; }
      busy_ = true;
      isOffset_ = true;
      busyUntil_ = millis() + SOLVE_MS;
      if (pointingValid_) {
        solvedRaH_ = pointingRaH_;
        solvedDec_ = pointingDec_;
      }
      txPut("1#");
      return;
    }
    if (body[0] == 'o') {
      offsetDx_ = offsetDy_ = 0.0;
      txPut("1#");
      return;
    }
    if (body[0] == 'R') {
      replyResult();
      return;
    }
    if (body[0] == 'N') {
      if (body[1] == '1') { nmeaOn_ = true;  txPut("1#"); return; }
      if (body[1] == '0') { nmeaOn_ = false; txPut("1#"); return; }
      if (body[1] == '?') { txPut(gpsPresent_ ? "1#" : "0#"); return; }
    }
    if (body[0] == 'X') {
      // :eXkey=value# or :eXkey#
      const char* p = body + 1;
      const char* eq = strchr(p, '=');
      if (eq) {
        char key[24];
        size_t n = (size_t)(eq - p);
        if (n >= sizeof(key)) n = sizeof(key) - 1;
        memcpy(key, p, n);
        key[n] = 0;
        if (strcmp(key, "Exposure") == 0) {
          strncpy(exposure_, eq + 1, sizeof(exposure_) - 1);
          char* hash = strchr(exposure_, '#');
          if (hash) *hash = 0;
        } else if (strcmp(key, "Gain") == 0) {
          strncpy(gain_, eq + 1, sizeof(gain_) - 1);
          char* hash = strchr(gain_, '#');
          if (hash) *hash = 0;
        }
        txPut("1#");
      } else {
        char key[24];
        strncpy(key, p, sizeof(key) - 1);
        key[sizeof(key) - 1] = 0;
        char* hash = strchr(key, '#');
        if (hash) *hash = 0;
        if (strcmp(key, "Exposure") == 0) { txPut(exposure_); txPutChar('#'); }
        else if (strcmp(key, "Gain") == 0) { txPut(gain_); txPutChar('#'); }
        else txPut("#");
      }
      return;
    }
    txPut("0#");
  }

  void maybeInjectNmea()
  {
    if (!nmeaOn_ || !gpsPresent_) return;
    if (millis() - lastNmea_ < 1000) return;
    lastNmea_ = millis();
    // Minimal GGA so TinyGPS++ sees a fix (lat/lon approx from site)
    char buf[96];
    int latDeg = (int)fabs(lat_);
    double latMin = (fabs(lat_) - latDeg) * 60.0;
    char ns = lat_ >= 0 ? 'N' : 'S';
    int lonDeg = (int)fabs(lonEast_);
    double lonMin = (fabs(lonEast_) - lonDeg) * 60.0;
    char ew = lonEast_ >= 0 ? 'E' : 'W';
    snprintf(buf, sizeof(buf),
             "$GPGGA,120000.00,%02d%07.4f,%c,%03d%07.4f,%c,1,08,1.0,%.1f,M,0.0,M,,*00\r\n",
             latDeg, latMin, ns, lonDeg, lonMin, ew, (double)elev_);
    txPut(buf);
  }

public:
  EfinderStub()
  {
    memset(rxBuf_, 0, sizeof(rxBuf_));
    memset(txBuf_, 0, sizeof(txBuf_));
  }

  void setSolved(double raH, double decD)
  {
    solvedRaH_ = raH;
    solvedDec_ = decD;
  }

  void setFailNext(bool v) { failNext_ = v; }
  void setGpsPresent(bool v) { gpsPresent_ = v; }

  /** Call from the emulator main loop so NMEA keeps flowing when enabled. */
  void tick() { maybeInjectNmea(); }

  int available() override
  {
    maybeInjectNmea();
    int n = txHead_ - txTail_;
    if (n < 0) n += TXBUF;
    return n;
  }

  int read() override
  {
    if (txHead_ == txTail_) return -1;
    char c = txBuf_[txTail_];
    txTail_ = (txTail_ + 1) % TXBUF;
    return (unsigned char)c;
  }

  int peek() override
  {
    if (txHead_ == txTail_) return -1;
    return (unsigned char)txBuf_[txTail_];
  }

  size_t write(uint8_t b) override
  {
    char c = (char)b;
    if (!rxStarted_) {
      if (c == ':') {
        rxStarted_ = true;
        rxPos_ = 0;
        rxBuf_[rxPos_++] = c;
      }
      return 1;
    }
    if (rxPos_ < RXBUF - 1)
      rxBuf_[rxPos_++] = c;
    if (c == '#') {
      rxBuf_[rxPos_] = 0;
      rxStarted_ = false;
      processCommand();
      rxPos_ = 0;
    } else if (rxPos_ >= RXBUF - 1) {
      rxStarted_ = false;
      rxPos_ = 0;
    }
    return 1;
  }

  void flush() override {}
};
