#!/usr/bin/env python3
"""Add an ASCOM Alpaca parallelogram to the Home architecture diagram."""
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

SHOTS = Path(__file__).resolve().parent / "screenshots"
SRC = SHOTS / "home_architecture_base.png"
DST = SHOTS / "home_architecture_alpaca.png"

FILL = (112, 173, 71)
OUTLINE = (70, 120, 40)
TEXT = (0, 0, 0)
BUS = (120, 120, 120)


def parallelogram(cx: int, cy: int, w: int, h: int, skew: int) -> list[tuple[int, int]]:
    return [
        (cx - w // 2 + skew, cy - h // 2),
        (cx + w // 2 + skew, cy - h // 2),
        (cx + w // 2 - skew, cy + h // 2),
        (cx - w // 2 - skew, cy + h // 2),
    ]


def main() -> None:
    if not SRC.is_file():
        raise SystemExit(f"missing base diagram {SRC}")
    im = Image.open(SRC).convert("RGBA")
    draw = ImageDraw.Draw(im)
    # Fit between SkySafari (~633) and Indi (~819) so the wiki sidebar does not clip it.
    cx, cy = 726, 96
    w, h, skew = 140, 52, 14
    pts = parallelogram(cx, cy, w=w, h=h, skew=skew)
    draw.polygon(pts, fill=FILL + (255,), outline=OUTLINE + (255,))
    try:
        font = ImageFont.truetype("arial.ttf", 14)
        font_sm = ImageFont.truetype("arial.ttf", 11)
    except OSError:
        font = ImageFont.load_default()
        font_sm = font
    y = cy - 18
    for text, f in [("ASCOM Alpaca", font), ("Wi‑Fi / any OS", font_sm)]:
        bbox = draw.textbbox((0, 0), text, font=f)
        tw = bbox[2] - bbox[0]
        draw.text((cx - tw // 2, y), text, fill=TEXT + (255,), font=f)
        y += 16
    draw.line([(cx, cy + h // 2), (cx, 138)], fill=BUS + (255,), width=3)
    im.convert("RGB").save(DST, "PNG", optimize=True)
    print(f"wrote {DST} size={im.size}")


if __name__ == "__main__":
    main()
