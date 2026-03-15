#!/usr/bin/env python3
"""
generate_star_catalog.py - Generate a dense star catalog for the planetarium.

Downloads the Hipparcos catalog from VizieR (with B-V color index) and
filters to mag <= 9.0, producing a JSON file for the Flutter app.

Usage:
    python tools/generate_star_catalog.py

Output:
    teenastro_app/assets/data/stars_mag9.json
"""

import json
import os
import ssl
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT = REPO_ROOT / "teenastro_app" / "assets" / "data" / "stars_mag9.json"
IAU_CSN_URL = "https://www.pas.rochester.edu/~emamajek/WGSN/IAU-CSN.txt"

MAG_LIMIT = 9.0


def load_iau_star_names():
    """Load IAU-approved star names from IAU-CSN. Returns dict HIP -> name."""
    result = {}
    try:
        req = urllib.request.Request(
            IAU_CSN_URL, headers={"User-Agent": "TeenAstro/1.0"}
        )
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        with urllib.request.urlopen(req, timeout=30, context=ctx) as resp:
            data = resp.read().decode("utf-8", errors="replace")
    except Exception as e:
        print(f"  IAU-CSN download failed: {e}, using fallback")
        return _fallback_star_names()

    for line in data.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("$"):
            continue
        parts = line.split()
        if len(parts) < 12:
            continue
        try:
            hip_str = parts[-5]
            if hip_str == "_":
                continue
            hip = int(hip_str)
        except (ValueError, IndexError):
            continue
        # Name: find where ASCII name ends (diacritics copy follows)
        i = 1
        for n in range(1, len(parts) // 2 + 1):
            if n + n <= len(parts) and parts[n : n + n] == parts[:n]:
                i = n
                break
        name = " ".join(parts[:i]).strip()
        if name and hip not in result:
            result[hip] = name
    print(f"  Loaded {len(result)} IAU star names")
    return result if result else _fallback_star_names()


def _fallback_star_names():
    """Minimal fallback when IAU-CSN is unavailable."""
    return {
        677: "Alpheratz", 21421: "Aldebaran", 24608: "Capella", 27989: "Betelgeuse",
        30438: "Canopus", 32349: "Sirius", 37279: "Procyon", 37826: "Pollux",
        45238: "Regulus", 57632: "Mizar", 58001: "Spica", 69673: "Arcturus",
        91262: "Vega", 97649: "Altair", 102098: "Deneb",
    }


def download_hipparcos():
    """Download Hipparcos main catalog from VizieR with B-V color index."""
    url = (
        "https://vizier.cds.unistra.fr/viz-bin/asu-tsv?"
        "-source=I/239/hip_main"
        "&-out=HIP,RAICRS,DEICRS,Vmag,B-V"
        "&-out.max=999999"
        f"&Vmag=<={MAG_LIMIT}"
        "&-oc.form=dec"
    )
    print(f"Downloading Hipparcos catalog (mag <= {MAG_LIMIT}) from VizieR...")
    try:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        req = urllib.request.Request(url, headers={'User-Agent': 'TeenAstro/1.0'})
        with urllib.request.urlopen(req, timeout=120, context=ctx) as resp:
            data = resp.read().decode('utf-8', errors='replace')
        return data
    except Exception as e:
        print(f"  VizieR download failed: {e}")
        return None


def parse_vizier_tsv(data, star_names):
    """Parse VizieR TSV output into star records."""
    stars = []
    for line in data.strip().split('\n'):
        line = line.strip()
        if not line or line.startswith('#') or line.startswith('-'):
            continue
        parts = line.split('\t')
        if len(parts) < 4:
            continue
        try:
            hip = int(parts[0].strip())
            ra_deg = float(parts[1].strip())
            dec_deg = float(parts[2].strip())
            vmag = float(parts[3].strip())
        except (ValueError, IndexError):
            continue
        if vmag > MAG_LIMIT:
            continue

        # B-V color index (may be missing)
        bv = None
        if len(parts) >= 5 and parts[4].strip():
            try:
                bv = float(parts[4].strip())
            except ValueError:
                pass

        ra_hours = ra_deg / 15.0
        name = star_names.get(hip, "")

        rec = {
            "ra": round(ra_hours, 6),
            "dec": round(dec_deg, 6),
            "mag": round(vmag, 2),
            "name": name,
            "hip": hip,
        }
        if bv is not None:
            rec["bv"] = round(bv, 3)

        stars.append(rec)
    return stars


def generate_from_existing_catalogs():
    """Fallback: generate from the existing TeenAstro catalogs."""
    catalog_dir = REPO_ROOT / "teenastro_app" / "assets" / "catalogs"
    stars = []
    seen_positions = set()

    for cat_name in ['stars', 'stf', 'stt', 'gcvs']:
        cat_file = catalog_dir / f"{cat_name}.json"
        if not cat_file.exists():
            continue
        with open(cat_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
        for obj in data.get('objects', []):
            ra = obj.get('ra', 0)
            dec = obj.get('dec', 0)
            mag = obj.get('mag')
            if mag is None or mag > MAG_LIMIT:
                continue
            key = f"{ra:.4f},{dec:.4f}"
            if key in seen_positions:
                continue
            seen_positions.add(key)
            stars.append({
                "ra": round(ra, 6),
                "dec": round(dec, 6),
                "mag": round(mag, 2),
                "name": obj.get('name', ''),
            })

    return stars


def main():
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    star_names = load_iau_star_names()
    tsv_data = download_hipparcos()
    stars = None
    if tsv_data:
        stars = parse_vizier_tsv(tsv_data, star_names)
        if len(stars) > 1000:
            print(f"  Downloaded {len(stars)} stars from Hipparcos")
        else:
            print(f"  Only {len(stars)} stars parsed, falling back to local catalogs")
            stars = None

    if not stars:
        print("  Using existing TeenAstro catalogs as fallback...")
        stars = generate_from_existing_catalogs()
        print(f"  Collected {len(stars)} stars from local catalogs")

    # Deduplicate names: keep only first occurrence of each name
    seen_names = set()
    for s in stars:
        n = s.get("name", "")
        if n:
            if n in seen_names:
                s["name"] = ""
            else:
                seen_names.add(n)

    stars.sort(key=lambda s: s['mag'])

    output_data = {"stars": stars}
    with open(OUTPUT, 'w', encoding='utf-8') as f:
        json.dump(output_data, f, separators=(',', ':'))

    named = sum(1 for s in stars if s.get("name"))
    size_kb = os.path.getsize(OUTPUT) / 1024
    print(f"\nWrote {len(stars)} stars ({named} named) to {OUTPUT}")
    print(f"File size: {size_kb:.1f} KB")


if __name__ == '__main__':
    main()
