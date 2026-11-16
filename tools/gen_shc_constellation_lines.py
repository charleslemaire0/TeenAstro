#!/usr/bin/env python3
"""Pack the app constellation lines for the SHC bright-star chart.

Reads teenastro_app/assets/data/constellation_lines.json and writes
libraries/U8g2ext/src/constellation_lines_data.h. Segments are stored in
catalog order (Txt_Constellations). RA is millihours, Dec is centidegrees.

    python tools/gen_shc_constellation_lines.py
    python tools/gen_shc_constellation_lines.py --preview <dir>
"""

import json
import math
import struct
import sys
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SRC = REPO / "teenastro_app" / "assets" / "data" / "constellation_lines.json"
OUT = REPO / "libraries" / "U8g2ext" / "src" / "constellation_lines_data.h"

# Same order as Txt_Constellations in CatalogTypes.h, without the trailing "---".
CATALOG_ABBR = [
    "And", "Ant", "Aps", "Aql", "Aqr", "Ara", "Ari", "Aur", "Boo", "CMa", "CMi", "CVn",
    "Cae", "Cam", "Cap", "Car", "Cas", "Cen", "Cep", "Cet", "Cha", "Cir",
    "Cnc", "Col", "Com", "CrA", "CrB", "Crt", "Cru", "Crv", "Cyg", "Del", "Dor", "Dra",
    "Equ", "Eri", "For", "Gem", "Gru", "Her", "Hor", "Hya", "Hyi", "Ind",
    "LMi", "Lac", "Leo", "Lep", "Lib", "Lup", "Lyn", "Lyr", "Men", "Mic", "Mon", "Mus",
    "Nor", "Oct", "Oph", "Ori", "Pav", "Peg", "Per", "Phe", "Pic", "PsA",
    "Psc", "Pup", "Pyx", "Ret", "Scl", "Sco", "Sct", "Ser", "Sex", "Sge", "Sgr",
    "Tau", "Tel", "TrA", "Tri", "Tuc", "UMa", "UMi", "Vel", "Vir", "Vol", "Vul",
]


def load_lines():
    data = json.loads(SRC.read_text(encoding="utf-8"))
    by_abbr = {}
    for con in data["constellations"]:
        by_abbr[con["abbr"].casefold()] = con["lines"]
    segments = []
    begin = [0]
    missing = []
    for abbr in CATALOG_ABBR:
        lines = by_abbr.pop(abbr.casefold(), None)
        if not lines:
            missing.append(abbr)
            begin.append(begin[-1])
            continue
        for seg in lines:
            ra1, de1, ra2, de2 = seg
            segments.append((
                int(round(ra1 * 1000.0)),
                int(round(de1 * 100.0)),
                int(round(ra2 * 1000.0)),
                int(round(de2 * 100.0)),
            ))
        begin.append(len(segments))
    if len(begin) != 89:
        raise SystemExit(f"expected 89 index entries, got {len(begin)}")
    for raw in segments:
        for i, v in enumerate(raw):
            if not -32768 <= v <= 32767:
                raise SystemExit(f"coordinate does not fit int16: {raw}")
    return begin, segments, missing, sorted(by_abbr)


def write_header(begin, segments):
    lines = [
        "#pragma once",
        "#include <stdint.h>",
        "#include <pgmspace.h>",
        "",
        "// Packed constellation stick figures for the SHC bright-star chart.",
        "// Generated from teenastro_app/assets/data/constellation_lines.json",
        "// by tools/gen_shc_constellation_lines.py. Do not edit.",
        "//",
        "// kConsBegin[i] is the first segment of catalog constellation i",
        "// (Txt_Constellations order). kConsBegin[88] is the sentinel.",
        "// Each segment is ra1, de1, ra2, de2: RA in millihours, Dec in centidegrees.",
        f"// {len(segments)} segments, {len(segments) * 8 + 89 * 2} bytes in flash.",
        "",
        "const uint16_t kConsBegin[89] PROGMEM = {",
    ]
    row = []
    for i, v in enumerate(begin):
        row.append(f"{v:4d}")
        if len(row) == 12 or i == len(begin) - 1:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    lines.append("};")
    lines.append("")
    lines.append("const int16_t kConsSeg[] PROGMEM = {")
    row = []
    for seg in segments:
        row.extend(f"{v:6d}" for v in seg)
        if len(row) >= 8:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("  " + ", ".join(row) + ",")
    lines.append("};")
    lines.append("")
    OUT.write_text("\n".join(lines), encoding="utf-8")


def to_vec(ra_h, dec_d):
    ra = ra_h * math.pi / 12.0
    dec = dec_d * math.pi / 180.0
    cd = math.cos(dec)
    return (cd * math.cos(ra), cd * math.sin(ra), math.sin(dec))


def vadd(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def vdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def vcross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def vnorm(a):
    n = math.sqrt(vdot(a, a))
    if n < 1e-6:
        return (0.0, 0.0, 1.0)
    return (a[0] / n, a[1] / n, a[2] / n)


def decode(raw):
    return raw[0] / 1000.0, raw[1] / 100.0, raw[2] / 1000.0, raw[3] / 100.0


def frame_of(segments, star):
    acc = to_vec(star[0], star[1])
    for raw in segments:
        ra1, de1, ra2, de2 = decode(raw)
        acc = vadd(acc, to_vec(ra1, de1))
        acc = vadd(acc, to_vec(ra2, de2))
    center = vnorm(acc)
    east = vcross((0.0, 0.0, 1.0), center)
    east = (0.0, 1.0, 0.0) if vdot(east, east) < 1e-10 else vnorm(east)
    north = vnorm(vcross(center, east))
    return east, north


def project(east, north, ra, dec):
    p = to_vec(ra, dec)
    return -vdot(p, east), vdot(p, north)


def self_check(begin, segments):
    # North is up, east (higher RA) is left, around a point on the equator.
    east, north = frame_of([], (6.0, 0.0))
    ox, oy = project(east, north, 6.0, 0.0)
    nx, ny = project(east, north, 6.0, 10.0)
    ex, ey = project(east, north, 6.5, 0.0)
    if not (ny > oy and ex < ox and abs(nx - ox) < 0.05):
        raise SystemExit("chart orientation check failed")

    # A figure crossing 0h stays together.
    wrap = [
        (int(round(23.9 * 1000)), 1000, int(round(0.1 * 1000)), 1000),
        (int(round(0.0 * 1000)), 2000, int(round(0.1 * 1000)), 1000),
    ]
    east, north = frame_of(wrap, (0.0, 10.0))
    a = project(east, north, 23.9, 10.0)
    b = project(east, north, 0.1, 10.0)
    far = project(east, north, 12.0, 10.0)
    gap = math.hypot(a[0] - b[0], a[1] - b[1])
    opposite = math.hypot(far[0] - b[0], far[1] - b[1])
    if gap > 0.2 or opposite < gap * 5:
        raise SystemExit("0h wrap check failed")

    # Alpheratz sits on the Andromeda figure, north of the line's southern end.
    and_lines = segments[begin[0]:begin[1]]
    if not and_lines:
        raise SystemExit("Andromeda has no lines")
    star = (0.1398, 29.0904)
    east, north = frame_of(and_lines, star)
    sx, sy = project(east, north, *star)
    ra1, de1, ra2, de2 = decode(and_lines[0])
    ax, ay = project(east, north, ra1, de1)
    if math.hypot(sx - ax, sy - ay) > 0.02:
        raise SystemExit("Alpheratz does not land on the Andromeda line")
    if sy <= min(project(east, north, ra1, de1)[1], project(east, north, ra2, de2)[1]) - 0.05:
        raise SystemExit("Alpheratz is not in the northern part of its segment")


def write_png(path, width, height, rgb):
    def chunk(tag, data):
        crc = zlib.crc32(tag + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

    raw = b"".join(b"\x00" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    path.write_bytes(png)


def render_preview(path, begin, segments, abbr, star, box_w=50, box_h=34, scale_px=8):
    idx = CATALOG_ABBR.index(abbr)
    segs = segments[begin[idx]:begin[idx + 1]]
    east, north = frame_of(segs, star)
    pts = []
    for raw in segs:
        ra1, de1, ra2, de2 = decode(raw)
        pts.append(project(east, north, ra1, de1))
        pts.append(project(east, north, ra2, de2))
    star_p = project(east, north, *star)
    xs = [p[0] for p in pts] + [star_p[0]]
    ys = [p[1] for p in pts] + [star_p[1]]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    cx, cy = (min_x + max_x) / 2.0, (min_y + max_y) / 2.0
    half_w = max((max_x - min_x) / 2.0, 1e-4)
    half_h = max((max_y - min_y) / 2.0, 1e-4)
    scale = min((box_w - 6) / (2 * half_w), (box_h - 6) / (2 * half_h))

    def pix(p):
        return (
            int(round(box_w / 2 + (p[0] - cx) * scale)),
            int(round(box_h / 2 - (p[1] - cy) * scale)),
        )

    img_w, img_h = box_w * scale_px, box_h * scale_px
    buf = bytearray(img_w * img_h * 3)

    def setp(x, y, color):
        if 0 <= x < img_w and 0 <= y < img_h:
            i = (y * img_w + x) * 3
            buf[i:i + 3] = color

    def blot(x, y, color, r=0):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                for sy in range(scale_px):
                    for sx in range(scale_px):
                        setp(x * scale_px + sx + dx, y * scale_px + sy + dy, color)

    for raw in segs:
        ra1, de1, ra2, de2 = decode(raw)
        x0, y0 = pix(project(east, north, ra1, de1))
        x1, y1 = pix(project(east, north, ra2, de2))
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(steps + 1):
            x = x0 + (x1 - x0) * i // steps
            y = y0 + (y1 - y0) * i // steps
            blot(x, y, b"\xb0\xc4\xff")
    sx, sy = pix(star_p)
    blot(sx, sy, b"\xff\xc0\x40", r=2)
    write_png(path, img_w, img_h, buf)


def main():
    begin, segments, missing, unused = load_lines()
    self_check(begin, segments)
    write_header(begin, segments)
    counts = [begin[i + 1] - begin[i] for i in range(88)]
    print(f"Wrote {OUT}")
    print(f"{len(segments)} segments, max {max(counts)} in one constellation")
    if missing:
        print("missing:", ", ".join(missing))
    if unused:
        print("unused JSON abbrs:", ", ".join(unused))
    if "--preview" in sys.argv:
        out = Path(sys.argv[sys.argv.index("--preview") + 1])
        out.mkdir(parents=True, exist_ok=True)
        samples = [
            ("And", (0.1398, 29.0904)),
            ("Ori", (5.9195, 7.4070)),
            ("Ori", (5.2423, -8.2016)),
            ("UMa", (11.0621, 61.7510)),
            ("Cas", (0.6751, 56.5373)),
            ("Cru", (12.4433, -63.0991)),
            ("Psc", (0.1398, 29.0904)),  # replaced below if present
        ]
        # Pisces: mark a vertex of its own figure.
        psc = CATALOG_ABBR.index("Psc")
        if begin[psc + 1] > begin[psc]:
            ra, de, _, _ = decode(segments[begin[psc]])
            samples[-1] = ("Psc", (ra, de))
        for abbr, star in samples:
            name = f"{abbr}_{star[0]:.2f}.png"
            render_preview(out / name, begin, segments, abbr, star)
            print(f"preview {name}")


if __name__ == "__main__":
    main()
