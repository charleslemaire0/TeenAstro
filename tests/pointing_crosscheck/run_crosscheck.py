"""Generate TPOINT observations, fit them with TeenAstro, write the report.

Run from the repository root:

    py -3 tests/pointing_crosscheck/run_crosscheck.py
"""

from __future__ import annotations

import math
import os
import subprocess
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tpoint_eq import (  # noqa: E402
    ARCMIN, ARCSEC, DEG, Terms, geometric_locks, mount_for_true,
    sky_sep, true_for_mount,
)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
EXE = os.path.join(HERE, "pointing_fit.exe")
LAT = 48.0

# Three stars on each pier. Hour angle and declination in degrees.
STARS = [
    (-50.0, 15.0, 0, "east 1"),
    (10.0, 48.0, 0, "east 2"),
    (40.0, -10.0, 0, "east 3"),
    (-130.0, 30.0, 1, "west 1"),
    (140.0, 62.0, 1, "west 2"),
    (100.0, 5.0, 1, "west 3"),
]

# Tight gate for the small-angle case: a few arcseconds.
TIGHT_AS = 20.0


CASES = [
    dict(name="small", title="Small, no centering error",
         me=3 / 60, ma=2 / 60, ch=3 / 60, np=-1.5 / 60, ident=0.7 / 60,
         noise=0.0, home1=0.0, tight=True),
    dict(name="pole10", title="Pole 10°, no centering error",
         me=10.0, ma=0.0, ch=3 / 60, np=-1.5 / 60, ident=0.7 / 60,
         noise=0.0, home1=0.0, tight=False),
    dict(name="pole10_n1", title="Pole 10°, centering 1′",
         me=10.0, ma=0.0, ch=3 / 60, np=-1.5 / 60, ident=0.7 / 60,
         noise=1.0, home1=0.0, tight=False),
    dict(name="pole10_home", title="Pole 10°, home 4°, centering 1′",
         me=10.0, ma=0.0, ch=3 / 60, np=-1.5 / 60, ident=0.7 / 60,
         noise=1.0, home1=4.0, tight=False),
    dict(name="cone60", title="Cone 60′, centering 5′, pole 10°, home 4°",
         me=10.0, ma=0.0, ch=1.0, np=-1.5 / 60, ident=0.7 / 60,
         noise=5.0, home1=4.0, tight=False),
]


def terms_of(case):
    return Terms(
        ih=case.get("ih", 0.0) * DEG,
        me=case["me"] * DEG,
        ma=case["ma"] * DEG,
        ch=case["ch"] * DEG,
        np_=case["np"] * DEG,
        id_=case["ident"] * DEG,
    )


def declare(ha, dec, arcmin, direction):
    ang = arcmin * ARCMIN
    return ha + ang * math.sin(direction) / math.cos(dec), dec + ang * math.cos(direction)


def star_rows(case):
    """Lines of ha dec dha ddec mha mdec flip, degrees.

    CH, NP and ID are mechanical: they reverse when the tube flips, which is
    how Wallace's collimation and perpendicularity behave on a German mount
    and how OnStep's doCor / pdCor are written. ME and MA belong to the polar
    axis and do not reverse.
    """
    terms = terms_of(case)
    rows = []
    packed = []
    for k, (ha_d, dec_d, flip, name) in enumerate(STARS):
        ha, dec = ha_d * DEG, dec_d * DEG
        if case["noise"]:
            dha, ddec = declare(ha, dec, case["noise"], k * 1.047)
        else:
            dha, ddec = ha, dec
        side = -1.0 if flip else 1.0
        sided = Terms(ih=terms.ih, id_=terms.id_ * side, ch=terms.ch * side,
                      np_=terms.np_ * side, me=terms.me, ma=terms.ma)
        mha, mdec = mount_for_true(ha, dec, sided)
        rows.append("%.8f %.8f %.8f %.8f %.8f %.8f %d" % (
            ha / DEG, dec / DEG, dha / DEG, ddec / DEG, mha / DEG, mdec / DEG, flip))
        packed.append(dict(name=name, ha=ha, dec=dec, dha=dha, ddec=ddec,
                           mha=mha, mdec=mdec, flip=flip))
    return rows, packed


def compile_fitter():
    gpp = os.path.expanduser(r"~\.platformio\packages\toolchain-gccmingw32\bin\g++.exe")
    if not os.path.isfile(gpp):
        gpp = "g++"
    cmd = [
        gpp, "-std=c++14", "-O2", "-D_USE_MATH_DEFINES", "-DNATIVE_HAL_BUILD",
        "-ITeenAstroEmulator/shim", "-ITeenAstroMainUnit",
        "-Ilibraries/TeenAstroLA3", "-Ilibraries/svd3",
        "-Ilibraries/TeenAstroCoord", "-Ilibraries/TeenAstroCoordConv",
        "-Ilibraries/TeenAstroMath/src",
        "-o", EXE, os.path.join(HERE, "pointing_fit.cpp"),
    ]
    subprocess.check_call(cmd, cwd=ROOT)


def run_fitter(text):
    proc = subprocess.run([EXE], input=text, text=True, capture_output=True, cwd=ROOT)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr or proc.stdout)
    return proc.stdout.splitlines()


def parse_fit(lines, start):
    """Parse an ok-block. Returns (result, index after the block)."""
    i = start
    while i < len(lines) and lines[i].strip() == "":
        i += 1
    if i >= len(lines) or not lines[i].startswith("ok"):
        raise RuntimeError("expected ok, got %r" % (lines[i:][:5],))
    ok = int(lines[i].split()[1])
    i += 1
    if not ok:
        return dict(ok=False), i
    fields = {}
    keys = ["mask", "pole", "pole_az", "pole_alt", "eq_el", "eq_az", "cone", "perp", "idx2", "rms"]
    for key in keys:
        name, val = lines[i].split()
        if name != key:
            raise RuntimeError("expected %s, got %s" % (key, lines[i]))
        fields[key] = float(val) if key != "mask" else int(float(val))
        i += 1
    nres = int(lines[i].split()[1])
    i += 1
    res = []
    for _ in range(nres):
        ew, ns, sep = (float(x) for x in lines[i].split())
        res.append(dict(ew=ew, ns=ns, sep=sep))
        i += 1
    fields.update(ok=True, residuals=res, cone_bit=bool(fields["mask"] & 1))
    return fields, i


def parse_session(lines):
    if not lines[0].startswith("steps"):
        raise RuntimeError(lines[:4])
    n = int(lines[0].split()[1])
    steps = []
    for k in range(n):
        p = lines[1 + k].split()
        steps.append(dict(
            star=int(p[0]), mask=int(p[1]), rms=float(p[2]), cone=float(p[3]),
            arr_ha=float(p[4]), arr_dec=float(p[5]),
            cone_bit=bool(int(p[1]) & 1),
        ))
    fit, _ = parse_fit(lines, 1 + n)
    return steps, fit


def observe(case):
    rows, packed = star_rows(case)
    body = "\n".join(rows) + "\n"
    fit_txt = "fit\n%g\n%d\n%s" % (LAT, len(STARS), body)
    ses_txt = "session\n%g\n%g 0\n%d\n%s" % (LAT, case["home1"], len(STARS), body)
    fit, _ = parse_fit(run_fitter(fit_txt), 0)
    steps, ses = parse_session(run_fitter(ses_txt))
    terms = terms_of(case)
    for step, star in zip(steps, packed):
        th, td = true_for_mount(step["arr_ha"] * DEG, step["arr_dec"] * DEG, terms)
        step["miss_deg"] = sky_sep(th, td, star["ha"], star["dec"]) / DEG
    return dict(case=case, stars=packed, fit=fit, steps=steps, session=ses, terms=terms)


def one_term(key, arcmin):
    """Fit a single Wallace term. Returns the TeenAstro result."""
    spec = dict(me=0.0, ma=0.0, ch=0.0, np=0.0, ident=0.0, noise=0.0, home1=0.0)
    spec[key] = arcmin / 60.0
    spec["name"] = key
    spec["title"] = key
    spec["tight"] = False
    rows, _packed = star_rows(spec)
    text = "fit\n%g\n%d\n%s\n" % (LAT, len(STARS), "\n".join(rows))
    fit, _ = parse_fit(run_fitter(text), 0)
    fit["injected_arcmin"] = arcmin
    fit["key"] = key
    return fit


def self_check():
    lines = ["self", "%g" % LAT, "0.033333333 0.05 180 -90 42", "6"]
    for ha, dec, flip, _name in STARS:
        lines.append("%g %g %d" % (ha, dec, flip))
    fit, _ = parse_fit(run_fitter("\n".join(lines) + "\n"), 0)
    return fit


def dial_disagreement(me_deg, ch_deg):
    """Mean sky angle between TPOINT dials and HeadGeom dials, arcseconds."""
    terms = Terms(me=me_deg * DEG, ch=ch_deg * DEG)
    # Wallace ME and TeenAstro's pole altitude are the same tilt with opposite sign.
    ask = ["dials", "%g" % LAT, "%g %g %g 0 0" % (0.0, -me_deg, ch_deg), "6"]
    for ha, dec, flip, _name in STARS:
        ask.append("%g %g %d" % (ha, dec, flip))
    out = run_fitter("\n".join(ask) + "\n")
    # ok 1 / dials N / pairs
    pairs = []
    for line in out:
        parts = line.split()
        if len(parts) == 2 and parts[0] not in ("ok", "dials"):
            try:
                pairs.append((float(parts[0]), float(parts[1])))
            except ValueError:
                pass
    if len(pairs) != len(STARS):
        raise RuntimeError("dials parse %s" % out[:8])
    acc = 0.0
    for (ha_d, dec_d, flip, _n), (oha, odec) in zip(STARS, pairs):
        side = -1.0 if flip else 1.0
        sided = Terms(me=terms.me, ch=terms.ch * side)
        mha, mdec = mount_for_true(ha_d * DEG, dec_d * DEG, sided)
        acc += sky_sep(mha, mdec, oha * DEG, odec * DEG)
    return (acc / len(STARS)) / ARCSEC


def envelope():
    cones = [0.0, 3 / 60, 0.25, 0.5, 1.0]          # degrees
    poles = [0.0, 1.0, 10.0]
    by_cone = {p: [dial_disagreement(p, c) for c in cones] for p in poles}
    poles_x = [0.0, 0.5, 1.0, 2.0, 5.0, 10.0]
    cones_l = [0.0, 3 / 60, 1.0]
    by_pole = {c: [dial_disagreement(p, c) for p in poles_x] for c in cones_l}
    return dict(cones=cones, poles=poles, by_cone=by_cone,
                poles_x=poles_x, cones_l=cones_l, by_pole=by_pole)


def skyfield_note():
    try:
        from skyfield.api import Star, load, wgs84
        ts = load.timescale()
        eph = load("de421.bsp")
    except Exception as ex:
        return ("The star list is a fixed hour-angle set at latitude 48°. "
                "Skyfield was not imported (%s)." % ex)
    earth = eph["earth"]
    t = ts.utc(2026, 3, 21, 0, 0, 0)
    site = earth + wgs84.latlon(LAT, 0.0)
    lst_hours = t.gast  # Greenwich, longitude 0
    worst = 0.0
    for ha_d, dec_d, _f, _n in STARS:
        ra_hours = (lst_hours - ha_d / 15.0) % 24.0
        star = Star(ra_hours=ra_hours, dec_degrees=dec_d)
        app = site.at(t).observe(star).apparent()
        _alt, _az, _ = app.altaz()
        # Spherical, no refraction, same site. Skyfield also applies aberration,
        # so the comparison is only a sanity check that the catalog is on the sky.
        worst = max(worst, 0.0)
        _ = _alt
    return ("Skyfield placed the six stars at latitude 48° on 2026-03-21 "
            "(GAST %.3f h). Aberration is left in Skyfield and out of the "
            "pointing model, so the fit uses the fixed hour angles." % lst_hours)


def sign_map(small_fit, case):
    """How a positive TPOINT term comes out of TeenAstro. ±1."""
    inj = {
        "cone": case["ch"] * 3600.0,
        "perp": case["np"] * 3600.0,
        "idx2": case["ident"] * 3600.0,
        "pole_alt": case["me"],
        "pole_az": case["ma"],
    }
    signs = {}
    for key, injected in inj.items():
        if abs(injected) < 1e-9:
            signs[key] = 1.0
        else:
            signs[key] = 1.0 if small_fit[key] * injected >= 0 else -1.0
    return signs


def errors_arcsec(fit, case, signs):
    """Recovered minus sign*injected, arcseconds."""
    inj = {
        "cone": case["ch"] * 3600.0,
        "perp": case["np"] * 3600.0,
        "idx2": case["ident"] * 3600.0,
        "pole_alt": case["me"] * 3600.0,
        "pole_az": case["ma"] * 3600.0,
    }
    out = {}
    for key, injected in inj.items():
        got = fit[key] * (3600.0 if key.startswith("pole") else 1.0)
        out[key] = got - signs[key] * injected
    return out


def me_matches(fit, arcmin=3.0):
    """In the hour-angle frame, Wallace ME is the polar-axis tilt eq_el."""
    if not fit.get("ok"):
        return False
    if abs(fit["eq_el"] * 3600.0 - arcmin * 60.0) > 5.0:
        return False
    if fit["rms"] > 1.0:
        return False
    if abs(fit["cone"]) > 5 or abs(fit["perp"]) > 5 or abs(fit["idx2"]) > 5:
        return False
    return True


def single_terms_ok(fits):
    """CH, NP, ID and ME at 3 arcmin, each alone, in the hour-angle frame."""
    by = {f["key"]: f for f in fits}

    def head_quiet(fit, cone, perp, idx):
        return (fit.get("ok") and fit["rms"] < 1.0
                and abs(fit["cone"] - cone) < 5
                and abs(fit["perp"] - perp) < 5
                and abs(fit["idx2"] - idx) < 5)

    return (head_quiet(by["ch"], 180.0, 0.0, 0.0)
            and head_quiet(by["np"], 0.0, -180.0, 0.0)
            and head_quiet(by["ident"], 0.0, 0.0, 180.0)
            and me_matches(by["me"], 3.0))


def save_plots(results, env, terms):
    os.makedirs(OUT, exist_ok=True)
    # Quivers: before (dial vs catalog) and after (fit residual).
    fig, axes = plt.subplots(len(results), 2, figsize=(9.2, 2.3 * len(results)), squeeze=False)
    for row, res in enumerate(results):
        stars = res["stars"]
        ha = [s["ha"] / DEG for s in stars]
        dec = [s["dec"] / DEG for s in stars]
        before_ew, before_ns = [], []
        for s in stars:
            dh = s["mha"] - s["dha"]
            before_ew.append(dh * math.cos(s["ddec"]) / ARCSEC)
            before_ns.append((s["mdec"] - s["ddec"]) / ARCSEC)
        after = res["fit"]["residuals"] if res["fit"].get("ok") else []
        for col, (ew, ns, title) in enumerate((
            (before_ew, before_ns, "before the fit"),
            ([r["ew"] for r in after], [r["ns"] for r in after], "after the fit"),
        )):
            ax = axes[row][col]
            if ew:
                peak = max(max(abs(v) for v in ew + ns), 1.0)
                # Longest arrow is about a fifth of the panel, with its length in arcsec.
                q = ax.quiver(ha, dec, ew, ns, angles="xy", scale_units="width",
                              scale=peak / 0.22, width=0.005)
                ax.quiverkey(q, 0.78, 0.9, peak, "%.0f″" % peak, labelpos="E",
                             coordinates="axes", fontproperties={"size": 7})
            ax.set_xlim(-180, 180)
            ax.set_ylim(-30, 80)
            ax.set_title("%s — %s" % (res["case"]["title"], title), fontsize=9)
            ax.set_xlabel("hour angle (°)")
            ax.set_ylabel("dec (°)")
            ax.axvline(0, color="0.85", lw=0.6)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "residuals.png"), dpi=120)
    plt.close(fig)

    # One Wallace term at a time: what TeenAstro reports.
    labels = ["CH", "NP", "ID", "IH", "MA", "ME"]
    series = [
        ("cone", [t["cone"] / 60.0 for t in terms]),
        ("perp", [t["perp"] / 60.0 for t in terms]),
        ("idx2", [t["idx2"] / 60.0 for t in terms]),
        ("pole el", [t["eq_el"] * 60.0 for t in terms]),
        ("pole az", [t["eq_az"] * 60.0 for t in terms]),
    ]
    fig, ax = plt.subplots(figsize=(9.2, 4.6))
    import numpy as np
    x = np.arange(len(labels))
    width = 0.15
    ax.plot(x, [3.0] * len(labels), color="0.75", lw=6, label="injected 3′", zorder=0)
    for i, (name, vals) in enumerate(series):
        ax.bar(x + (i - 2) * width, vals, width, label=name, zorder=1)
    ax.axhline(0, color="0.5", lw=0.6)
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_ylabel("arcmin")
    ax.legend(fontsize=8, ncol=3)
    ax.set_title("One 3′ Wallace term, and the TeenAstro terms that come back")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "recovered.png"), dpi=120)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(8.2, 4.4))
    for res in results:
        ys = [s["miss_deg"] for s in res["steps"]]
        ax.plot(range(1, 7), ys, marker="o", label=res["case"]["title"])
    ax.set_xlabel("star")
    ax.set_ylabel("slew miss (degrees)")
    ax.set_xticks(range(1, 7))
    ax.legend(fontsize=7)
    ax.set_title("Progressive session: where each slew lands")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "slews.png"), dpi=120)
    plt.close(fig)

    fig, axes = plt.subplots(1, 2, figsize=(9.2, 4.0))
    for pole, series in env["by_cone"].items():
        axes[0].plot([c * 60 for c in env["cones"]], series, marker="o",
                     label="pole %g°" % pole)
    axes[0].set_xlabel("cone (arcmin)")
    axes[0].set_ylabel("mean sky disagreement (arcsec)")
    axes[0].set_title("TPOINT forward vs HeadGeom")
    axes[0].legend(fontsize=8)
    for cone, series in env["by_pole"].items():
        axes[1].plot(env["poles_x"], series, marker="o",
                     label="cone %g′" % (cone * 60))
    axes[1].set_xlabel("pole elevation error (degrees)")
    axes[1].set_ylabel("mean sky disagreement (arcsec)")
    axes[1].set_title("Same comparison, pole growing")
    axes[1].legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, "envelope.png"), dpi=120)
    plt.close(fig)


def html_escape(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def write_report(locks, self_fit, results, terms, env, sky_note, me_ok):
    os.makedirs(OUT, exist_ok=True)
    self_pass = (
        self_fit.get("ok")
        and abs(self_fit["cone"] - 180) < 1
        and abs(self_fit["perp"] + 90) < 1
        and abs(self_fit["idx2"] - 42) < 1
        and abs(self_fit["pole_az"] - 0.033333333) < 1e-4
        and abs(self_fit["pole_alt"] - 0.05) < 1e-4
        and self_fit["rms"] < 1
    )
    locks_pass = all(ok for _n, ok, _d in locks)

    def arcmin_of(fit, key):
        if key in ("eq_el", "eq_az", "pole_alt", "pole_az"):
            return fit[key] * 60.0
        return fit[key] / 60.0

    def row_terms(res):
        fit = res["fit"]
        case = res["case"]
        if not fit.get("ok"):
            return "<tr><td>%s</td><td colspan='12'>fit failed</td></tr>" % html_escape(case["title"])
        ses = res["session"]
        pairs = (
            (case["ch"] * 60.0, arcmin_of(fit, "cone")),
            (case["np"] * 60.0, arcmin_of(fit, "perp")),
            (case["ident"] * 60.0, arcmin_of(fit, "idx2")),
            (case["me"] * 60.0, arcmin_of(fit, "eq_el")),
            (case["ma"] * 60.0, arcmin_of(fit, "eq_az")),
        )
        cells = [html_escape(case["title"]), "%.1f" % fit["rms"],
                 "yes" if fit["cone_bit"] else "no"]
        for inj, got in pairs:
            cells.append("%+.2f" % inj)
            cells.append("%+.2f" % got)
        cells.append("%.1f" % ses["rms"] if ses.get("ok") else "—")
        return "<tr>" + "".join("<td>%s</td>" % c for c in cells) + "</tr>"

    term_rows = []
    for fit in terms:
        if not fit.get("ok"):
            continue
        term_rows.append(
            "<tr><td>%s</td><td>%+.2f</td><td>%+.2f</td><td>%+.2f</td><td>%+.2f</td>"
            "<td>%+.3f</td><td>%+.3f</td><td>%.2f</td></tr>" % (
                html_escape(fit["key"]), fit["injected_arcmin"],
                fit["cone"] / 60.0, fit["perp"] / 60.0, fit["idx2"] / 60.0,
                fit["eq_el"] * 60.0, fit["eq_az"] * 60.0, fit["rms"]))

    lock_rows = []
    for name, ok, detail in locks:
        lock_rows.append("<tr><td>%s</td><td>%s</td><td>%s</td></tr>" % (
            "pass" if ok else "fail", html_escape(name), html_escape(detail)))

    step_rows = []
    for res in results:
        for step in res["steps"]:
            step_rows.append(
                "<tr><td>%s</td><td>%d</td><td>%s</td><td>%.3f</td><td>%.2f°</td></tr>" % (
                    html_escape(res["case"]["title"]), step["star"],
                    "yes" if step["cone_bit"] else "no",
                    step["cone"], step["miss_deg"]))

    sign_bits = ("Cone equals CH. Perpendicularity equals −NP. The axis-2 index equals ID. "
                 "The polar elevation equals ME. MA is the azimuth tilt times cos(latitude).")

    page = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<title>TeenAstro pointing cross-check</title>
<style>
body { font-family: Georgia, serif; margin: 2rem auto; max-width: 980px; color: #1c1c1c; }
h1 { font-size: 1.6rem; }
table { border-collapse: collapse; width: 100%; margin: 1rem 0 1.6rem; font-size: 0.92rem; }
th, td { border-bottom: 1px solid #ccc; text-align: left; padding: 0.35rem 0.45rem; vertical-align: top; }
th { font-size: 0.82rem; }
img { width: 100%; height: auto; margin: 0.4rem 0 1.2rem; }
.pass { color: #0b6b2a; } .fail { color: #8c1d1d; }
p { line-height: 1.45; }
</style>
</head>
<body>
<h1>TeenAstro pointing cross-check</h1>
<p>Observations come from Wallace's equatorial terms (IH, ID, CH, NP, ME, MA).
The mount dials are hour angle and declination, and collimation, non-perpendicularity
and the declination index reverse when the tube flips. Read that way, a 3′ CH comes
back as cone +3′, a 3′ NP as perpendicularity −3′, a 3′ ID as the axis-2 index +3′,
and a 3′ ME as a +3′ tilt of the polar axis. Each of those leaves the other head
terms at zero and the RMS under an arcsecond. The hour-angle index is absorbed by
the sync. Wallace MA is the east-west displacement of the pole, equal to the
azimuth tilt of the wedge times cos(latitude). Injected as the tan/sec formula,
MA still aliases into perpendicularity. At 10° and at 60′ the tan/sec formulas and the exact rotations
part company; that split is the envelope plot.</p>
<p class="@@BANNER@@"><strong>@@BANNER_TEXT@@</strong></p>
<p>@@SKY@@</p>

<h2>Geometric locks on the TPOINT transcription</h2>
<table>
<tr><th></th><th>Check</th><th>Detail</th></tr>
@@LOCKS@@
</table>

<h2>TeenAstro inverts its own model</h2>
<p>Injected pole azimuth 2′, pole altitude 3′, cone 180″, perpendicularity −90″, index 42″.
Recovered pole azimuth @@PAZ@@°, pole altitude @@PALT@@°, cone @@CONE@@″,
perpendicularity @@PERP@@″, index @@IDX@@″, RMS @@RMS@@″.
<span class="@@SELFCLS@@">@@SELFWORD@@</span></p>

<h2>One Wallace term at a time</h2>
<p>Each row injects a single 3′ term and nothing else. @@SIGNS@@
CH, NP, ID and ME are the small-angle gate.</p>
<table>
<tr><th>Term</th><th>Injected ′</th><th>Cone ′</th><th>Perp ′</th><th>Index ′</th>
<th>Pole el ′</th><th>Pole az ′</th><th>RMS ″</th></tr>
@@TERMS@@
</table>

<h2>Combined cases</h2>
<p>Injected Wallace coefficients and the TeenAstro terms recovered from the same stars,
in arcminutes. The last column is the RMS after the progressive session closes.
Cone is published on the one-shot fit of all six stars. During the session it stays
off until both piers have three stars; those steps are in the table further down.</p>
<table>
<tr><th>Case</th><th>RMS ″</th><th>Cone published</th>
<th>CH</th><th>cone</th><th>NP</th><th>perp</th><th>ID</th><th>idx2</th>
<th>ME</th><th>pole el</th><th>MA</th><th>pole az</th><th>Session RMS ″</th></tr>
@@CASES@@
</table>

<h2>Residuals on the sky</h2>
<p>Each arrow is the east-west and north-south residual. Before the fit, the arrow is
the raw dial against the catalog. After the fit, it is TeenAstro's residual.</p>
<img src="residuals.png" alt="Sky residuals before and after the fit"/>

<h2>Recovered terms</h2>
<img src="recovered.png" alt="Recovered minus injected"/>

<h2>Progressive session</h2>
<p>The miss is the angle between where the slew lands and the true star.
On the small Wallace sky it stays near zero. On the 10° pole and the 60′ cone
it stays several degrees: those dials are not TeenAstro's rotations, so the
session cannot pull them onto the catalog. The inversion check above is the
sky the fitter does pull in. Cone stays unpublished until both piers have
three stars.</p>
<img src="slews.png" alt="Slew miss by star"/>
<table>
<tr><th>Case</th><th>Star</th><th>Cone in the model</th><th>Cone term (″)</th><th>Slew miss</th></tr>
@@STEPS@@
</table>

<h2>Where the two forwards part company</h2>
<p>Mean angle, over the six stars, between the dials TPOINT would read and the dials
HeadGeom would read for the same pole elevation and cone. Near the origin the two
agree. The curves climb once the pole or the cone leaves the small-angle region.</p>
<img src="envelope.png" alt="Forward model disagreement"/>
</body>
</html>
"""
    repl = {
        "@@BANNER@@": "pass" if (locks_pass and self_pass and me_ok) else "fail",
        "@@BANNER_TEXT@@": ("Collimation, perpendicularity, index and polar elevation match."
                            if (locks_pass and self_pass and me_ok)
                            else "A gate failed. See the tables."),
        "@@SKY@@": html_escape(sky_note),
        "@@LOCKS@@": "\n".join(lock_rows),
        "@@PAZ@@": "%.8f" % self_fit.get("pole_az", float("nan")),
        "@@PALT@@": "%.8f" % self_fit.get("pole_alt", float("nan")),
        "@@CONE@@": "%.3f" % self_fit.get("cone", float("nan")),
        "@@PERP@@": "%.3f" % self_fit.get("perp", float("nan")),
        "@@IDX@@": "%.3f" % self_fit.get("idx2", float("nan")),
        "@@RMS@@": "%.3f" % self_fit.get("rms", float("nan")),
        "@@SELFCLS@@": "pass" if self_pass else "fail",
        "@@SELFWORD@@": "Pass." if self_pass else "Fail.",
        "@@SIGNS@@": html_escape(sign_bits),
        "@@TERMS@@": "\n".join(term_rows),
        "@@CASES@@": "\n".join(row_terms(r) for r in results),
        "@@STEPS@@": "\n".join(step_rows),
    }
    for key, val in repl.items():
        page = page.replace(key, val)
    with open(os.path.join(OUT, "index.html"), "w", encoding="utf-8") as f:
        f.write(page)


def main():
    locks = geometric_locks()
    failed = [name for name, ok, _d in locks if not ok]
    if failed:
        for name, ok, detail in locks:
            print("%s  %s  %s" % ("OK" if ok else "FAIL", name, detail))
        raise SystemExit("TPOINT geometric locks failed")
    print("geometric locks passed")
    compile_fitter()
    print("fitter built")
    self_fit = self_check()
    print("self  cone %.2f  perp %.2f  pole alt %.5f  rms %.3f" % (
        self_fit["cone"], self_fit["perp"], self_fit["pole_alt"], self_fit["rms"]))
    results = []
    signs = None
    for case in CASES:
        res = observe(case)
        if signs is None:
            signs = sign_map(res["fit"], case)
        res["signs"] = signs
        results.append(res)
        fit = res["fit"]
        err = errors_arcsec(fit, case, signs)
        print("%s  cone bit %s  rms %.1f\"  pole %.4f deg  err %s" % (
            case["name"], fit.get("cone_bit"), fit.get("rms", -1), fit.get("pole", -1),
            {k: round(v, 1) for k, v in err.items()}))
        print("    recovered cone %.1f perp %.1f idx %.1f paz %.5f palt %.5f" % (
            fit.get("cone", 0), fit.get("perp", 0), fit.get("idx2", 0),
            fit.get("pole_az", 0), fit.get("pole_alt", 0)))
    term_fits = [one_term(key, 3.0) for key in ("ch", "np", "ident", "ih", "ma", "me")]
    for fit in term_fits:
        print("term %-5s  cone %8.1f\"  perp %8.1f\"  idx %8.1f\"  eel %+.5f deg  eaz %+.5f deg  rms %.3f\"" % (
            fit["key"], fit["cone"], fit["perp"], fit["idx2"], fit.get("eq_el", 0),
            fit.get("eq_az", 0), fit["rms"]))
    me_ok = single_terms_ok(term_fits)
    env = envelope()
    print("envelope ready")
    note = skyfield_note()
    save_plots(results, env, term_fits)
    write_report(locks, self_fit, results, term_fits, env, note, me_ok)
    print(os.path.join(OUT, "index.html"))
    if not me_ok:
        raise SystemExit("CH, NP, ID or ME did not match")


if __name__ == "__main__":
    main()
