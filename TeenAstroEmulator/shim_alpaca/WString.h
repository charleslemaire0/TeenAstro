/*
 * WString.h - Arduino-compatible String class for the verify_alpaca shim.
 *
 * Backed by std::string.  Implements the subset of the API used by
 * libraries/TeenAstroAlpaca (AlpacaResponse + AlpacaTelescope).
 */
#pragma once

#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstdint>

class __FlashStringHelper;

class String
{
public:
  std::string s;

  // ----- constructors -----
  String() = default;
  String(const String& o)         : s(o.s) {}
  String(const char* p)           : s(p ? p : "") {}
  String(char c)                  { s.push_back(c); }
  String(const __FlashStringHelper* p)
    : s(p ? reinterpret_cast<const char*>(p) : "") {}
  explicit String(int v)          { char b[32]; std::snprintf(b, sizeof(b), "%d",   v); s = b; }
  explicit String(unsigned int v) { char b[32]; std::snprintf(b, sizeof(b), "%u",   v); s = b; }
  explicit String(long v)         { char b[32]; std::snprintf(b, sizeof(b), "%ld",  v); s = b; }
  explicit String(unsigned long v){ char b[32]; std::snprintf(b, sizeof(b), "%lu",  v); s = b; }
  explicit String(double v, int prec = 2)
                                  { char b[64]; std::snprintf(b, sizeof(b), "%.*f", prec, v); s = b; }
  explicit String(float v, int prec = 2)
                                  { char b[64]; std::snprintf(b, sizeof(b), "%.*f", prec, (double)v); s = b; }

  // ----- assignment -----
  String& operator=(const String& o)        { s = o.s; return *this; }
  String& operator=(const char* p)          { s = (p ? p : ""); return *this; }
  String& operator=(const __FlashStringHelper* p)
                                            { s = (p ? reinterpret_cast<const char*>(p) : ""); return *this; }

  // ----- concatenation -----
  String& operator+=(const String& o)       { s += o.s; return *this; }
  String& operator+=(const char* p)         { if (p) s += p; return *this; }
  String& operator+=(char c)                { s += c; return *this; }
  String& operator+=(int v)                 { char b[32]; std::snprintf(b, sizeof(b), "%d", v); s += b; return *this; }
  String& operator+=(unsigned int v)        { char b[32]; std::snprintf(b, sizeof(b), "%u", v); s += b; return *this; }
  String& operator+=(long v)                { char b[32]; std::snprintf(b, sizeof(b), "%ld", v); s += b; return *this; }
  String& operator+=(unsigned long v)       { char b[32]; std::snprintf(b, sizeof(b), "%lu", v); s += b; return *this; }
  String& operator+=(double v)              { char b[64]; std::snprintf(b, sizeof(b), "%g", v); s += b; return *this; }
  String& operator+=(const __FlashStringHelper* p)
                                            { if (p) s += reinterpret_cast<const char*>(p); return *this; }

  // ----- comparison -----
  bool operator==(const String& o) const    { return s == o.s; }
  bool operator==(const char* p) const      { return p && s == p; }
  bool operator==(const __FlashStringHelper* p) const
                                            { return p && s == reinterpret_cast<const char*>(p); }
  bool operator!=(const String& o) const    { return !(*this == o); }
  bool operator!=(const char* p) const      { return !(*this == p); }

  // ----- access -----
  int         length() const                { return (int)s.size(); }
  const char* c_str()  const                { return s.c_str(); }
  operator    const char*() const           { return s.c_str(); }
  char        operator[](int i) const       { return (i >= 0 && i < (int)s.size()) ? s[i] : 0; }
  char&       operator[](int i)             { return s[(size_t)i]; }
  char        charAt(int i) const           { return (*this)[i]; }

  // ----- buffer management -----
  void reserve(size_t n)                    { s.reserve(n); }

  // ----- case -----
  void toLowerCase() { for (auto& c : s) c = (char)std::tolower((unsigned char)c); }
  void toUpperCase() { for (auto& c : s) c = (char)std::toupper((unsigned char)c); }

  // ----- numeric -----
  long   toInt()    const { return std::strtol(s.c_str(), nullptr, 10); }
  double toDouble() const { return std::strtod(s.c_str(), nullptr); }
  float  toFloat()  const { return (float)toDouble(); }

  // ----- search -----
  int indexOf(char c, int from = 0) const
  {
    auto p = s.find(c, (size_t)from);
    return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const char* needle, int from = 0) const
  {
    if (!needle) return -1;
    auto p = s.find(needle, (size_t)from);
    return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const String& needle, int from = 0) const { return indexOf(needle.c_str(), from); }
  int lastIndexOf(char c) const
  {
    auto p = s.rfind(c);
    return p == std::string::npos ? -1 : (int)p;
  }

  // ----- substring -----
  String substring(int from) const
  {
    if (from < 0) from = 0;
    if (from > (int)s.size()) from = (int)s.size();
    return String(s.substr((size_t)from).c_str());
  }
  String substring(int from, int to) const
  {
    if (from < 0) from = 0;
    if (to > (int)s.size()) to = (int)s.size();
    if (to <= from) return String("");
    return String(s.substr((size_t)from, (size_t)(to - from)).c_str());
  }

  // ----- prefix / suffix -----
  bool startsWith(const String& p) const { return s.rfind(p.s, 0) == 0; }
  bool startsWith(const char* p)   const { return p && s.rfind(p, 0) == 0; }
  bool startsWith(const __FlashStringHelper* p) const
  { return p && s.rfind(reinterpret_cast<const char*>(p), 0) == 0; }
  bool endsWith(const char* p) const
  {
    if (!p) return false;
    size_t n = std::strlen(p);
    if (n > s.size()) return false;
    return s.compare(s.size() - n, n, p) == 0;
  }

  // ----- trim -----
  void trim()
  {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) { s.clear(); return; }
    size_t b = s.find_last_not_of(" \t\r\n");
    s = s.substr(a, b - a + 1);
  }
};

inline String operator+(const String& a, const String& b)            { String r(a); r += b; return r; }
inline String operator+(const String& a, const char* b)              { String r(a); r += b; return r; }
inline String operator+(const char* a,   const String& b)            { String r(a); r += b; return r; }
inline String operator+(const String& a, char b)                     { String r(a); r += b; return r; }
inline String operator+(const String& a, int b)                      { String r(a); r += b; return r; }
inline String operator+(const String& a, const __FlashStringHelper* b){ String r(a); r += b; return r; }
