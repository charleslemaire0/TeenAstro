/*
 * Print.h -- Arduino Print base class for the emulator.
 * U8G2 inherits from Print and only uses write(uint8_t).
 */
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t b) { (void)b; return 1; }
    virtual size_t write(const uint8_t* buf, size_t len) {
        size_t n = 0;
        for (size_t i = 0; i < len; i++) n += write(buf[i]);
        return n;
    }

    size_t print(const char* s) {
        size_t n = 0;
        while (*s) { write((uint8_t)*s++); n++; }
        return n;
    }
    size_t print(char c) { return write((uint8_t)c); }
    size_t print(int val) { char buf[16]; sprintf(buf, "%d", val); return print(buf); }
    size_t print(unsigned int val) { char buf[16]; sprintf(buf, "%u", val); return print(buf); }
    size_t print(long val) { char buf[24]; sprintf(buf, "%ld", val); return print(buf); }
    size_t print(unsigned long val) { char buf[24]; sprintf(buf, "%lu", val); return print(buf); }
    size_t print(double val, int prec = 2) {
        char fmt[16], buf[32];
        sprintf(fmt, "%%.%df", prec);
        sprintf(buf, fmt, val);
        return print(buf);
    }
    size_t println(const char* s = "") { return print(s) + write('\n'); }
    size_t println(int val) { return print(val) + write('\n'); }
    size_t println(unsigned long val) { return print(val) + write('\n'); }
    size_t println(double val, int prec = 2) { return print(val, prec) + write('\n'); }
};
