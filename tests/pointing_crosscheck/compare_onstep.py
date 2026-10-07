"""TeenAstro vs OnStep GeoAlign on the same Wallace observations.

TeenAstro: pointing_fit.exe (exact HeadGeom + T).
OnStep: transcribed autoModel search (altCor/azmCor/doCor/pdCor + indices)
from OnStepX Align.ref.cpp, rigid + polar terms only (no flex/harmonics).

True RMS is the sky residual on N equidistributed Alt/Az points (default 1000).
Section B uses Wallace tan/sec with a very small pole; score is that grid RMS.

Usage (repo root):
  py -3 tests/pointing_crosscheck/compare_onstep.py
"""
from __future__ import annotations

import math
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tpoint_eq import ARCMIN, DEG, Terms, mount_for_true, sky_sep, true_for_mount  # noqa: E402
from run_crosscheck import (  # noqa: E402
    CASES, LAT, STARS, compile_fitter, parse_fit, run_fitter, star_rows, terms_of,
)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ARCSEC = math.pi / (180.0 * 3600.0)
DEG180 = math.pi
DEG360 = 2.0 * math.pi
N_GRID = 1000
MIN_ALT = 15.0
MAX_ALT = 85.0


def wrap180(a):
    while a > DEG180:
        a -= DEG360
    while a < -DEG180:
        a += DEG360
    return a


def onstep_correct(a1, a2, pier, sf, deo, pd, pz, pe):
    """OnStep GeoAlign::correct, rigid+polar only. Returns (da1, da2)."""
    cos_a2 = math.cos(a2)
    sin_a2 = math.sin(a2)
    tan_a2 = sin_a2 / cos_a2 if abs(cos_a2) > 1e-12 else 0.0
    sin_a1 = math.sin(a1)
    cos_a1 = math.cos(a1)
    doh = deo * sf * (1.0 / cos_a2) * pier if abs(cos_a2) > 1e-12 else 0.0
    pdh = -pd * sf * tan_a2 * pier
    a1r = (-pz * sf * cos_a1 * tan_a2 + pe * sf * sin_a1 * tan_a2 + doh + pdh)
    a2r = (+pz * sf * sin_a1 + pe * sf * cos_a1)
    return a1r, a2r


def onstep_search(stars, include_cone=True):
    """Coarse-to-fine grid search mirroring GeoAlign::autoModel for GEM EQ."""
    n = len(stars)
    ohe = 0.0
    for s in stars:
        ohe += wrap180(s["actual_h"] - s["mount_h"])
    ohe /= n
    best = dict(
        dist=3600.0 * 180.0,
        deo=0.0, pd=0.0, pz=0.0, pe=0.0,
        ode=0.0, ohe=round(ohe / ARCSEC),
    )
    best["ohw"] = best["ohe"]
    Do = 1 if (include_cone and n > 2) else 0

    schedule = [
        (16384, 0, 0, 1, 1, 1, 1),
        (8192, Do, 0, 1, 1, 1, 1),
        (4096, Do, 0, 1, 1, 1, 1),
        (2048, Do, 0, 1, 1, 1, 1),
        (1024, Do, 0, 1, 1, 1, 1),
        (512, Do, 0, 1, 1, 1, 1),
        (256, Do, 1, 1, 1, 1, 1),
        (128, Do, 1, 1, 1, 1, 1),
        (64, Do, 1, 1, 1, 1, 1),
        (32, Do, 1, 1, 1, 1, 1),
        (16, Do, 1, 1, 1, 1, 1),
    ]

    for sf, p1, p2, p3, p4, p8, p9 in schedule:
        sf1 = sf * ARCSEC
        deo_m = -p1 + int(round(best["deo"] / sf))
        deo_p = p1 + int(round(best["deo"] / sf))
        pd_m = -p2 + int(round(best["pd"] / sf))
        pd_p = p2 + int(round(best["pd"] / sf))
        pz_m = -p3 + int(round(best["pz"] / sf))
        pz_p = p3 + int(round(best["pz"] / sf))
        pe_m = -p4 + int(round(best["pe"] / sf))
        pe_p = p4 + int(round(best["pe"] / sf))
        od_m = -p8 + int(round(best["ode"] / sf))
        od_p = p8 + int(round(best["ode"] / sf))
        oh_m = -p9 + int(round(best["ohe"] / sf))
        oh_p = p9 + int(round(best["ohe"] / sf))

        for deo in range(deo_m, deo_p + 1):
            for pd in range(pd_m, pd_p + 1):
                for pz in range(pz_m, pz_p + 1):
                    for pe in range(pe_m, pe_p + 1):
                        for ode_i in range(od_m, od_p + 1):
                            for ohe_i in range(oh_m, oh_p + 1):
                                ode = ode_i * sf1
                                ohe_r = ohe_i * sf1
                                sum_h = 0.0
                                sum_d = 0.0
                                for s in stars:
                                    side = s["side"]
                                    ma1 = s["mount_h"] + ohe_r
                                    ma2 = s["mount_d"] + (ode if side > 0 else -ode)
                                    a1r, a2r = onstep_correct(
                                        ma1, ma2, side, sf1, deo, pd, pz, pe)
                                    dh = wrap180(s["actual_h"] - (ma1 - a1r))
                                    dd = s["actual_d"] - (ma2 - a2r)
                                    sum_h += (dh * math.cos(s["actual_d"])) ** 2
                                    sum_d += dd ** 2
                                a = math.sqrt(sum_h / max(1, n - 1))
                                b = math.sqrt(sum_d / max(1, n - 1))
                                dist = math.sqrt(a * a + b * b)
                                if dist < best["dist"]:
                                    best["dist"] = dist
                                    best["deo"] = deo * sf
                                    best["pd"] = pd * sf
                                    best["pz"] = pz * sf
                                    best["pe"] = pe * sf
                                    best["ode"] = ode / ARCSEC if p8 else best["pe"] / 2.0
                                    best["odw"] = -best["ode"]
                                    best["ohe"] = ohe_i * sf if p9 else best["ohe"]
                                    best["ohw"] = best["ohe"]

    return dict(
        CH_as=best["deo"],
        NP_as=best["pd"],
        MA_as=best["pz"],
        ME_as=best["pe"],
        ID_as=best["ode"],
        IH_as=best["ohe"],
        rms_as=best["dist"] / ARCSEC,
        deo=best["deo"] * ARCSEC,
        pd=best["pd"] * ARCSEC,
        pz=best["pz"] * ARCSEC,
        pe=best["pe"] * ARCSEC,
        ode=best["ode"] * ARCSEC,
        ohe=best["ohe"] * ARCSEC,
    )


def wallace_obs(case):
    terms = terms_of(case)
    rows = []
    for ha_d, dec_d, flip, name in STARS:
        ha, dec = ha_d * DEG, dec_d * DEG
        side = -1.0 if flip else 1.0
        sided = Terms(ih=terms.ih, id_=terms.id_ * side, ch=terms.ch * side,
                      np_=terms.np_ * side, me=terms.me, ma=terms.ma)
        mha, mdec = mount_for_true(ha, dec, sided)
        rows.append(dict(
            name=name, side=side,
            actual_h=ha, actual_d=dec,
            mount_h=mha, mount_d=mdec,
        ))
    return rows


def altaz_grid(n, min_alt_deg, max_alt_deg):
    """Equal-area samples: sin(alt) uniform, az on a golden-angle spiral."""
    golden = math.pi * (3.0 - math.sqrt(5.0))
    z0 = math.sin(math.radians(min_alt_deg))
    z1 = math.sin(math.radians(max_alt_deg))
    lat = math.radians(LAT)
    out = []
    for i in range(n):
        z = z0 + (z1 - z0) * ((i + 0.5) / n)
        alt = math.asin(max(-1.0, min(1.0, z)))
        az = (i * golden) % (2.0 * math.pi)
        # Horizontal → equatorial at site latitude (same as Coord_HO.To_Coord_EQ).
        sin_dec = math.sin(lat) * math.sin(alt) + math.cos(lat) * math.cos(alt) * math.cos(az)
        sin_dec = max(-1.0, min(1.0, sin_dec))
        dec = math.asin(sin_dec)
        cos_dec = math.cos(dec)
        if abs(cos_dec) < 1e-12:
            continue
        sin_ha = -math.cos(alt) * math.sin(az) / cos_dec
        cos_ha = (math.sin(alt) - math.sin(lat) * sin_dec) / (math.cos(lat) * cos_dec)
        ha = math.atan2(sin_ha, cos_ha)
        if abs(dec) > math.radians(80.0):
            continue
        out.append((az, alt, ha, dec))
    return out


def onstep_mount_to_sky(model, mha, mdec, side):
    """OnStep mountToObservedPlace, rigid+polar subset. Radians.

    Index handling matches the search path: shared HA index, Dec index
    +ode east / -ode west.
    """
    ax1 = mha + model["ohe"]
    ax2 = mdec + (model["ode"] if side > 0 else -model["ode"])
    if abs(ax2) >= math.radians(89.98333333):
        return ax1, ax2
    sin_a2, cos_a2 = math.sin(ax2), math.cos(ax2)
    sin_a1, cos_a1 = math.sin(ax1), math.cos(ax1)
    tan_a2 = sin_a2 / cos_a2
    doh = model["deo"] * (1.0 / cos_a2) * side
    pdh = -model["pd"] * tan_a2 * side
    ax1c = -model["pz"] * cos_a1 * tan_a2 + model["pe"] * sin_a1 * tan_a2
    ax2c = +model["pz"] * sin_a1 + model["pe"] * cos_a1
    return ax1 - (ax1c + pdh + doh), ax2 - ax2c


def onstep_grid_rms(model, truth_terms, n=N_GRID):
    """Sky residual RMS of recovered OnStep model on the Alt/Az grid."""
    pts = altaz_grid(n, MIN_ALT, MAX_ALT)
    sum_sq = 0.0
    worst = 0.0
    used = 0
    for _az, _alt, ha, dec in pts:
        flip = ha < 0.0
        side = -1.0 if flip else 1.0
        sided = Terms(
            ih=truth_terms.ih, id_=truth_terms.id_ * side,
            ch=truth_terms.ch * side, np_=truth_terms.np_ * side,
            me=truth_terms.me, ma=truth_terms.ma,
        )
        mha, mdec = mount_for_true(ha, dec, sided)
        if abs(mdec) > math.radians(89.0):
            continue
        pha, pdec = onstep_mount_to_sky(model, mha, mdec, side)
        sep = sky_sep(ha, dec, pha, pdec)
        sum_sq += sep * sep
        worst = max(worst, sep)
        used += 1
    rms = math.sqrt(sum_sq / used) if used else 0.0
    return dict(n=used, rms_as=rms / ARCSEC, max_as=worst / ARCSEC)


def uncorrected_grid_rms(truth_terms, n=N_GRID):
    """RMS if dials are taken as sky (no model) — baseline."""
    pts = altaz_grid(n, MIN_ALT, MAX_ALT)
    sum_sq = 0.0
    worst = 0.0
    used = 0
    for _az, _alt, ha, dec in pts:
        flip = ha < 0.0
        side = -1.0 if flip else 1.0
        sided = Terms(
            ih=truth_terms.ih, id_=truth_terms.id_ * side,
            ch=truth_terms.ch * side, np_=truth_terms.np_ * side,
            me=truth_terms.me, ma=truth_terms.ma,
        )
        mha, mdec = mount_for_true(ha, dec, sided)
        sep = sky_sep(ha, dec, mha, mdec)
        sum_sq += sep * sep
        worst = max(worst, sep)
        used += 1
    rms = math.sqrt(sum_sq / used) if used else 0.0
    return dict(n=used, rms_as=rms / ARCSEC, max_as=worst / ARCSEC)


def parse_grid_fit(lines):
    fit, i = parse_fit(lines, 0)
    grid = {}
    while i < len(lines):
        parts = lines[i].split()
        if len(parts) == 2 and parts[0] in ("grid_n", "grid_rms", "grid_max"):
            grid[parts[0]] = float(parts[1]) if parts[0] != "grid_n" else int(float(parts[1]))
        i += 1
    fit["grid_n"] = grid.get("grid_n", 0)
    fit["grid_rms"] = grid.get("grid_rms", float("nan"))
    fit["grid_max"] = grid.get("grid_max", float("nan"))
    return fit


def teenastro_grid(case):
    rows, _ = star_rows(case)
    inj = terms_of(case).as_arcmin()
    body = (
        "gridrms\n%g\n%d %g %g\n%g %g %g %g %g\n%d\n%s"
        % (LAT, N_GRID, MIN_ALT, MAX_ALT,
           inj["ME"], inj["MA"], inj["CH"], inj["NP"], inj["ID"],
           len(STARS), "\n".join(rows) + "\n")
    )
    fit = parse_grid_fit(run_fitter(body))
    return dict(
        ok=fit.get("ok", False),
        ME_as=fit["eq_el"] * 3600.0,
        MA_as=fit["eq_az"] * 3600.0,
        CH_as=fit["cone"],
        NP_as=-fit["perp"],
        ID_as=fit["idx2"],
        star_rms_as=fit["rms"],
        grid_n=fit["grid_n"],
        grid_rms_as=fit["grid_rms"],
        grid_max_as=fit["grid_max"],
    )


def onstep_as_wallace(os_fit):
    return dict(
        ME_as=os_fit["ME_as"],
        MA_as=os_fit["MA_as"],
        CH_as=-os_fit["CH_as"],
        NP_as=os_fit["NP_as"],
        ID_as=os_fit["ID_as"],
        star_rms_as=os_fit["rms_as"],
        model=os_fit,
    )


def fmt(v, w=10):
    return ("%+*.2f" % (w, v)) if abs(v) < 1e6 else "%*s" % (w, "nan")


def headgeom_self_grid(d_az_deg, d_alt_deg, cone_as, perp_as, idx_as):
    """Fit HeadGeom-generated stars; true RMS on Alt/Az with same HeadGeom truth."""
    lines = [
        "selfgrid",
        "%g" % LAT,
        "%d %g %g" % (N_GRID, MIN_ALT, MAX_ALT),
        "%g %g %g %g %g %d" % (d_az_deg, d_alt_deg, cone_as, perp_as, idx_as, len(STARS)),
    ]
    for ha, dec, flip, _name in STARS:
        lines.append("%g %g %d" % (ha, dec, flip))
    fit = parse_grid_fit(run_fitter("\n".join(lines) + "\n"))
    return dict(
        ok=fit.get("ok", False),
        star_rms_as=fit["rms"],
        grid_n=fit["grid_n"],
        grid_rms_as=fit["grid_rms"],
        grid_max_as=fit["grid_max"],
        ME_as=-fit["pole_alt"] * 3600.0,
        CH_as=fit["cone"],
        NP_as=-fit["perp"],
        ID_as=fit["idx2"],
    )


def plumbed_grid(case):
    """ME/MA as plumbed pole tilts + HeadGeom CH/NP/ID; fit + Alt/Az grid RMS."""
    inj = terms_of(case).as_arcmin()
    lines = [
        "plumbedgrid",
        "%g" % LAT,
        "%d %g %g" % (N_GRID, MIN_ALT, MAX_ALT),
        "%g %g %g %g %g" % (inj["ME"], inj["MA"], inj["CH"], inj["NP"], inj["ID"]),
        "%d" % len(STARS),
    ]
    for ha, dec, flip, _name in STARS:
        lines.append("%g %g %d" % (ha, dec, flip))
    raw = run_fitter("\n".join(lines) + "\n")
    fit = parse_grid_fit(raw)
    export = {}
    for line in raw:
        parts = line.split()
        if len(parts) == 2 and parts[0] in ("export_ME", "export_MA"):
            export[parts[0]] = float(parts[1])
    return dict(
        ok=fit.get("ok", False),
        star_rms_as=fit["rms"],
        grid_n=fit["grid_n"],
        grid_rms_as=fit["grid_rms"],
        grid_max_as=fit["grid_max"],
        CH_as=fit["cone"],
        NP_as=-fit["perp"],
        ID_as=fit["idx2"],
        export_ME_as=export.get("export_ME", float("nan")),
        export_MA_as=export.get("export_MA", float("nan")),
    )


def _case_as(name, title, me_as, ma_as, ch_as=0.0, np_as=0.0, id_as=0.0):
    """Case dict from arcseconds (degrees stored for terms_of)."""
    return dict(
        name=name, title=title,
        me=me_as / 3600.0, ma=ma_as / 3600.0, ch=ch_as / 3600.0,
        np=np_as / 3600.0, ident=id_as / 3600.0,
        noise=0.0, home1=0.0, tight=True,
    )


def one_term_cases():
    """Single Wallace term at a few arcminutes (plumbed / legacy grid checks)."""
    out = []
    for key, label, me, ma, ch, np_, ident in (
        ("me", "ME +180\"", 3 / 60, 0, 0, 0, 0),
        ("ma", "MA +180\"", 0, 3 / 60, 0, 0, 0),
        ("ch", "CH +180\"", 0, 0, 3 / 60, 0, 0),
        ("np", "NP -90\"", 0, 0, 0, -1.5 / 60, 0),
        ("ident", "ID +42\"", 0, 0, 0, 0, 0.7 / 60),
    ):
        out.append(dict(
            me=me, ma=ma, ch=ch, np=np_, ident=ident,
            noise=0.0, home1=0.0, tight=False,
            name="one_%s" % key, title="Wallace one-term: %s" % label))
    return out


def wallace_star_cases():
    """Wallace tan/sec, very small pole — star-RMS bench only."""
    return [
        _case_as("tiny", "Tiny cocktail", 30, 20, 30, -15, 7),
        _case_as("one_me", "ME +30\"", 30, 0),
        _case_as("one_ma", "MA +30\"", 0, 30),
        _case_as("one_ch", "CH +30\"", 0, 0, 30),
        _case_as("one_np", "NP -15\"", 0, 0, 0, -15),
        _case_as("one_id", "ID +7\"", 0, 0, 0, 0, 7),
    ]


def uncorrected_star_rms(case):
    """Sky residual of raw Wallace dials vs true star positions, arcseconds."""
    _, packed = star_rows(case)
    sum_sq = 0.0
    for s in packed:
        sep = sky_sep(s["ha"], s["dec"], s["mha"], s["mdec"])
        sum_sq += sep * sep
    n = len(packed)
    return (math.sqrt(sum_sq / n) / ARCSEC) if n else 0.0


def teenastro_star(case):
    """Fit TeenAstro on Wallace stars; report star RMS only."""
    rows, _ = star_rows(case)
    body = "fit\n%g\n%d\n%s\n" % (LAT, len(STARS), "\n".join(rows))
    fit, _ = parse_fit(run_fitter(body), 0)
    return dict(
        ok=fit.get("ok", False),
        ME_as=fit["eq_el"] * 3600.0,
        MA_as=fit["eq_az"] * 3600.0,
        CH_as=fit["cone"],
        NP_as=-fit["perp"],
        ID_as=fit["idx2"],
        star_rms_as=fit["rms"],
    )


def inj_as(case):
    """Injected terms in arcseconds."""
    a = terms_of(case).as_arcmin()
    return {k: a[k] * 60.0 for k in ("ME", "MA", "CH", "NP", "ID")}


def fmt_inj(inj):
    return "ME %+g\"  MA %+g\"  CH %+g\"  NP %+g\"  ID %+g\"" % (
        inj["ME"], inj["MA"], inj["CH"], inj["NP"], inj["ID"])


def case_title_as(case):
    """Section titles with angle extras in arcseconds (not °/′)."""
    if case["name"].startswith("one_") or case["name"] == "small":
        return case["title"]
    noise_as = float(case.get("noise", 0.0)) * 60.0
    home_as = float(case.get("home1", 0.0)) * 3600.0
    inj = inj_as(case)
    bits = []
    if abs(inj["ME"]) >= 3600.0:
        bits.append("pole %+g\"" % inj["ME"])
    if abs(inj["CH"]) >= 600.0:
        bits.append("cone %+g\"" % inj["CH"])
    if abs(home_as) > 0.0:
        bits.append("home %+g\"" % home_as)
    if abs(noise_as) > 0.0:
        bits.append("centering %+g\"" % noise_as)
    else:
        bits.append("no centering error")
    return ", ".join(bits) if bits else case["title"]


def print_quality(label, star_rms, grid_rms, grid_max, n, verdict=""):
    tag = ("  " + verdict) if verdict else ""
    print("  %-14s  %s %s %s %6d%s" % (
        label, fmt(star_rms), fmt(grid_rms), fmt(grid_max), n, tag))


def main():
    compile_fitter()
    print("All angles in arcseconds. True RMS on %d equal-area Alt/Az points "
          "(alt %.0f..%.0f deg)." % (N_GRID, MIN_ALT, MAX_ALT))
    print("lat %.1f deg  align stars %d" % (LAT, len(STARS)))
    print()

    # ------------------------------------------------------------------
    # A. HeadGeom native math — must be near zero on the grid
    # ------------------------------------------------------------------
    print("=" * 72)
    print("A. HeadGeom + T  (native inject = native fit)  PASS if gridRMS < 1\"")
    print("=" * 72)
    print("  %-18s  starRMS\"   gridRMS\"   gridMax\"   n" % "model")
    # dAz/dAlt in degrees; cone/perp/idx in arcseconds (internal HeadGeom).
    hg_cases = [
        ("ideal", 0.0, 0.0, 0.0, 0.0, 0.0),
        ("ME +180\"", 0.0, -180.0 / 3600.0, 0.0, 0.0, 0.0),
        ("MA-tilt +120\"", 120.0 / 3600.0 / math.cos(math.radians(LAT)), 0.0, 0.0, 0.0, 0.0),
        ("CH +180\"", 0.0, 0.0, 180.0, 0.0, 0.0),
        ("NP -90\"", 0.0, 0.0, 0.0, 90.0, 0.0),  # internal perp = -Wallace NP
        ("ID +42\"", 0.0, 0.0, 0.0, 0.0, 42.0),
        ("cocktail", 120.0 / 3600.0 / math.cos(math.radians(LAT)),
         -180.0 / 3600.0, 180.0, 90.0, 42.0),
        ("pole +36000\"+CH", 0.0, -10.0, 180.0, 90.0, 42.0),
    ]
    hg_fail = 0
    for name, daz, dalt, cone, perp, idx in hg_cases:
        r = headgeom_self_grid(daz, dalt, cone, perp, idx)
        ok = r["ok"] and r["grid_rms_as"] < 1.0
        if not ok:
            hg_fail += 1
        print_quality(name, r["star_rms_as"], r["grid_rms_as"], r["grid_max_as"],
                      r["grid_n"], "PASS" if ok else "FAIL")
    print("  HeadGeom self: %d failed\n" % hg_fail)

    # ------------------------------------------------------------------
    # B. Wallace tan/sec, very small pole — score = 1000-point grid RMS
    # ------------------------------------------------------------------
    print("=" * 72)
    print("B. Wallace tan/sec (pole ~30\")  score = grid RMS on %d points" % N_GRID)
    print("=" * 72)
    for case in wallace_star_cases():
        inj = inj_as(case)
        terms = terms_of(case)
        print("-- %s --" % case["title"])
        print("  inject %s" % fmt_inj(inj))
        base = uncorrected_grid_rms(terms)
        ta = teenastro_grid(case)
        os_raw = onstep_search(wallace_obs(case), include_cone=True)
        os_w = onstep_as_wallace(os_raw)
        os_grid = onstep_grid_rms(os_raw, terms)
        print("  %-14s  starRMS\"   gridRMS\"   gridMax\"   n" % "model")
        print_quality("no model", base["rms_as"], base["rms_as"], base["max_as"], base["n"])
        print_quality("TeenAstro", ta["star_rms_as"], ta["grid_rms_as"],
                      ta["grid_max_as"], ta["grid_n"])
        print_quality("OnStep", os_w["star_rms_as"], os_grid["rms_as"],
                      os_grid["max_as"], os_grid["n"])
        print("  recovered TA   ME %+g\"  MA %+g\"  CH %+g\"  NP %+g\"  ID %+g\"" % (
            ta["ME_as"], ta["MA_as"], ta["CH_as"], ta["NP_as"], ta["ID_as"]))
        print("  recovered OS   ME %+g\"  MA %+g\"  CH %+g\"  NP %+g\"  ID %+g\"" % (
            os_w["ME_as"], os_w["MA_as"], os_w["CH_as"], os_w["NP_as"], os_w["ID_as"]))
        if base["rms_as"] > 1e-6:
            ta_gain = 100.0 * (1.0 - ta["grid_rms_as"] / base["rms_as"])
            os_gain = 100.0 * (1.0 - os_grid["rms_as"] / base["rms_as"])
            print("  grid improvement vs no-model: TeenAstro %+.0f%%  OnStep %+.0f%%" % (
                ta_gain, os_gain))
        # Informational: report gridRMS; no hard PASS (MA tan/sec is not HeadGeom).
        print()

    # ------------------------------------------------------------------
    # C. Plumbed Wallace transfer (ME/MA on T, CH/NP/ID in HeadGeom)
    # ------------------------------------------------------------------
    print("=" * 72)
    print("C. Plumbed ME/MA + HeadGeom CH/NP/ID  PASS if gridRMS < 1\"")
    print("=" * 72)
    print("  %-22s  starRMS\"   gridRMS\"   exportME\"  exportMA\"  n" % "case")
    plumbed_fail = 0
    plumbed_cases = [
        c for c in CASES if c["name"] in ("small", "pole10")
    ] + [c for c in one_term_cases() if c["name"] in (
        "one_me", "one_ma", "one_ch", "one_np", "one_ident")]
    for case in plumbed_cases:
        r = plumbed_grid(case)
        inj = inj_as(case)
        ok = (r["ok"] and r["grid_rms_as"] < 1.0
              and abs(r["export_ME_as"] - inj["ME"]) < 5.0
              and abs(r["export_MA_as"] - inj["MA"]) < 5.0)
        if not ok:
            plumbed_fail += 1
        print("  %-22s  %s %s %s %s %6d  %s" % (
            case["name"],
            fmt(r["star_rms_as"]), fmt(r["grid_rms_as"]),
            fmt(r["export_ME_as"]), fmt(r["export_MA_as"]),
            r["grid_n"], "PASS" if ok else "FAIL"))
    print("  Plumbed transfer failures: %d\n" % plumbed_fail)

    print("Summary:")
    print("  HeadGeom self-grid failures: %d (must be 0)" % hg_fail)
    print("  Plumbed Wallace transfer failures: %d (must be 0)" % plumbed_fail)
    print("  Section B: Wallace tan/sec grid RMS (tiny pole) is informational.")
    return 1 if (hg_fail or plumbed_fail) else 0


if __name__ == "__main__":
    sys.exit(main())

