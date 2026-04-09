#pragma once
// Emulator-only NDJSON debug logging (session 6b215f). No-op on real hardware builds.
// Uses C stdio only — iostream/fstream conflicts with arduino.h min()/max() macros in emu TU.
#ifdef EMU_MAINUNIT
#include <cstdio>
#include <ctime>
inline void emuDbgGotoLog(const char* location, const char* hypothesisId, int settling,
  unsigned long elapsedMs, int gotoState, int earlyReturn)
{
  FILE* fp = fopen("c:/Users/charl/source/repos/charleslemaire0/TeenAstro/debug-6b215f.log", "a");
  if (!fp) return;
  long long ms = (long long)std::time(nullptr) * 1000LL;
  std::fprintf(fp,
    "{\"sessionId\":\"6b215f\",\"hypothesisId\":\"%s\",\"location\":\"%s\","
    "\"data\":{\"settling\":%d,\"elapsedMs\":%lu,\"gotoState\":%d,\"earlyReturn\":%d},\"timestamp\":%lld}\n",
    hypothesisId, location, settling,
    static_cast<unsigned long>(elapsedMs), gotoState, earlyReturn, ms);
  std::fclose(fp);
}
#else
inline void emuDbgGotoLog(const char*, const char*, int, unsigned long, int, int) {}
#endif
