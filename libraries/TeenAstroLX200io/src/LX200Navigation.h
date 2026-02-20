/*
 * LX200Navigation.h - Higher-level navigation helpers for TeenAstro
 *
 * These functions combine the LX200Client with Ephemeris and
 * TeenAstroCatalog to provide catalog-based sync/goto operations
 * with automatic equinox precession (epoch -> JNow).
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#pragma once

#include <Arduino.h>
#include <TeenAstroCommandDef.h>
#include "LX200Client.h"

// ---------------------------------------------------------------------------
//  Sync / Goto with equinox precession
// ---------------------------------------------------------------------------

/// Sync or goto a given RA/Dec from the specified epoch, precessed to JNow.
LX200RETURN SyncGotoLX200(LX200Client& client, NAV mode, float &Ra, float &Dec, double epoch);

/// Sync or goto the currently selected catalog object, precessed to JNow.
LX200RETURN SyncGotoCatLX200(LX200Client& client, NAV mode);

/// Sync or goto a solar system object by index, precessed to JNow.
LX200RETURN SyncGotoPlanetLX200(LX200Client& client, NAV mode, unsigned short obj);

/// Sync the currently selected alignment star (delegates to SyncGotoCatLX200).
LX200RETURN SyncSelectedStarLX200(LX200Client& client, unsigned short alignSelectedStar);
