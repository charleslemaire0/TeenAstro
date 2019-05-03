#pragma once

#ifndef uint8_t
typedef unsigned char uint8_t ;
#endif

void gethms(const long& v, uint8_t& v1, uint8_t& v2, uint8_t& v3)
{
  v3 = v % 60;
  v2 = (v / 60) % 60;
  v1 = v / 3600;
}

void secondsToFloat(const long& v, float& f)
{
  f = (double)v / 3600.0;
}


void getdms(const long& v, short& v1, uint8_t& v2, uint8_t& v3)
{
  v3 = abs(v) % 60;
  v2 = (abs(v) / 60) % 60;
  v1 = v / 3600;
}

void longRa2Ra(long Ra, int& h, int& m, int& s)
{
  h = Ra / 30;
  m = (Ra - h * 30) / 60;
  s = (Ra / 30) % 60;
}

void longDec2Dec(long Dec, int& deg, int& min)
{
  deg = Dec / 60;
  min = Dec % 60;
}