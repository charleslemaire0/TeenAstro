"""Wallace equatorial pointing terms, used as the source of observations.

The six geometrical terms are IH, ID, CH, NP, ME and MA, as published for
TPOINT and restated in TheSkyX Appendix L.

Two ways to realise ME/MA (they are not the same operator):

1. **Plumbed transfer (TeenAstro native).** Wallace assumes a plumbed mount:
   ME/MA are the polar-axis altitude and azimuth tilts in the local horizontal
   frame. TeenAstro stores those as `setPoleError(dAz, dAlt)` with
   `MA = dAz·cos(lat)` and `ME = -dAlt`. CH/NP/ID sit in HeadGeom. Use this
   path for inject/export that must match `:GXAa#` / `:GXAz#`.

2. **Tan/sec formula (classic TPOINT dials).** `corrections()` below. That
   linearized HA/Dec MA is *not* a pure plumbed polar-az rotation; fitting it
   with HeadGeom aliases MA into NP and raises true (Alt/Az grid) RMS. Keep it
   for OnStep / legacy formula comparisons only.

Sign convention locked by geometric_locks():

- CH  constant east-west arc on the sky. Positive CH places the true
    position west of the dial (hour angle increases to the west).
- NP  east-west arc NP * sin(dec), zero on the equator, same west sense.
- IH  hour-angle index. The east-west arc is IH * cos(dec).
- ID  declination index, the same at every hour angle.
- ME  positive means the dial's pole lies below the true pole, so on the
  meridian true dec = mount dec - ME.
- MA  hour-angle shift MA * tan(dec) on the meridian, with no dec shift there
  (tan/sec formula). Plumbed transfer uses dAz = MA/cos(lat) instead.
"""

from __future__ import annotations

import math
from dataclasses import dataclass


ARCSEC = math.pi / (180.0 * 3600.0)
ARCMIN = 60.0 * ARCSEC
DEG = 60.0 * ARCMIN


@dataclass
class Terms:
    ih: float = 0.0
    id_: float = 0.0
    ch: float = 0.0
    np_: float = 0.0
    me: float = 0.0
    ma: float = 0.0

    def as_arcmin(self):
        s = 1.0 / ARCMIN
        return {
            "IH": self.ih * s,
            "ID": self.id_ * s,
            "CH": self.ch * s,
            "NP": self.np_ * s,
            "ME": self.me * s,
            "MA": self.ma * s,
        }


def _sec(d):
    c = math.cos(d)
    if abs(c) < 1e-8:
        raise ValueError("declination too close to the pole for the tan/sec model")
    return 1.0 / c


def corrections(mount_ha, mount_dec, terms: Terms):
    """(dHA, dDec) added to the mount dials to obtain the true sky. Radians."""
    sec = _sec(mount_dec)
    tn = math.tan(mount_dec)
    dh = (terms.ih
          + terms.ch * sec
          + terms.np_ * tn
          + terms.ma * math.cos(mount_ha) * tn
          - terms.me * math.sin(mount_ha) * tn)
    dd = (terms.id_
          - terms.me * math.cos(mount_ha)
          + terms.ma * math.sin(mount_ha))
    return dh, dd


def true_for_mount(mount_ha, mount_dec, terms: Terms):
    dh, dd = corrections(mount_ha, mount_dec, terms)
    return mount_ha + dh, mount_dec + dd


def mount_for_true(true_ha, true_dec, terms: Terms, rounds=16):
    """Mount dials that TPOINT maps onto this true position."""
    ha, dec = true_ha, true_dec
    for _ in range(rounds):
        dh, dd = corrections(ha, dec, terms)
        ha, dec = true_ha - dh, true_dec - dd
    return ha, dec


def wallace_to_pole_tilt(me, ma, lat):
    """Plumbed transfer: Wallace ME/MA (radians) → setPoleError dAz, dAlt.

    MA = dAz * cos(lat), ME = -dAlt. Matches `:GXAz#` / `:GXAa#`.
    """
    c = math.cos(lat)
    d_az = (ma / c) if abs(c) > 1e-8 else 0.0
    d_alt = -me
    return d_az, d_alt


def pole_tilt_to_wallace(d_az, d_alt, lat):
    """Inverse plumbed transfer: setPoleError dAz/dAlt → Wallace ME/MA."""
    return -d_alt, d_az * math.cos(lat)


def sky_sep(ha1, dec1, ha2, dec2):
    """Great-circle angle, radians. Hour angle and dec in radians."""
    a = _vec(ha1, dec1)
    b = _vec(ha2, dec2)
    c = max(-1.0, min(1.0, a[0] * b[0] + a[1] * b[1] + a[2] * b[2]))
    return math.acos(c)


def _vec(ha, dec):
    # HA increases westward. The east-west checks use dh * cos(dec), which
    # does not depend on this embedding; the embedding only serves sky_sep.
    return (
        math.cos(dec) * math.cos(ha),
        math.cos(dec) * math.sin(ha),
        math.sin(dec),
    )


def ew_arc(dha, dec):
    """East-west arc of an hour-angle shift, radians on the sky."""
    return dha * math.cos(dec)


def geometric_locks():
    """Return a list of (name, ok, detail). These lock the transcription."""
    out = []

    def check(name, ok, detail):
        out.append((name, bool(ok), detail))

    ch = 3.0 * ARCMIN
    t = Terms(ch=ch)
    for dec_deg in (0.0, 40.0):
        dec = dec_deg * DEG
        dh, dd = corrections(0.2, dec, t)
        arc = ew_arc(dh, dec)
        check(
            "CH constant east-west arc at dec %g" % dec_deg,
            abs(arc - ch) < 1e-12 and abs(dd) < 1e-15,
            "arc %.6f arcsec, expected %.6f" % (arc / ARCSEC, ch / ARCSEC),
        )

    np_ = 2.0 * ARCMIN
    t = Terms(np_=np_)
    d1, d2 = 20.0 * DEG, 50.0 * DEG
    dh1, dd1 = corrections(0.3, d1, t)
    dh2, dd2 = corrections(0.3, d2, t)
    ratio = dh1 / dh2
    expect = math.tan(d1) / math.tan(d2)
    check(
        "NP grows as tan(dec)",
        abs(ratio - expect) < 1e-12 and abs(dd1) < 1e-15,
        "ratio %.6f, tan ratio %.6f" % (ratio, expect),
    )
    dh0, _ = corrections(0.3, 0.0, t)
    check("NP is zero on the equator", abs(dh0) < 1e-15, "dh %.3e" % dh0)

    ih = 4.0 * ARCMIN
    t = Terms(ih=ih)
    for dec_deg in (0.0, 60.0):
        dec = dec_deg * DEG
        dh, dd = corrections(-0.4, dec, t)
        check(
            "IH east-west arc scales with cos(dec) at %g" % dec_deg,
            abs(ew_arc(dh, dec) - ih * math.cos(dec)) < 1e-12 and abs(dd) < 1e-15,
            "arc %.6f arcsec" % (ew_arc(dh, dec) / ARCSEC),
        )

    id_ = -1.5 * ARCMIN
    t = Terms(id_=id_)
    for ha_deg in (0.0, 90.0, -40.0):
        dh, dd = corrections(ha_deg * DEG, 0.4, t)
        check(
            "ID is a pure dec shift at HA %g" % ha_deg,
            abs(dd - id_) < 1e-15 and abs(dh) < 1e-15,
            "dd %.6f arcsec" % (dd / ARCSEC),
        )

    me = 5.0 * ARCMIN
    t = Terms(me=me)
    dh, dd = corrections(0.0, 0.5, t)
    check(
        "ME on the meridian lowers the dial pole",
        abs(dd + me) < 1e-12 and abs(dh) < 1e-12,
        "dd %.6f arcmin (expected %.6f)" % (dd / ARCMIN, -me / ARCMIN),
    )
    dh, dd = corrections(math.pi / 2, 0.5, t)
    check(
        "ME at HA 6h is an hour-angle term",
        abs(dd) < 1e-9 and abs(dh + me * math.tan(0.5)) < 1e-9,
        "dh %.6f arcmin" % (dh / ARCMIN),
    )

    ma = 4.0 * ARCMIN
    t = Terms(ma=ma)
    dh, dd = corrections(0.0, 0.45, t)
    check(
        "MA on the meridian shifts hour angle as tan(dec)",
        abs(dd) < 1e-12 and abs(dh - ma * math.tan(0.45)) < 1e-12,
        "dh %.6f arcmin" % (dh / ARCMIN),
    )

    # Round trip at a few arcminutes, and still at a degree.
    quiet = Terms(ch=3 * ARCMIN, np_=-1.5 * ARCMIN, id_=0.7 * ARCMIN,
                  me=3 * ARCMIN, ma=2 * ARCMIN)
    wide = Terms(ch=60 * ARCMIN, np_=-1.5 * ARCMIN, id_=0.7 * ARCMIN,
                 me=10 * DEG, ma=0.0)
    for label, terms, limit in (("quiet", quiet, 0.05), ("wide", wide, 5.0)):
        worst = 0.0
        for ha_deg, dec_deg in ((-40, 15), (20, 48), (70, -10), (120, 30)):
            th, td = ha_deg * DEG, dec_deg * DEG
            mh, md = mount_for_true(th, td, terms)
            bh, bd = true_for_mount(mh, md, terms)
            worst = max(worst, sky_sep(th, td, bh, bd) / ARCSEC)
        check(
            "mount/true round trip (%s)" % label,
            worst < limit,
            "worst %.4f arcsec" % worst,
        )
    return out


def locks_ok(rows=None):
    rows = geometric_locks() if rows is None else rows
    return all(ok for _, ok, _ in rows)
