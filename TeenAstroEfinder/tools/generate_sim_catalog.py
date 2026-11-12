#!/usr/bin/env python3
"""
Build TeenAstroEfinder/data/bright_stars.csv for the simulator.

Primary source (offline): teenastro_app/assets/data/stars_mag9.json
(Hipparcos via VizieR, mag <= 9 — already in the repo).

Optional online top-up: VizieR Hipparcos + Tycho-2 to --mag (when reachable).

Fainter stars (to the optics limiting magnitude, typically ~10–12 for a 1°
guide field) are synthesized at capture time by SimCamera using a published
average sky density law — see solver/sim_camera.py.

Usage:
  py -3 tools/generate_sim_catalog.py
  py -3 tools/generate_sim_catalog.py --online --mag 11
"""
from __future__ import annotations

import argparse
import csv
import json
import ssl
import sys
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
OUT = ROOT / "data" / "bright_stars.csv"
MAG9 = REPO / "teenastro_app" / "assets" / "data" / "stars_mag9.json"


def from_mag9(mag_limit: float) -> list:
    data = json.loads(MAG9.read_text(encoding="utf-8"))
    rows = []
    for s in data.get("stars", []):
        mag = float(s["mag"])
        if mag > mag_limit:
            continue
        rows.append(
            (
                str(s.get("hip", "") or ""),
                float(s["ra"]),
                float(s["dec"]),
                mag,
                s.get("name", "") or "",
            )
        )
    return rows


def _ssl_ctx() -> ssl.SSLContext:
    ctx = ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    return ctx


def vizier(source: str, out_cols: str, mag_col: str, mag_limit: float) -> str:
    url = (
        f"https://vizier.cds.unistra.fr/viz-bin/asu-tsv?"
        f"-source={source}&-out={out_cols}&-out.max=999999"
        f"&{mag_col}=<={mag_limit}&-oc.form=dec"
    )
    req = urllib.request.Request(url, headers={"User-Agent": "TeenAstroEfinder/1.0"})
    with urllib.request.urlopen(req, timeout=180, context=_ssl_ctx()) as resp:
        return resp.read().decode("utf-8", errors="replace")


def from_vizier(mag_limit: float) -> list:
    print("Downloading Hipparcos from VizieR…")
    hip_tsv = vizier("I/239/hip_main", "HIP,RAICRS,DEICRS,Vmag", "Vmag", mag_limit)
    by = {}
    for line in hip_tsv.splitlines():
        line = line.strip()
        if not line or line[0] in "#-" or line.startswith("HIP"):
            continue
        p = [x.strip() for x in line.split("\t")]
        if len(p) < 4:
            continue
        try:
            by[f"HIP{p[0]}"] = (p[0], float(p[1]) / 15.0, float(p[2]), float(p[3]), "")
        except ValueError:
            continue
    print(f"  Hipparcos: {len(by)}")
    if mag_limit > 9.0:
        print("Downloading Tycho-2 from VizieR…")
        time.sleep(0.5)
        tyc_tsv = vizier(
            "I/259/tyc2", "TYC1,TYC2,TYC3,RAdeg,DEdeg,VTmag,HIP", "VTmag", mag_limit
        )
        n = 0
        for line in tyc_tsv.splitlines():
            line = line.strip()
            if not line or line[0] in "#-" or line.startswith("TYC"):
                continue
            p = [x.strip() for x in line.split("\t")]
            if len(p) < 6:
                continue
            try:
                tyc = f"{p[0]}-{p[1]}-{p[2]}"
                ra = float(p[3]) / 15.0
                dec = float(p[4])
                mag = float(p[5])
                hip = p[6] if len(p) > 6 else ""
            except ValueError:
                continue
            if mag > mag_limit:
                continue
            key = f"HIP{hip}" if hip else f"TYC{tyc}"
            if key in by:
                continue
            by[key] = (hip or tyc, ra, dec, mag, "")
            n += 1
        print(f"  Tycho-2 added: {n}")
    return list(by.values())


def write_csv(path: Path, rows: list) -> None:
    rows = sorted(rows, key=lambda r: r[3])
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["hip", "ra_hours", "dec_deg", "mag", "name"])
        for hip, ra, dec, mag, name in rows:
            w.writerow([hip, f"{ra:.8f}", f"{dec:.8f}", f"{mag:.2f}", name])
    print(f"Wrote {len(rows)} stars → {path} ({path.stat().st_size / 1024:.1f} KiB)")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--mag", type=float, default=9.0, help="Catalog magnitude limit")
    ap.add_argument("--online", action="store_true", help="Try VizieR download")
    ap.add_argument("--out", type=Path, default=OUT)
    args = ap.parse_args()

    rows = None
    if args.online:
        try:
            rows = from_vizier(args.mag)
        except Exception as exc:  # noqa: BLE001
            print(f"Online download failed ({exc}); falling back to local mag9 JSON", file=sys.stderr)

    if not rows:
        if not MAG9.exists():
            print(f"Missing {MAG9}", file=sys.stderr)
            return 1
        print(f"Building from {MAG9.name} (mag<={args.mag})")
        rows = from_mag9(args.mag)

    write_csv(args.out, rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
