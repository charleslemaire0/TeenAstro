/*
 * ESP8266WebServer.h -- Stub for the SHC emulator.
 */
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>

#ifndef CONTENT_LENGTH_UNKNOWN
#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)
#endif

class String {
public:
    char buf[512];
    String() { buf[0] = 0; }
    String(const char* s) { strncpy(buf, s, sizeof(buf)-1); buf[sizeof(buf)-1]=0; }
    String& operator+=(const char* s) { strncat(buf, s, sizeof(buf)-strlen(buf)-1); return *this; }
    String& operator+=(const String& s) { return operator+=(s.buf); }
    String& operator=(const char* s) { strncpy(buf, s, sizeof(buf)-1); buf[sizeof(buf)-1]=0; return *this; }
    const char* c_str() const { return buf; }
    operator const char*() const { return buf; }
    int length() const { return (int)strlen(buf); }
};

class ESP8266WebServer {
public:
    ESP8266WebServer(int port = 80) { (void)port; }
    void begin() {}
    void on(const char*, void(*)()) {}
    void onNotFound(void(*)()) {}
    void handleClient() {}
    void setContentLength(size_t) {}
    void sendHeader(const char*, const char*) {}
    void send(int, const char*, const String&) {}
    void sendContent(const char*) {}
    void sendContent(const String&) {}
    bool hasArg(const char*) { return false; }
    String arg(const char*) { return String(""); }
    String arg(int) { return String(""); }
    int args() { return 0; }
    String argName(int) { return String(""); }
};
