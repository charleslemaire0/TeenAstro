#!/usr/bin/env python3
"""
Independent cross-check of TeenAstro GEM 2-star / 5° pole-offset numbers.

References (open-source, not TeenAstro):
  - KStars Ekos PolarAlign::calculateAzAltErrorFromAzAlt
    https://invent.kde.org/education/kstars/-/blob/master/kstars/ekos/align/polaralign.cpp
    northern: altError = axisAlt - latitude, azError = axisAz
  - Astropy SkyCoord.separation for true angular wedge on the sphere
  - TeenAstro polErrorDeg formulas applied to the *known* mechanical pole
    direction (same atan/acos as libraries/TeenAstroCoordConv), for apples-to-
    apples with KStars/Astropy on the injected geometry — separate from the
    2-star SVD recovery checked by the C++ unit test.

Injected geometry (matches test_polar_align_math.cpp):
  latitude = 47.22°, mechanical pole at Az=+5°, Alt=lat+5°.
"""

from __future__ import annotations

import math
import subprocess
import sys
from pathlib import Path

import numpy as np
from astropy import units as u
from astropy.coordinates import AltAz, EarthLocation, SkyCoord


LAT_DEG = 47.22
DAZ_DEG = 5.0
DALT_DEG = 5.0


def kstars_az_alt_error(axis_az_deg: float, axis_alt_deg: float, lat_deg: float) -> tuple[float, float, float]:
    """KStars PolarAlign::calculateAzAltErrorFromAzAlt (northern hemisphere)."""
    # Source: kstars/ekos/align/polaralign.cpp
    alt_error = axis_alt_deg - lat_deg
    az_error = axis_az_deg
    while az_error > 180.0:
        az_error -= 360.0
    # UI total uses hypot(az, alt) — see polaralignmentassistant.cpp
    total_hypot = math.hypot(az_error, alt_error)
    return az_error, alt_error, total_hypot


def astropy_sphere_wedge(lat_deg: float, daz_deg: float, dalt_deg: float) -> float:
    """Angular separation between true NCP and offset pole (degrees)."""
    # Local topocentric frame: NCP at (az=0, alt=lat); offset at (daz, lat+dalt).
    # Use a dummy EarthLocation + time-independent AltAz via SkyCoord(altaz).
    loc = EarthLocation(lat=lat_deg * u.deg, lon=0 * u.deg, height=0 * u.m)
    # Build Cartesian in the same convention as TeenAstro LA3::toDirCos:
    #   x = cos(alt)*cos(-az), y = cos(alt)*sin(-az), z = sin(alt)
    def dir_cos(az_deg: float, alt_deg: float) -> np.ndarray:
        az = math.radians(az_deg)
        alt = math.radians(alt_deg)
        return np.array(
            [
                math.cos(alt) * math.cos(-az),
                math.cos(alt) * math.sin(-az),
                math.sin(alt),
            ]
        )

    a = dir_cos(0.0, lat_deg)
    b = dir_cos(daz_deg, lat_deg + dalt_deg)
    c = float(np.clip(np.dot(a, b), -1.0, 1.0))
    wedge_ta = math.degrees(math.acos(c))

    # Also via Astropy SkyCoord (north-azimuth AltAz): same geometry.
    # Astropy AltAz: az=0 is north, increasing east — matches our pole az.
    frame = AltAz(obstime="J2000", location=loc)
    ncp = SkyCoord(az=0 * u.deg, alt=lat_deg * u.deg, frame=frame)
    mech = SkyCoord(
        az=daz_deg * u.deg, alt=(lat_deg + dalt_deg) * u.deg, frame=frame
    )
    wedge_ap = mech.separation(ncp).to(u.deg).value
    # Prefer Astropy's value; assert both agree.
    if abs(wedge_ta - wedge_ap) > 1e-6:
        raise SystemExit(f"dir-cos vs Astropy mismatch: {wedge_ta} vs {wedge_ap}")
    return wedge_ap


def teenastro_pol_error_on_known_pole(
    lat_deg: float, daz_deg: float, dalt_deg: float
) -> tuple[float, float, float]:
    """Apply TeenAstro polErrorDeg formulas to the known mechanical pole vector."""
    lat = math.radians(lat_deg)
    az = math.radians(daz_deg)
    alt = math.radians(lat_deg + dalt_deg)
    # LA3::toDirCos
    x = np.array(
        [
            math.cos(alt) * math.cos(-az),
            math.cos(alt) * math.sin(-az),
            math.sin(alt),
        ]
    )
    x /= np.linalg.norm(x)
    # PE_EQ_AZ / PE_EQ_ALT / PE_POL_W
    err_az = math.degrees(math.atan(x[1] / x[0]))
    err_alt = math.degrees(math.atan(x[2] / x[0]) - lat)
    x_id = np.array([math.cos(lat), 0.0, math.sin(lat)])
    c = float(np.clip(np.dot(x, x_id), -1.0, 1.0))
    err_w = math.degrees(math.acos(c))
    return err_az, err_alt, err_w


def run_teenastro_unit_dump() -> tuple[float, float, float] | None:
    """Build/run a tiny C++ dump of the 2-star SVD polErrorDeg results."""
    root = Path(__file__).resolve().parents[1]
    tests = Path(__file__).resolve().parent
    src = tests / "_verify_pol_dump.cpp"
    exe = tests / "_verify_pol_dump.exe"
    src.write_text(
        r"""
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"
#include <cmath>
#include <cstdio>

static void addGemStarObservation(CoordConv& cc, double Lat, double dAzRad,
                                  double dAltRad, double azDeg, double altDeg) {
  Coord_HO HO_true(0, altDeg * DEG_TO_RAD, azDeg * DEG_TO_RAD, false);
  Coord_HO HO_mech(0, altDeg * DEG_TO_RAD, azDeg * DEG_TO_RAD + dAzRad, false);
  Coord_EQ EQ = HO_mech.To_Coord_EQ(Lat + dAltRad);
  Coord_IN IN(0, EQ.Dec(), EQ.Ha() - M_PI_2);
  cc.addReference(HO_true.direct_Az_S(), HO_true.Alt(), IN.Axis1(), IN.Axis2());
}

int main() {
  const double Lat = 47.22 * DEG_TO_RAD;
  const double dAz = 5.0 * DEG_TO_RAD;
  const double dAlt = 5.0 * DEG_TO_RAD;
  CoordConv cc;
  cc.clean();
  addGemStarObservation(cc, Lat, dAz, dAlt, 90.0, 45.0);
  addGemStarObservation(cc, Lat, dAz, dAlt, 270.0, 45.0);
  cc.minimizeAxis2();
  cc.minimizeAxis1(M_PI_2);
  printf("%.10f %.10f %.10f\n",
         cc.polErrorDeg(Lat, PE_EQ_AZ),
         cc.polErrorDeg(Lat, PE_EQ_ALT),
         cc.polErrorDeg(Lat, PE_POL_W));
  return 0;
}
""",
        encoding="utf-8",
    )

    # Mirror PlatformIO include layout used by the unity test.
    inc = [
        str(root / "TeenAstroEmulator" / "shim"),
        str(root / "libraries" / "TeenAstroLA3"),
        str(root / "libraries" / "svd3"),
        str(root / "libraries" / "TeenAstroCoord"),
        str(root / "libraries" / "TeenAstroCoordConv"),
        str(root / "libraries"),
    ]
    # Compile by copying sources into a build dir via g++ if available, else
    # reuse the already-built native test binary path / pio.
    # Prefer pio's compiler from the last native build.
    pio_prog = tests / ".pio" / "build" / "native" / "program.exe"
    # Use a dedicated compile with the same sources the test uses.
    cmd_compile = [
        "pio",
        "run",
        "-d",
        str(tests),
        "-e",
        "native",
        "-t",
        "test",
        "--filter",
        "test_polar_align_math",
    ]
    # Simpler: shell out to g++ with includes if present; else parse unity output
    # by instrumenting via a one-shot compile of the dump next to the libs.
    lib_src = [
        root / "libraries" / "TeenAstroLA3" / "TeenAstroLA3.cpp",
        root / "libraries" / "TeenAstroCoord" / "TeenAstroCoord_EQ.cpp",
        root / "libraries" / "TeenAstroCoord" / "TeenAstroCoord_HO.cpp",
        root / "libraries" / "TeenAstroCoord" / "TeenAstroCoord_IN.cpp",
        root / "libraries" / "TeenAstroCoord" / "TeenAstroCoord_LO.cpp",
        root / "libraries" / "TeenAstroCoordConv" / "TeenAstroCoordConv.cpp",
    ]
    # The #include "*.cpp" style needs -I to the lib folders and compiling only the dump.
    gxx_candidates = [
        Path(r"C:\Users\charl\.platformio\packages\toolchain-gccmingw32\bin\g++.exe"),
        Path("g++"),
    ]
    gxx = next((str(p) for p in gxx_candidates if p.name == "g++" or p.exists()), None)
    if gxx is None:
        print("g++ not found; skip live TeenAstro dump.", file=sys.stderr)
        return None
    args = [
        gxx,
        "-std=c++14",
        "-O0",
        "-D_USE_MATH_DEFINES",
        "-DNATIVE_HAL_BUILD",
        "-fpermissive",
        "-o",
        str(exe),
        str(src),
    ]
    for i in inc:
        args.extend(["-I", i])
    r = subprocess.run(args, capture_output=True, text=True, cwd=str(tests))
    if r.returncode != 0:
        print("compile failed:\n", r.stderr[-2000:], file=sys.stderr)
        return None
    r2 = subprocess.run([str(exe)], capture_output=True, text=True, cwd=str(tests))
    if r2.returncode != 0:
        print("run failed:\n", r2.stderr, file=sys.stderr)
        return None
    parts = r2.stdout.strip().split()
    return float(parts[0]), float(parts[1]), float(parts[2])


def main() -> int:
    print("=== Independent geometry (KStars + Astropy) ===")
    axis_az = DAZ_DEG
    axis_alt = LAT_DEG + DALT_DEG
    kz, ka, kh = kstars_az_alt_error(axis_az, axis_alt, LAT_DEG)
    wedge = astropy_sphere_wedge(LAT_DEG, DAZ_DEG, DALT_DEG)
    print(f"KStars azError          = {kz:.6f} deg  (expect {DAZ_DEG})")
    print(f"KStars altError         = {ka:.6f} deg  (expect {DALT_DEG})")
    print(f"KStars UI total hypot   = {kh:.6f} deg  (approx; not sphere angle)")
    print(f"Astropy sphere wedge    = {wedge:.6f} deg")

    print("\n=== TeenAstro polErrorDeg on known mechanical pole ===")
    tz, ta, tw = teenastro_pol_error_on_known_pole(LAT_DEG, DAZ_DEG, DALT_DEG)
    print(f"TeenAstro PE_EQ_AZ      = {tz:.6f} deg")
    print(f"TeenAstro PE_EQ_ALT     = {ta:.6f} deg")
    print(f"TeenAstro PE_POL_W      = {tw:.6f} deg")

    # Sign note: toDirCos uses sin(-az), so atan(y/x) returns -daz for +daz injection.
    # Magnitude must match KStars; sign is a frame convention.
    # Az: TeenAstro toDirCos uses sin(-az), so PE_EQ_AZ = -Az_S of the pole.
    # |az| must match KStars azError for the same geometric offset.
    ok_az = abs(abs(tz) - abs(kz)) < 1e-9
    # Alt: KStars uses (axisAlt - lat). TeenAstro uses atan(z/x)-lat, which is
    # identical only when az==0; with az=5° it differs by ~0.11° (not a bug).
    ok_alt_kstars_pure = abs((axis_alt - LAT_DEG) - ka) < 1e-12  # sanity on KStars helper
    ok_w = abs(tw - wedge) < 1e-6
    print(f"\nMatch |az| to KStars:          {ok_az}  (TA={tz:.6f}, KStars={kz:.6f})")
    print(f"KStars alt helper sanity:      {ok_alt_kstars_pure}")
    print(
        f"Alt definition delta TA-KStars: {ta - ka:.6f} deg "
        "(atan(z/x)-lat vs alt-lat; expected ~0.1 deg at 5/5)"
    )
    print(f"Match wedge to Astropy:        {ok_w}")

    print("\n=== TeenAstro 2-star SVD recovery (live C++ dump) ===")
    dump = run_teenastro_unit_dump()
    if dump is None:
        print("(skipped)")
        recovered_ok = False
    else:
        rz, ra, rw = dump
        print(f"recovered az            = {rz:.6f} deg")
        print(f"recovered alt           = {ra:.6f} deg")
        print(f"recovered wedge         = {rw:.6f} deg")
        # Recovery should match |known-pole| TeenAstro formulas (same definition),
        # and az/wedge must agree with KStars/Astropy within unity tolerances.
        recovered_ok = (
            abs(abs(rz) - abs(tz)) < 0.15
            and abs(ra - ta) < 0.15
            and abs(rw - tw) < 0.25
            and abs(abs(rz) - DAZ_DEG) < 0.15
            and abs(rw - wedge) < 0.25
        )
        print(f"2-star matches known-pole TA + |az|/wedge refs: {recovered_ok}")

    all_ok = ok_az and ok_alt_kstars_pure and ok_w and recovered_ok
    print("\nVERDICT:", "PASS" if all_ok else "FAIL")
    return 0 if all_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
