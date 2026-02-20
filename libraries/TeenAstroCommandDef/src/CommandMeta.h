/*
 * CommandMeta.h - Wire format metadata for TeenAstro LX200 protocol
 *
 * Provides:
 *   - Cmd:: namespace with command-letter constants
 *   - getReplyType(): the single source of truth that maps any LX200
 *     command string to its expected reply type.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include "CommandEnums.h"
#include <string.h>

// ---------------------------------------------------------------------------
//  Command lead characters (first character after ':')
//
//  These are the canonical protocol constants shared by client and server.
// ---------------------------------------------------------------------------
namespace Cmd {
  constexpr char RESET       = '$';   // :$x#  Reset / reboot / init
  constexpr char ACK         = 6;     // <ACK> Mount type (LX200)
  constexpr char ALIGNMENT   = 'A';   // :Ax#  Alignment
  constexpr char RETICULE    = 'B';   // :B+#  Reticule brightness
  constexpr char SYNC        = 'C';   // :Cx#  Sync
  constexpr char DISTANCE    = 'D';   // :D#   Distance bars
  constexpr char ENCODER     = 'E';   // :Ex#  Encoder / push-to
  constexpr char FOCUSER     = 'F';   // :Fx#  Focuser
  constexpr char GET         = 'G';   // :Gx#  Get (LX200 + :GXnn# TeenAstro)
  constexpr char GNSS        = 'g';   // :gx#  GNSS sync
  constexpr char HOME_PARK   = 'h';   // :hx#  Home / park
  constexpr char MOVE        = 'M';   // :Mx#  Move / slew / goto
  constexpr char HALT        = 'Q';   // :Qx#  Halt
  constexpr char RATE        = 'R';   // :Rx#  Slew rate
  constexpr char SET         = 'S';   // :Sx#  Set (LX200 + :SXnnn# TeenAstro)
  constexpr char TRACKING    = 'T';   // :Tx#  Tracking
  constexpr char PRECISION   = 'U';   // :U#   Precision toggle
  constexpr char SITE        = 'W';   // :Wn#  Site select
}

// ---------------------------------------------------------------------------
//  getReplyType() -- authoritative protocol table
// ---------------------------------------------------------------------------

/// Determine the expected reply type for a given LX200 command string.
///
/// The command must include the leading ':' and trailing '#'.
/// Returns CMDR_INVALID for unrecognised commands.
///
/// This is the authoritative protocol table: keep it in sync whenever a
/// command is added or changed in the main unit firmware.
inline CMDREPLY getReplyType(const char* command)
{
  // ACK (0x06) is a special single-byte command
  if ((command[0] == Cmd::ACK) && (command[1] == 0))
    return CMDR_SHORT;

  if (command[0] != ':')
    return CMDR_INVALID;

  switch (command[1])
  {
  // ---- A  Alignment ---------------------------------------------------
  case Cmd::ALIGNMENT:
    if (strchr("*0123456789CWA", command[2])) return CMDR_SHORT_BOOL;
    if (strchr("E", command[2]))              return CMDR_LONG;
    return CMDR_INVALID;

  // ---- B  Reticule brightness ----------------------------------------
  case Cmd::RETICULE:
    if (strchr("+-", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- C  Sync -------------------------------------------------------
  case Cmd::SYNC:
    if (strchr("AMU", command[2])) return CMDR_LONG;
    if (strchr("S", command[2]))   return CMDR_NO;
    return CMDR_INVALID;

  // ---- D  Distance bars ----------------------------------------------
  case Cmd::DISTANCE:
    if (strchr("#", command[2])) return CMDR_LONG;
    return CMDR_INVALID;

  // ---- E  Encoder / Push-to ------------------------------------------
  case Cmd::ENCODER:
    if (strchr("ACD", command[2])) return CMDR_LONG;
    if (command[2] == 'M' && strchr("ASUQ", command[3])) return CMDR_SHORT;
    return CMDR_INVALID;

  // ---- F  Focuser -----------------------------------------------------
  case Cmd::FOCUSER:
    if (strchr("+-gGPQsS$!", command[2]))                    return CMDR_NO;
    if (strchr("OoIi:012345678cCmrW", command[2]))           return CMDR_SHORT_BOOL;
    if (strchr("x?~MV", command[2]))                         return CMDR_LONG;
    return CMDR_INVALID;

  // ---- g  GNSS --------------------------------------------------------
  case Cmd::GNSS:
    if (strchr("ts", command[2])) return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- G  Get ---------------------------------------------------------
  case Cmd::GET:
    if (strchr("AaCcDdefgGhLMNOPmnoRrSTtVXWZ", command[2]))
      return CMDR_LONG;
    return CMDR_INVALID;

  // ---- h  Home / Park -------------------------------------------------
  case Cmd::HOME_PARK:
    if (strchr("F", command[2]))       return CMDR_NO;
    if (strchr("BbCOPQRS", command[2])) return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- M  Move / Slew -------------------------------------------------
  case Cmd::MOVE:
    if (strchr("ewnsg", command[2]))    return CMDR_NO;
    if (strchr("SAUF?", command[2]))    return CMDR_SHORT;
    if (strchr("12@", command[2]))      return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- Q  Halt --------------------------------------------------------
  case Cmd::HALT:
    if (strchr("#ewns", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- R  Rate --------------------------------------------------------
  case Cmd::RATE:
    if (strchr("GCMS01234", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- S  Set ---------------------------------------------------------
  case Cmd::SET:
    if (strchr("!aBCedgGhLmMnNoOrtTUXz", command[2])) return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- T  Tracking ----------------------------------------------------
  case Cmd::TRACKING:
    if (strchr("R+-TSLQ", command[2]))   return CMDR_NO;
    if (strchr("ed012", command[2]))     return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- U  Precision ---------------------------------------------------
  case Cmd::PRECISION:
    if (strchr("#", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- W  Site --------------------------------------------------------
  case Cmd::SITE:
    if (strchr("0123", command[2])) return CMDR_NO;
    if (strchr("?", command[2]))    return CMDR_LONG;
    return CMDR_INVALID;

  // ---- $  Reset -------------------------------------------------------
  case Cmd::RESET:
    if (strchr("$!", command[2]))  return CMDR_NO;
    if (strchr("X", command[2]))   return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  default:
    return CMDR_INVALID;
  }
}
