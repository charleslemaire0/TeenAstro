/*
 * CommandConstants.h - Protocol constants for TeenAstro LX200-style serial protocol.
 *
 * The Cmd:: namespace and getReplyType() are now defined in the shared
 * TeenAstroCommandDef library (CommandMeta.h).  This header re-exports
 * them and adds server-specific buffer sizes.
 */
#pragma once

#include <CommandMeta.h>
#include <CommandReplyLength.h>

// Command buffer limits (from protocol: : + cmd + #)
constexpr int CMD_BUFFER_LEN   = 28;
// :GXCS# returns 120 base64; :FA# focuser all-config returns 200 base64 + '#' + NUL.
constexpr int REPLY_BUFFER_LEN = 210;
// Inclusive of ':' through '#' in T_Serial::update (see CommandSerial.cpp). Must fit
// :SXRr,<16 hex># (23 bytes) and similar; stripped command still fits CMD_BUFFER_LEN.
constexpr int CMD_MAX_PAYLOAD  = 27;  // max stored index+1 before '#' completes frame (<= CMD_BUFFER_LEN-1)
