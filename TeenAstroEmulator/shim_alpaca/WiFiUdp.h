/*
 * WiFiUdp.h - native shim for the verify_alpaca test harness.
 * The test does not exercise UDP discovery; this is a pure stub.
 */
#pragma once

#include "Arduino.h"

class WiFiUDP
{
public:
  bool      begin(uint16_t)           { return true; }
  void      stop()                    {}
  int       parsePacket()             { return 0; }
  int       read(char*, int)          { return 0; }
  IPAddress remoteIP()                { return IPAddress(0); }
  uint16_t  remotePort()              { return 0; }
  void      beginPacket(IPAddress, uint16_t) {}
  void      write(const uint8_t*, size_t)    {}
  void      endPacket()               {}
};
