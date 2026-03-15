#!/usr/bin/env python3
"""
Generate TeenAstro Navigation app icon (assets/icon/app_icon.png).
Design: Compass rose in a double-lined rounded square — black and white.
Uses 4x supersampling and LANCZOS downscale for smooth, anti-aliased edges.
Requires: pip install Pillow
Run from repo root: python teenastro_app/scripts/generate_app_icon.py
"""
import math
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Install Pillow: pip install Pillow")
    raise

SIZE = 1024
SCALE = 4  # Draw at 4x then downscale for anti-aliasing
OUTPUT = Path(__file__).resolve().parent.parent / "assets" / "icon" / "app_icon.png"

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)

# High-quality resampler for downscale (Pillow 9.1+ uses Resampling enum)
try:
    _RESAMPLE = Image.Resampling.LANCZOS
except AttributeError:
    _RESAMPLE = Image.LANCZOS


def draw_rounded_rect(draw, box, radius, fill=None, outline=None):
    x1, y1, x2, y2 = box
    if fill:
        draw.rectangle([x1 + radius, y1, x2 - radius, y2], fill=fill)
        draw.rectangle([x1, y1 + radius, x2, y2 - radius], fill=fill)
        draw.pieslice([x1, y1, x1 + 2 * radius, y1 + 2 * radius], 180, 270, fill=fill)
        draw.pieslice([x2 - 2 * radius, y1, x2, y1 + 2 * radius], 270, 360, fill=fill)
        draw.pieslice([x1, y2 - 2 * radius, x1 + 2 * radius, y2], 90, 180, fill=fill)
        draw.pieslice([x2 - 2 * radius, y2 - 2 * radius, x2, y2], 0, 90, fill=fill)
    if outline:
        draw.arc([x1, y1, x1 + 2 * radius, y1 + 2 * radius], 180, 270, fill=outline, width=2)
        draw.arc([x2 - 2 * radius, y1, x2, y1 + 2 * radius], 270, 360, fill=outline, width=2)
        draw.arc([x1, y2 - 2 * radius, x1 + 2 * radius, y2], 90, 180, fill=outline, width=2)
        draw.arc([x2 - 2 * radius, y2 - 2 * radius, x2, y2], 0, 90, fill=outline, width=2)
        draw.line([(x1 + radius, y1), (x2 - radius, y1)], fill=outline, width=2)
        draw.line([(x1 + radius, y2), (x2 - radius, y2)], fill=outline, width=2)
        draw.line([(x1, y1 + radius), (x1, y2 - radius)], fill=outline, width=2)
        draw.line([(x2, y1 + radius), (x2, y2 - radius)], fill=outline, width=2)


def main():
    w, h = SIZE * SCALE, SIZE * SCALE
    img = Image.new("RGBA", (w, h), WHITE)
    draw = ImageDraw.Draw(img)

    # Outer border (thick)
    margin_outer = int(w * 0.04)
    radius_outer = int(w * 0.18)
    box_outer = [margin_outer, margin_outer, w - margin_outer, h - margin_outer]
    width_outer = max(4, int(w * 0.025))
    try:
        draw.rounded_rectangle(
            box_outer, radius=radius_outer, outline=BLACK, width=width_outer
        )
    except TypeError:
        draw_rounded_rect(draw, box_outer, radius_outer, outline=BLACK)

    # Inner border (thin)
    margin_inner = int(w * 0.10)
    radius_inner = int(w * 0.14)
    box_inner = [margin_inner, margin_inner, w - margin_inner, h - margin_inner]
    width_inner = max(2, int(w * 0.008))
    try:
        draw.rounded_rectangle(
            box_inner, radius=radius_inner, outline=BLACK, width=width_inner
        )
    except TypeError:
        draw_rounded_rect(draw, box_inner, radius_inner, outline=BLACK)

    cx, cy = w / 2, h / 2

    # Circle around compass rose
    circle_r = int(w * 0.32)
    width_circle = max(1, int(w * 0.006))
    draw.ellipse(
        [cx - circle_r, cy - circle_r, cx + circle_r, cy + circle_r],
        outline=BLACK,
        width=width_circle,
    )

    # 4 cardinal triangles (solid)
    outer_r = w * 0.30
    inner_r = w * 0.12
    for i in range(4):
        angle = math.pi / 2 + (i * math.pi / 2)
        tip_x = cx + outer_r * math.cos(angle)
        tip_y = cy - outer_r * math.sin(angle)
        a1 = angle + math.pi * 0.35
        a2 = angle - math.pi * 0.35
        x1 = cx + inner_r * math.cos(a1)
        y1 = cy - inner_r * math.sin(a1)
        x2 = cx + inner_r * math.cos(a2)
        y2 = cy - inner_r * math.sin(a2)
        draw.polygon(
            [(tip_x, tip_y), (x1, y1), (x2, y2)], fill=BLACK, outline=BLACK
        )

    # Downscale with LANCZOS for smooth, anti-aliased result
    img = img.resize((SIZE, SIZE), _RESAMPLE)

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUTPUT, "PNG", optimize=True)
    print(f"Saved {OUTPUT} ({OUTPUT.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
