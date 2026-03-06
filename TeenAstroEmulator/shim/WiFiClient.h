/*
 * WiFiClient.h -- Stub for Arduino WiFiClient (SHC emulator).
 */
#pragma once
#include "arduino.h"

class IPAddress {
public:
    uint8_t d[4] = {0};
    IPAddress() {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t e) { d[0]=a; d[1]=b; d[2]=c; d[3]=e; }
    uint8_t operator[](int i) const { return d[i]; }
    uint8_t& operator[](int i) { return d[i]; }
};

class WiFiClient : public Stream {
public:
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t write(uint8_t) override { return 0; }
    bool connected() { return false; }
    void stop() {}
};

class WiFiServer {
public:
    WiFiServer(int port = 80) { (void)port; }
    void begin() {}
    WiFiClient available() { return WiFiClient(); }
};
