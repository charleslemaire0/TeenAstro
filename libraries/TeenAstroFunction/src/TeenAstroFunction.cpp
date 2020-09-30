#include<TeenAstroFunction.h>

void gethms(const long& v, uint8_t& v1, uint8_t& v2, uint8_t& v3)
{
  v3 = v % 60;
  v2 = (v / 60) % 60;
  v1 = v / 3600;
  return;
}

void getdms(const long& v, bool& ispos, uint16_t& v1, uint8_t& v2, uint8_t& v3)
{
  ispos = v >= 0;
  long vabs = ispos ? v : -v;
  v3 = vabs % 60;
  v2 = (vabs / 60) % 60;
  v1 = vabs / 3600;
  return;
}

void longRa2Ra(long Ra, int& h, int& m, int& s)
{
  h = Ra / 30;
  m = (Ra - h * 30) / 60;
  s = (Ra / 30) % 60;
}

void longDec2Dec(long Dec, bool& ispos, unsigned short& deg, uint8_t& min)
{
  ispos = Dec >= 0;
  Dec = Dec ? Dec : -Dec;
  deg = Dec / 60;
  min = Dec % 60;
}
