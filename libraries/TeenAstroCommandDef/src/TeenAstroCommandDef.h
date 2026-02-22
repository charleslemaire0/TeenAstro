/*
 * TeenAstroCommandDef.h - Shared command definitions for TeenAstro LX200 protocol
 *
 * This library provides the single source of truth for the TeenAstro LX200
 * command protocol. Both the client (LX200Client) and the server (MainUnit)
 * should use these shared definitions to stay in sync.
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include "CommandEnums.h"
#include "CommandTypes.h"
#include "CommandMeta.h"
#include "CommandReplyLength.h"
#include "CommandCodec.h"
