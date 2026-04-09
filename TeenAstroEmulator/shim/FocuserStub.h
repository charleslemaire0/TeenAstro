/*
 * FocuserStub.h -- In-process emulation of the TeenAstro Focuser board.
 *
 * Implements the serial protocol that the real focuser firmware speaks over
 * its UART (Focus_Serial).  The MainUnit forwards :F...# commands here and
 * reads back the responses exactly as it would from a physical board.
 *
 * Supported commands:
 *   ?   state dump       (long reply ending with #)
 *   ~   text config      (long reply ending with #)
 *   M   motor config     (long reply ending with #)
 *   V   version          (long reply ending with #)
 *   A   binary config    (200 base64 chars + #)
 *   a   binary state     (12 base64 chars + #)
 *   x   position dump    (long reply ending with #)
 *   0-8,c,m,r  set param (short reply: '1' or '0')
 *   O,o,I,i    move      (short reply: '1')
 *   W          write/save (short reply: '1')
 *   +,-,G,P,Q,S,s,g,!,$  no reply
 */
#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>

class FocuserStub : public Stream {
    static constexpr int RXBUF = 64;
    static constexpr int TXBUF = 300;

    char   rxBuf_[RXBUF];
    int    rxPos_ = 0;
    bool   rxStarted_ = false;

    char   txBuf_[TXBUF];
    int    txHead_ = 0;
    int    txTail_ = 0;

    /* Focuser state (mirrors ConfigStorage defaults from ConfigStorage.ino) */
    unsigned int  resolution_  = 16;
    uint8_t       curr_        = 50;   /* ×10 = 500 mA */
    uint8_t       micro_       = 4;
    unsigned int  steprot_     = 200;
    uint8_t       reverse_     = 0;
    unsigned long startPos_    = 0;
    unsigned long maxPos_      = 65535;
    unsigned int  lowSpeed_    = 20;
    unsigned int  highSpeed_   = 20;
    uint8_t       cmdAcc_      = 30;
    uint8_t       manAcc_      = 30;
    uint8_t       manDec_      = 30;

    unsigned long position_    = 0;
    bool          moving_      = false;
    float         temperature_ = 22.5f;

    static const char B64[];

    void b64Encode(const uint8_t* in, char* out, int len) {
        int o = 0;
        for (int i = 0; i < len; i += 3) {
            uint32_t b = ((uint32_t)in[i] << 16)
                       | ((uint32_t)(i+1 < len ? in[i+1] : 0) << 8)
                       | (uint32_t)(i+2 < len ? in[i+2] : 0);
            out[o++] = B64[(b >> 18) & 0x3F];
            out[o++] = B64[(b >> 12) & 0x3F];
            out[o++] = B64[(b >>  6) & 0x3F];
            out[o++] = B64[ b        & 0x3F];
        }
        out[o] = 0;
    }

    static void packU16(uint8_t* p, int off, uint16_t v) {
        p[off]     = (uint8_t)(v & 0xFF);
        p[off + 1] = (uint8_t)(v >> 8);
    }
    static void packI16(uint8_t* p, int off, int16_t v) {
        packU16(p, off, (uint16_t)v);
    }

    void txPut(const char* s) {
        while (*s) {
            txBuf_[txHead_] = *s++;
            txHead_ = (txHead_ + 1) % TXBUF;
        }
    }
    void txPutChar(char c) {
        txBuf_[txHead_] = c;
        txHead_ = (txHead_ + 1) % TXBUF;
    }

    /* Process a complete :F<cmd>[<param>][,<value>]# command */
    void processCommand() {
        if (rxPos_ < 3) return;
        char cmd   = rxBuf_[2];
        char param = (rxPos_ >= 4) ? rxBuf_[3] : 0;
        bool hasValue = (param == ',');
        unsigned int value = hasValue ? (unsigned int)atoi(&rxBuf_[4]) : 0;

        switch (cmd) {
        /* --- State dump (:F?#) --- */
        case '?': {
            unsigned int pos = (unsigned int)(position_ / resolution_);
            char buf[50];
            sprintf(buf, "?%05u %03u +%02d.%02d#",
                    pos, 0,
                    (int)temperature_, abs((int)(fmod(temperature_*100, 100))));
            txPut(buf);
            break;
        }
        /* --- Text config dump (:F~#) --- */
        case '~': {
            char buf[60];
            sprintf(buf, "~%05u %05u %03u %03u %03u %03u %03u#",
                    (unsigned int)(startPos_ / resolution_),
                    (unsigned int)(maxPos_ / resolution_),
                    lowSpeed_, highSpeed_,
                    (unsigned int)cmdAcc_,
                    (unsigned int)manAcc_,
                    (unsigned int)manDec_);
            txPut(buf);
            break;
        }
        /* --- Motor config dump (:FM#) --- */
        case 'M': {
            char buf[50];
            sprintf(buf, "M%u %u %03u %03u %03u#",
                    (unsigned int)reverse_, (unsigned int)micro_,
                    resolution_, (unsigned int)curr_, steprot_);
            txPut(buf);
            break;
        }
        /* --- Version (:FV#) --- */
        case 'V': {
            txPut("$ TeenAstro Focuser 2.4.0 1.6.2#");
            break;
        }
        /* --- Binary config dump (:FA#) --- 150 bytes -> 200 b64 + '#' */
        case 'A': {
            uint8_t pkt[150];
            memset(pkt, 0, sizeof(pkt));
            packU16(pkt,  0, (uint16_t)(startPos_ / resolution_));
            packU16(pkt,  2, (uint16_t)(maxPos_ / resolution_));
            packU16(pkt,  4, (uint16_t)lowSpeed_);
            packU16(pkt,  6, (uint16_t)highSpeed_);
            pkt[8]  = cmdAcc_;
            pkt[9]  = manAcc_;
            pkt[10] = manDec_;
            pkt[11] = reverse_;
            pkt[12] = micro_;
            packU16(pkt, 13, (uint16_t)resolution_);
            pkt[15] = curr_;
            packU16(pkt, 16, (uint16_t)steprot_);
            /* positions 18..147: 10 × (uint16 pos + char[11] id) all zero */
            uint8_t xorChk = 0;
            for (int i = 0; i < 149; i++) xorChk ^= pkt[i];
            pkt[149] = xorChk;
            char b64[201];
            b64Encode(pkt, b64, 150);
            txPut(b64);
            txPutChar('#');
            break;
        }
        /* --- Binary state dump (:Fa#) --- 9 bytes -> 12 b64 + '#' */
        case 'a': {
            uint8_t pkt[9];
            memset(pkt, 0, sizeof(pkt));
            unsigned int p = (unsigned int)(position_ / resolution_);
            if (p > 65535U) p = 65535U;
            packU16(pkt, 0, (uint16_t)p);
            packU16(pkt, 2, 0);
            packI16(pkt, 4, (int16_t)(temperature_ * 100.0f));
            pkt[6] = moving_ ? 1 : 0;
            uint8_t xorChk = 0;
            for (int i = 0; i < 8; i++) xorChk ^= pkt[i];
            pkt[8] = xorChk;
            char b64[13];
            b64Encode(pkt, b64, 9);
            txPut(b64);
            txPutChar('#');
            break;
        }
        /* --- Position preset dump (:Fxn#) --- */
        case 'x':
            txPut("0#");
            break;

        /* --- Set commands (short response '1') --- */
        case '0': startPos_  = hasValue ? (unsigned long)value * resolution_ : startPos_;  txPutChar('1'); break;
        case '1': maxPos_    = hasValue ? (unsigned long)value * resolution_ : maxPos_;    txPutChar('1'); break;
        case '2': lowSpeed_  = hasValue ? value : lowSpeed_;  txPutChar('1'); break;
        case '3': highSpeed_ = hasValue ? value : highSpeed_; txPutChar('1'); break;
        case '4': cmdAcc_    = hasValue ? (uint8_t)value : cmdAcc_;    txPutChar('1'); break;
        case '5': manAcc_    = hasValue ? (uint8_t)value : manAcc_;    txPutChar('1'); break;
        case '6': manDec_    = hasValue ? (uint8_t)value : manDec_;    txPutChar('1'); break;
        case '7': reverse_   = hasValue ? (uint8_t)value : reverse_;   txPutChar('1'); break;
        case '8': resolution_ = hasValue ? value : resolution_;        txPutChar('1'); break;
        case 'c': curr_      = hasValue ? (uint8_t)value : curr_;      txPutChar('1'); break;
        case 'm': micro_     = hasValue ? (uint8_t)value : micro_;     txPutChar('1'); break;
        case 'r': steprot_   = hasValue ? value : steprot_;            txPutChar('1'); break;

        /* --- Move with response --- */
        case 'O': case 'o': case 'I': case 'i':
            txPutChar('1');
            break;

        /* --- Write/save --- */
        case 'W':
            txPutChar('1');
            break;

        /* --- No-response commands --- */
        case '+': case '-': case 'G': case 'g':
        case 'P': case 'Q': case 'S': case 's':
        case '!': case '$':
            break;

        default:
            break;
        }
    }

public:
    FocuserStub() {
        memset(rxBuf_, 0, sizeof(rxBuf_));
        memset(txBuf_, 0, sizeof(txBuf_));
    }

    int available() override {
        int n = txHead_ - txTail_;
        if (n < 0) n += TXBUF;
        return n;
    }

    int read() override {
        if (txHead_ == txTail_) return -1;
        char c = txBuf_[txTail_];
        txTail_ = (txTail_ + 1) % TXBUF;
        return (unsigned char)c;
    }

    int peek() override {
        if (txHead_ == txTail_) return -1;
        return (unsigned char)txBuf_[txTail_];
    }

    size_t write(uint8_t b) override {
        char c = (char)b;
        if (!rxStarted_) {
            if (c == ':') {
                rxStarted_ = true;
                rxPos_ = 0;
                rxBuf_[rxPos_++] = c;
            }
            return 1;
        }
        rxBuf_[rxPos_] = c;
        if (c == '#') {
            rxBuf_[rxPos_ + 1] = 0;
            rxStarted_ = false;
            processCommand();
            rxPos_ = 0;
        } else {
            rxPos_++;
            if (rxPos_ >= RXBUF - 1) {
                rxStarted_ = false;
                rxPos_ = 0;
            }
        }
        return 1;
    }

    void flush() override {}
};

const char FocuserStub::B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
