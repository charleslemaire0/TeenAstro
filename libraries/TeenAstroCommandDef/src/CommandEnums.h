/*
 * CommandEnums.h - Shared enumerations for TeenAstro LX200 protocol
 *
 * These enums are the single source of truth used by both the client
 * (LX200Client / TeenAstroLX200io) and the server (MainUnit).
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
//  LX200RETURN - Result codes for LX200 client operations
// ---------------------------------------------------------------------------
enum LX200RETURN
{
  // Errors (values below LX200_OK)
  LX200_NOTOK,
  LX200_INVALIDCOMMAND, LX200_INVALIDREPLY,
  LX200_SETVALUEFAILED, LX200_GETVALUEFAILED, LX200_SYNCFAILED,
  LX200_SETTARGETFAILED, LX200_TARGETBELOWHORIZON, LX200_TARGETABOVEOVERHEAD,
  LX200_GOHOME_FAILED, LX200_GOPARK_FAILED,
  LX200_FLIPSAMESIDE,
  LX200_ERR_MOTOR_FAULT, LX200_ERR_ALT, LX200_ERR_AXIS1, LX200_ERR_AXIS2,
  LX200_ERR_LIMIT_SENSE, LX200_ERR_UNDER_POLE, LX200_ERR_MERIDIAN,
  LX200_ERRGOTO_NOOBJECTSELECTED, LX200_ERRGOTO_PARKED, LX200_ERRGOTO_BUSY,
  LX200_ERRGOTO_LIMITS, LX200_ERRGOTO_UNKOWN,
  // Success (values >= LX200_OK)
  LX200_OK,
  LX200_VALUESET, LX200_VALUEGET, LX200_SYNCED,
  LX200_GOTO_TARGET, LX200_GOHOME, LX200_GOPARK,
  LX200_PUSHTO_TARGET
};

// ---------------------------------------------------------------------------
//  CMDREPLY - Expected reply type from a command
// ---------------------------------------------------------------------------
enum CMDREPLY
{
  CMDR_NO,           // No reply expected
  CMDR_SHORT,        // Single character reply
  CMDR_SHORT_BOOL,   // Single character '0' or '1'
  CMDR_LONG,         // String terminated by '#'
  CMDR_INVALID       // Invalid / unknown command
};

// ---------------------------------------------------------------------------
//  ErrorsGoTo - Goto error codes (wire encoding from main unit)
// ---------------------------------------------------------------------------
enum ErrorsGoTo
{
  ERRGOTO_NONE,
  ERRGOTO_BELOWHORIZON,
  ERRGOTO_NOOBJECTSELECTED,
  ERRGOTO_SAMESIDE,
  ERRGOTO_PARKED,
  ERRGOTO_SLEWING,
  ERRGOTO_LIMITS,
  ERRGOTO_GUIDINGBUSY,
  ERRGOTO_ABOVEOVERHEAD,
  ERRGOTO_MOTOR,
  ERRGOTO____,
  ERRGOTO_MOTOR_FAULT,
  ERRGOTO_ALT,
  ERRGOTO_LIMIT_SENSE,
  ERRGOTO_AXIS1,
  ERRGOTO_AXIS2,
  ERRGOTO_UNDER_POLE,
  ERRGOTO_MERIDIAN
};

// ---------------------------------------------------------------------------
//  TARGETTYPE - Coordinate frame for goto/pushto target
// ---------------------------------------------------------------------------
enum TARGETTYPE
{
  T_AZALT, T_RADEC, T_USERRADEC
};

// ---------------------------------------------------------------------------
//  NAV - Navigation mode (sync / goto / pushto)
// ---------------------------------------------------------------------------
enum NAV { NAV_SYNC, NAV_GOTO, NAV_PUSHTO };

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

/// Returns true if the LX200RETURN value represents success.
inline bool isOk(LX200RETURN val)
{
  return val >= LX200_OK;
}

/// Convert an ErrorsGoTo wire value to the corresponding LX200RETURN.
inline LX200RETURN gotoErrorToReturn(ErrorsGoTo err, bool isGoto)
{
  switch (err)
  {
  case ERRGOTO_NONE:            return isGoto ? LX200_GOTO_TARGET : LX200_PUSHTO_TARGET;
  case ERRGOTO_BELOWHORIZON:    return LX200_TARGETBELOWHORIZON;
  case ERRGOTO_NOOBJECTSELECTED:return LX200_ERRGOTO_NOOBJECTSELECTED;
  case ERRGOTO_SAMESIDE:        return LX200_FLIPSAMESIDE;
  case ERRGOTO_PARKED:          return LX200_ERRGOTO_PARKED;
  case ERRGOTO_SLEWING:         return LX200_ERRGOTO_BUSY;
  case ERRGOTO_LIMITS:          return LX200_ERRGOTO_LIMITS;
  case ERRGOTO_GUIDINGBUSY:     return LX200_ERRGOTO_BUSY;
  case ERRGOTO_ABOVEOVERHEAD:   return LX200_TARGETABOVEOVERHEAD;
  case ERRGOTO_MOTOR_FAULT:     return LX200_ERR_MOTOR_FAULT;
  case ERRGOTO_ALT:             return LX200_ERR_ALT;
  case ERRGOTO_LIMIT_SENSE:     return LX200_ERR_LIMIT_SENSE;
  case ERRGOTO_AXIS1:           return LX200_ERR_AXIS1;
  case ERRGOTO_AXIS2:           return LX200_ERR_AXIS2;
  case ERRGOTO_UNDER_POLE:      return LX200_ERR_UNDER_POLE;
  case ERRGOTO_MERIDIAN:        return LX200_ERR_MERIDIAN;
  default:                      return LX200_ERRGOTO_UNKOWN;
  }
}

/// Select a character based on NAV mode (sync, goto, pushto).
inline char navSelectChar(NAV mode, char syncChar, char gotoChar, char pushtoChar)
{
  switch (mode)
  {
  case NAV_SYNC:   return syncChar;
  case NAV_GOTO:   return gotoChar;
  case NAV_PUSHTO: return pushtoChar;
  }
  return 0;
}

/// Convert axis number (1 or 2) to the protocol character ('R' or 'D').
inline char axisChar(uint8_t axis)
{
  return axis == 1 ? 'R' : 'D';
}
