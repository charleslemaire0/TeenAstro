#pragma once


#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

void gethms(const long& v, uint8_t& v1, uint8_t& v2, uint8_t& v3);

void getdms(const long& v, bool& ispos, uint16_t& v1, uint8_t& v2, uint8_t& v3);

void longRa2Ra(long Ra, int& h, int& m, int& s);

void longDec2Dec(long Dec, bool& ispos, unsigned short& deg, uint8_t& min);