#!/usr/bin/env python3
"""
Generate TeenAstro app icon (assets/icon/app_icon.png).
Requires: pip install Pillow
Run from repo root: python teenastro_app/scripts/generate_app_icon.py
"""
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("Install Pillow: pip install Pillow")
    raise

SIZE = 1024
OUTPUT = Path(__file__).resolve().parent.parent / "assets" / "icon" / "app_icon.png"


def main():
    # Dark night-sky gradient (deep blue to purple)
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Rounded rect background (night-sky blue)
    margin = int(SIZE * 0.08)
    radius = int(SIZE * 0.22)
    box = [margin, margin, SIZE - margin, SIZE - margin]
    draw.rounded_rectangle(box, radius=radius, fill=(22, 32, 72, 255))

    # Telescope silhouette (white): tube + eyepiece + tripod
    cx, cy = SIZE // 2, int(SIZE * 0.52)
    tube_w = int(SIZE * 0.14)
    tube_h = int(SIZE * 0.36)
    # Main tube (rounded rect)
    tube_box = [
        cx - tube_w, cy - tube_h,
        cx + tube_w, cy + tube_h,
    ]
    draw.rounded_rectangle(tube_box, radius=tube_w // 2, fill=(255, 255, 255, 255))
    # Eyepiece (small circle at top)
    eyepiece_y = cy - tube_h - int(SIZE * 0.02)
    eyepiece_r = int(SIZE * 0.06)
    draw.ellipse(
        [cx - eyepiece_r, eyepiece_y - eyepiece_r, cx + eyepiece_r, eyepiece_y + eyepiece_r],
        fill=(255, 255, 255, 255),
    )
    # Tripod: three legs
    leg_y = cy + tube_h
    leg_len = int(SIZE * 0.22)
    leg_w = int(SIZE * 0.04)
    for angle in [210, 270, 330]:  # degrees
        import math
        rad = math.radians(angle)
        x2 = cx + leg_len * math.cos(rad)
        y2 = leg_y + leg_len * math.sin(rad)
        draw.line([(cx, leg_y), (x2, y2)], fill=(255, 255, 255, 255), width=leg_w)

    # Small star accent (top-right)
    star_cx = int(SIZE * 0.78)
    star_cy = int(SIZE * 0.22)
    star_r = int(SIZE * 0.04)
    draw.ellipse(
        [
            star_cx - star_r, star_cy - star_r,
            star_cx + star_r, star_cy + star_r,
        ],
        fill=(255, 248, 220, 255),
    )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUTPUT, "PNG", optimize=True)
    print(f"Saved {OUTPUT} ({OUTPUT.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
