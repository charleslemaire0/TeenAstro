/*
 * EEPROM.h - File-backed EEPROM for the emulator.
 *
 * On construction, loads from a binary file in the current working directory.
 * On commit(), saves back to disk so values persist across restarts.
 *
 * File names:
 *   EMU_MAINUNIT  ->  "teenastro_mainunit_eeprom.bin"
 *   EMU_SHC       ->  "teenastro_shc_eeprom.bin"
 *   (other)       ->  "teenastro_eeprom.bin"
 */
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>

class EEPROMClass {
    static constexpr int SIZE = 4096;
    uint8_t data_[SIZE];
    bool    dirty_ = false;

    const char* filename() const {
#if defined(EMU_MAINUNIT)
        return "teenastro_mainunit_eeprom.bin";
#elif defined(EMU_SHC)
        return "teenastro_shc_eeprom.bin";
#else
        return "teenastro_eeprom.bin";
#endif
    }

    void loadFromFile() {
        FILE* f = fopen(filename(), "rb");
        if (f) {
            size_t n = fread(data_, 1, SIZE, f);
            fclose(f);
            printf("[EEPROM] Loaded %d bytes from %s\n", (int)n, filename());
            fflush(stdout);
        } else {
            printf("[EEPROM] No file %s found, starting with defaults (0xFF)\n", filename());
            fflush(stdout);
        }
    }

    void saveToFile() {
        FILE* f = fopen(filename(), "wb");
        if (f) {
            fwrite(data_, 1, SIZE, f);
            fclose(f);
            dirty_ = false;
        } else {
            printf("[EEPROM] WARNING: could not write %s\n", filename());
            fflush(stdout);
        }
    }

public:
    EEPROMClass() {
        memset(data_, 0xFF, SIZE);
        loadFromFile();
    }

    ~EEPROMClass() {
        if (dirty_) saveToFile();
    }

    uint8_t read(int addr) const {
        if (addr < 0 || addr >= SIZE) return 0xFF;
        return data_[addr];
    }

    void write(int addr, uint8_t val) {
        if (addr >= 0 && addr < SIZE) {
            data_[addr] = val;
            dirty_ = true;
        }
    }

    void update(int addr, uint8_t val) { write(addr, val); }

    void begin(int) {}

    void commit() {
        if (dirty_) saveToFile();
    }

    uint16_t length() const { return SIZE; }

    template<typename T>
    T& get(int addr, T& val) {
        if (addr >= 0 && addr + (int)sizeof(T) <= SIZE)
            memcpy(&val, &data_[addr], sizeof(T));
        return val;
    }

    template<typename T>
    void put(int addr, const T& val) {
        if (addr >= 0 && addr + (int)sizeof(T) <= SIZE) {
            memcpy(&data_[addr], &val, sizeof(T));
            dirty_ = true;
        }
    }

    uint8_t& operator[](int addr) {
        dirty_ = true;
        return data_[addr];
    }
};

#ifndef EEPROM_INSTANCE_DEFINED
#define EEPROM_INSTANCE_DEFINED
static EEPROMClass EEPROM;
#endif
