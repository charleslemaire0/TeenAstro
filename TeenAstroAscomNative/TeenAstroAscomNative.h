/*
 * TeenAstroAscomNative.h - C API for P/Invoke from ASCOM driver
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#ifndef TEENASTRO_ASCOM_NATIVE_H
#define TEENASTRO_ASCOM_NATIVE_H

#ifdef _WIN32
  #ifdef TEENASTROASCOMNATIVE_EXPORTS
    #define TA_ASCOM_API __declspec(dllexport)
  #else
    #define TA_ASCOM_API __declspec(dllimport)
  #endif
#else
  #define TA_ASCOM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TeenAstroAscom_Handle;

/// Parsed GXAS mount state for ASCOM properties
typedef struct TeenAstroAscom_GXASState
{
  int   valid;           /* 1 if parse succeeded */
  double rightAscensionHours;
  double declinationDegrees;
  double altitudeDegrees;
  double azimuthDegrees;
  double targetRAHours;
  double targetDecDegrees;
  double siderealTimeHours;
  int    tracking;      /* 0=off, 1=on */
  int    slewing;
  int    atHome;
  int    parkState;     /* 0=unparked, 1=parking, 2=parked, 3=failed */
  int    pierSideWest;  /* 0=east, 1=west */
  int    isPulseGuiding;
  int    utcYear;
  int    utcMonth;
  int    utcDay;
  int    utcHour;
  int    utcMin;
  int    utcSec;
} TeenAstroAscom_GXASState;

TA_ASCOM_API TeenAstroAscom_Handle TeenAstroAscom_ConnectSerial(const char* port);
TA_ASCOM_API TeenAstroAscom_Handle TeenAstroAscom_ConnectTcp(const char* ip, int port);
TA_ASCOM_API void TeenAstroAscom_Disconnect(TeenAstroAscom_Handle h);

TA_ASCOM_API int TeenAstroAscom_CommandBlind(TeenAstroAscom_Handle h, const char* cmd, int raw);
TA_ASCOM_API int TeenAstroAscom_CommandBool(TeenAstroAscom_Handle h, const char* cmd, int raw);
TA_ASCOM_API int TeenAstroAscom_CommandString(TeenAstroAscom_Handle h, const char* cmd, int raw,
                                               char* outBuf, int outBufSize);

TA_ASCOM_API int TeenAstroAscom_GetMountState(TeenAstroAscom_Handle h, TeenAstroAscom_GXASState* out);

/* Semantic API - ASCOM driver calls these, never raw commands */
TA_ASCOM_API int TeenAstroAscom_AbortSlew(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_HasSite(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_HasMotors(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_Park(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_Unpark(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_SetPark(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_PulseGuide(TeenAstroAscom_Handle h, int direction, int durationMs);
TA_ASCOM_API int TeenAstroAscom_MoveAxis(TeenAstroAscom_Handle h, int axis, double rateArcsecPerSec);
TA_ASCOM_API int TeenAstroAscom_SyncToEquatorial(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_SyncToAltAz(TeenAstroAscom_Handle h);
TA_ASCOM_API int TeenAstroAscom_SlewToEquatorial(TeenAstroAscom_Handle h, char* outReply, int outSize);
TA_ASCOM_API int TeenAstroAscom_SlewToAltAz(TeenAstroAscom_Handle h, char* outReply, int outSize);
TA_ASCOM_API int TeenAstroAscom_SetTargetRA(TeenAstroAscom_Handle h, double raHours);
TA_ASCOM_API int TeenAstroAscom_SetTargetDec(TeenAstroAscom_Handle h, double decDeg);
TA_ASCOM_API int TeenAstroAscom_SetTargetAz(TeenAstroAscom_Handle h, const char* azStr);
TA_ASCOM_API int TeenAstroAscom_SetTargetAlt(TeenAstroAscom_Handle h, const char* altStr);
TA_ASCOM_API int TeenAstroAscom_GetSiteLatitude(TeenAstroAscom_Handle h, double* outLat);
TA_ASCOM_API int TeenAstroAscom_GetSiteLongitude(TeenAstroAscom_Handle h, double* outLon);
TA_ASCOM_API int TeenAstroAscom_SetSiteLatitude(TeenAstroAscom_Handle h, const char* latStr);
TA_ASCOM_API int TeenAstroAscom_SetSiteLongitude(TeenAstroAscom_Handle h, const char* lonStr);
TA_ASCOM_API int TeenAstroAscom_GetUTCTimestamp(TeenAstroAscom_Handle h, double* outSecs);
TA_ASCOM_API int TeenAstroAscom_SetUTCTimestamp(TeenAstroAscom_Handle h, long unixSecs);
TA_ASCOM_API int TeenAstroAscom_EnableTracking(TeenAstroAscom_Handle h, int on);

#ifdef __cplusplus
}
#endif

#endif /* TEENASTRO_ASCOM_NATIVE_H */
