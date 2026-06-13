/*
 * ESP8266WiFi.h - native shim for the verify_alpaca test harness.
 * Only `WiFi.macAddress(uint8_t*)` is needed by TeenAstroAlpaca::uniqueId().
 */
#pragma once

#include "Arduino.h"

class WiFiClass
{
public:
  void macAddress(uint8_t* mac)
  {
    static const uint8_t fixed[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    for (int i = 0; i < 6; ++i) mac[i] = fixed[i];
  }
};

extern WiFiClass WiFi;
