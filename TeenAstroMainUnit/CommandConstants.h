/*
 * CommandConstants.h - Protocol constants for TeenAstro LX200-style serial protocol.
 *
 * The Cmd:: namespace and getReplyType() are now defined in the shared
 * TeenAstroCommandDef library (CommandMeta.h).  This header re-exports
 * them and adds server-specific buffer sizes.
 */
#pragma once

#include <CommandMeta.h>

// Command buffer limits (from protocol: : + cmd + #)
constexpr int CMD_BUFFER_LEN   = 28;
constexpr int REPLY_BUFFER_LEN = 50;
constexpr int CMD_MAX_PAYLOAD  = 22;  // max chars between : and #
