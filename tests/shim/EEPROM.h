/*
 * EEPROM.h - In-memory EEPROM stub for native tests.
 */
#pragma once
#include <cstdint>
#include <cstring>

class EEPROMClass {
    uint8_t data_[4096];
public:
    EEPROMClass() { memset(data_, 0xFF, sizeof(data_)); }
    uint8_t read(int addr) const {
        if (addr < 0 || addr >= (int)sizeof(data_)) return 0xFF;
        return data_[addr];
    }
    void write(int addr, uint8_t val) {
        if (addr >= 0 && addr < (int)sizeof(data_)) data_[addr] = val;
    }
    void update(int addr, uint8_t val) { write(addr, val); }
    void commit() {}
    uint16_t length() const { return sizeof(data_); }
    uint8_t& operator[](int addr) { return data_[addr]; }
};

#ifndef EEPROM_INSTANCE_DEFINED
#define EEPROM_INSTANCE_DEFINED
static EEPROMClass EEPROM;
#endif
