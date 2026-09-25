"""Render SHC 16x16 XBM icons from SmartController_Display.cpp to PNG."""
import re
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    import subprocess, sys
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pillow", "-q"])
    from PIL import Image

SRC = Path(__file__).resolve().parents[3] / "TeenAstroSHC" / "SmartController_Display.cpp"
OUT = Path(__file__).resolve().parent / "icons"
OUT.mkdir(parents=True, exist_ok=True)

text = SRC.read_text(encoding="utf-8", errors="replace")
pat = re.compile(
    r"static const unsigned char (\w+)\[\][^{]*\{([^}]+)\}",
    re.S,
)
icons = {}
for name, body in pat.findall(text):
    nums = [int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]+", body)]
    if len(nums) == 32:
        icons[name] = nums

def bits(data):
    px = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            b = data[y * 2 + (x // 8)]
            if (b >> (x % 8)) & 1:
                px[y][x] = 1
    return px

def paint(layers, scale=8, bg=(17, 17, 34), fg=(204, 221, 255)):
    canvas = [[0] * 16 for _ in range(16)]
    for data in layers:
        px = bits(data)
        for y in range(16):
            for x in range(16):
                if px[y][x]:
                    canvas[y][x] = 1
    im = Image.new("RGB", (16 * scale, 16 * scale), bg)
    for y in range(16):
        for x in range(16):
            if canvas[y][x]:
                for dy in range(scale):
                    for dx in range(scale):
                        im.putpixel((x * scale + dx, y * scale + dy), fg)
    return im

# Icons shown alone (overlays are composed separately).
alone = [
    "shift_bits", "wifi_sta0_bits", "wifi_sta1_bits", "wifi_sta2_bits", "wifi_ap_bits",
    "wifi_sta0_nc_bits", "wifi_sta1_nc_bits", "wifi_sta2_nc_bits", "wifi_ap_nc_bits",
    "GUIDINGSP_bits", "SLOWSP_bits", "MEDIUMSP_bits", "FASTSP_bits", "MAXSP_bits",
    "home_bits", "parked_bits", "parking_bits", "parkingFailed_bits",
    "no_tracking_bits", "tracking_bits", "sleewing_bits", "slewing_eq_bits",
    "slewing_altaz_bits", "slewing_flip_bits",
    "E_bits", "W_bits", "align1_bits", "align2_bits", "align3_bits", "Aligned_bits",
    "Spiral_bits", "GNSS_bits", "GNSSL_bits", "GNSST_bits",
    "ErrA1_bits", "ErrA2_bits", "ErrHo_bits", "ErrMe_bits", "ErrMf_bits", "ErrUp_bits",
    "Lock___bits", "guiding__bits", "recenter_base_bits", "atrate_base_bits",
]
for name in alone:
    if name not in icons:
        print("missing", name)
        continue
    paint([icons[name]]).save(OUT / f"{name.replace('_bits','')}.png")

composites = {
    "tracking_star": ["tracking_bits", "tracking_star_bits"],
    "tracking_sun": ["tracking_bits", "tracking_sun_bits"],
    "tracking_moon": ["tracking_bits", "tracking_moon_bits"],
    "tracking_target": ["tracking_bits", "tracking_target_bits"],
    "tracking_ra": ["tracking_bits", "tracking_1_bits"],
    "tracking_both": ["tracking_bits", "tracking_2_bits"],
    "tracking_star_both": ["tracking_bits", "tracking_star_bits", "tracking_2_bits"],
    "guide_n": ["guiding__bits", "guiding_N_bits"],
    "guide_s": ["guiding__bits", "guiding_S_bits"],
    "guide_e": ["guiding__bits", "guiding_E_bits"],
    "guide_w": ["guiding__bits", "guiding_W_bits"],
    "recenter_n": ["recenter_base_bits", "guiding_N_bits"],
    "atrate_e": ["atrate_base_bits", "guiding_E_bits"],
    "lock_t": ["Lock___bits", "Lock_T_bits"],
    "lock_f": ["Lock___bits", "Lock_F_bits"],
    "lock_both": ["Lock___bits", "Lock_T_bits", "Lock_F_bits"],
}
for name, layers in composites.items():
    paint([icons[n] for n in layers]).save(OUT / f"{name}.png")

print(f"{len(list(OUT.glob('*.png')))} png in {OUT}")
