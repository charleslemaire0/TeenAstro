#!/usr/bin/env python3
"""
Convert Stellarium's HIP-based constellation lines to our RA/Dec format.

Stellarium uses HIP (Hipparcos) star IDs to define which stars connect.
We resolve HIP -> (ra, dec) from our stars_mag9.json and output
constellation_lines.json in [ra1, dec1, ra2, dec2] format.

Usage:
    python tools/convert_stellarium_constellations.py

Requires: stars_mag9.json (run generate_star_catalog.py first)
"""

import json
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
STARS_JSON = REPO_ROOT / "teenastro_app" / "assets" / "data" / "stars_mag9.json"
OUTPUT = REPO_ROOT / "teenastro_app" / "assets" / "data" / "constellation_lines.json"
STELLARIUM_INDEX = "https://raw.githubusercontent.com/Stellarium/stellarium-skycultures/master/western/index.json"


def main():
    # Load our star catalog for HIP -> (ra, dec) lookup
    hip_to_pos = {}
    if STARS_JSON.exists():
        with open(STARS_JSON, "r", encoding="utf-8") as f:
            data = json.load(f)
        for s in data.get("stars", []):
            hip = s.get("hip")
            if hip is not None:
                hip_to_pos[hip] = (s["ra"], s["dec"])
    print(f"Loaded {len(hip_to_pos)} stars for HIP lookup")

    # Fetch Stellarium western constellation lines
    print("Fetching Stellarium western index...")
    req = urllib.request.Request(STELLARIUM_INDEX, headers={"User-Agent": "TeenAstro/1.0"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        index = json.loads(resp.read().decode())

    constellations = []
    total_segments = 0
    skipped_missing = 0

    for con in index.get("constellations", []):
        iau = con.get("iau", "")
        if not iau:
            continue
        lines_hip = con.get("lines", [])
        lines_ra_dec = []

        for polyline in lines_hip:
            if not isinstance(polyline, list) or len(polyline) < 2:
                continue
            prev_ra, prev_dec = None, None
            for hip in polyline:
                pos = hip_to_pos.get(hip)
                if pos is None:
                    prev_ra, prev_dec = None, None
                    skipped_missing += 1
                    continue
                ra, dec = pos
                if prev_ra is not None:
                    lines_ra_dec.append([round(prev_ra, 4), round(prev_dec, 4), round(ra, 4), round(dec, 4)])
                    total_segments += 1
                prev_ra, prev_dec = ra, dec

        if lines_ra_dec:
            constellations.append({"abbr": iau, "lines": lines_ra_dec})

    print(f"Converted {len(constellations)} constellations, {total_segments} segments")
    if skipped_missing:
        print(f"  (skipped {skipped_missing} segments due to missing HIP in catalog)")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT, "w", encoding="utf-8") as f:
        json.dump({"constellations": constellations}, f, indent=2)

    print(f"Wrote {OUTPUT}")


if __name__ == "__main__":
    main()
