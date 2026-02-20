/*
 * LX200Navigation.cpp - Higher-level navigation helpers for TeenAstro
 *
 * These functions depend on the Ephemeris and TeenAstroCatalog libraries
 * and therefore live separately from LX200Client (which is pure serial I/O).
 *
 * Copyright (C) 2024 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 */
#include <Arduino.h>
#include <Ephemeris.h>
#include <TeenAstroCatalog.h>
#include <TeenAstroMath.h>
#include "LX200Navigation.h"

// ---------------------------------------------------------------------------

LX200RETURN SyncGotoLX200(LX200Client& client, NAV mode, float& Ra, float& Dec, double epoch)
{
  unsigned int day, month, year;
  if (client.getUTCDate(day, month, year) != LX200_VALUEGET)
    return LX200_GETVALUEFAILED;
  EquatorialCoordinates coo;
  coo.ra = Ra;
  coo.dec = Dec;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(
    coo, epoch, day, month, year, 0, 0, 0);
  return client.syncGoto(mode, cooNow.ra, cooNow.dec);
}

LX200RETURN SyncGotoCatLX200(LX200Client& client, NAV mode)
{
  int epoch;
  unsigned int day, month, year;
  if (client.getUTCDate(day, month, year) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (!cat_mgr.isStarCatalog() && !cat_mgr.isDsoCatalog())
    return LX200_ERRGOTO_UNKOWN;
  EquatorialCoordinates coo;
  coo.ra = cat_mgr.rah();
  coo.dec = cat_mgr.dec();
  epoch = cat_mgr.epoch();
  if (epoch == 0) return LX200_GETVALUEFAILED;
  EquatorialCoordinates cooNow;
  cooNow = Ephemeris::equatorialEquinoxToEquatorialJNowAtDateAndTime(
    coo, epoch, day, month, year, 0, 0, 0);
  return client.syncGoto(mode, cooNow.ra, cooNow.dec);
}

LX200RETURN SyncGotoPlanetLX200(LX200Client& client, NAV mode, unsigned short objSys)
{
  unsigned int day, month, year, hour, minute, second;
  double degreeLat, degreeLong;

  if (client.getUTCDate(day, month, year) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (client.getUTCTime(hour, minute, second) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (client.getLongitude(degreeLong) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;
  if (client.getLatitude(degreeLat) == LX200_GETVALUEFAILED)
    return LX200_GETVALUEFAILED;

  Ephemeris Eph;
  Eph.flipLongitude(true);
  Eph.setLocationOnEarth((float)degreeLat, 0, 0, (float)degreeLong, 0, 0);
  SolarSystemObjectIndex objI = static_cast<SolarSystemObjectIndex>(objSys);
  SolarSystemObject obj = Eph.solarSystemObjectAtDateAndTime(
    objI, day, month, year, hour, minute, second);
  return client.syncGoto(mode, obj.equaCoordinates.ra, obj.equaCoordinates.dec);
}

LX200RETURN SyncSelectedStarLX200(LX200Client& client, unsigned short alignSelectedStar)
{
  if (alignSelectedStar >= 0)
    return SyncGotoCatLX200(client, NAV_GOTO);
  return LX200_ERRGOTO_UNKOWN;
}
