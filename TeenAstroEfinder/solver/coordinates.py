"""
Coordinate helpers. Adapted from AstroKeith eFinder Lite Coordinates_Lite.py
(no Nexus dependency — site comes from TeenAstro :eL / :eT).
Copyright (C) 2022 Keith Venables — GPL-3.0.
"""
from __future__ import annotations

import math
from datetime import datetime, timezone
from typing import Optional, Tuple


class Coordinates:
    def __init__(self) -> None:
        self.earth = None
        self.ts = None
        try:
            from skyfield.api import load  # type: ignore

            self.planets = load("de421.bsp")
            self.earth = self.planets["earth"]
            self.ts = load.timescale()
        except Exception:
            self.planets = None

    def now(self, utc: Optional[datetime] = None):
        if self.ts is None:
            return None
        if utc is None:
            return self.ts.now()
        if utc.tzinfo is None:
            utc = utc.replace(tzinfo=timezone.utc)
        return self.ts.from_datetime(utc)

    def j2000_to_jnow(
        self,
        ra_hours_j2000: float,
        dec_deg_j2000: float,
        lat: float,
        lon_east: float,
        utc: Optional[datetime] = None,
    ) -> Tuple[float, float]:
        """Return apparent RA (hours) / Dec (deg) at the observer for 'now'."""
        if self.earth is None or self.ts is None:
            return ra_hours_j2000, dec_deg_j2000
        from skyfield.api import Star, wgs84  # type: ignore

        t = self.now(utc)
        solved = Star(ra_hours=ra_hours_j2000, dec_degrees=dec_deg_j2000)
        location = self.earth + wgs84.latlon(lat, lon_east)
        app = location.at(t).observe(solved).apparent()
        ra, dec, _ = app.radec(t)
        return ra.hours, dec.degrees

    def conv_altaz(self, lat: float, lon_east: float, ra: float, dec: float) -> Tuple[float, float]:
        rad = math.pi / 180
        if self.ts is not None:
            t = self.ts.now()
            lst = t.gmst + lon_east / 15
        else:
            lst = 0.0
        ra_deg = ra * 15
        lstd = lst * 15
        lha = (lstd - ra_deg + 360) % 360
        x = math.cos(lha * rad) * math.cos(dec * rad)
        y = math.sin(lha * rad) * math.cos(dec * rad)
        z = math.sin(dec * rad)
        colat = 90 - lat
        xhor = x * math.cos(colat * rad) - z * math.sin(colat * rad)
        yhor = y
        zhor = x * math.sin(colat * rad) + z * math.cos(colat * rad)
        az = math.atan2(yhor, xhor) * (180 / math.pi) + 180
        alt = math.asin(zhor) * (180 / math.pi)
        return alt, az

    @staticmethod
    def dd2dms(dd: float) -> str:
        is_positive = dd >= 0
        dd = abs(dd)
        minutes, seconds = divmod(dd * 3600, 60)
        degrees, minutes = divmod(minutes, 60)
        sign = "+" if is_positive else "-"
        return "%s%02d:%02d:%02d" % (sign, degrees, minutes, seconds)

    @staticmethod
    def hh2dms(hh: float) -> str:
        minutes, seconds = divmod(hh * 3600, 60)
        hours, minutes = divmod(minutes, 60)
        return "%02d:%02d:%02d" % (hours, minutes, seconds)
