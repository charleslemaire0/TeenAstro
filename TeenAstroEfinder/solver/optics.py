"""
Guide-camera optics and limiting-magnitude model for the eFinder simulator.

FOV / plate scale from sensor size + guider focal length.
Limiting magnitude from aperture, exposure, QE and read noise using the
atmosphere-limited CCD form discussed by AAVSO:

  LM ≈ M0 + 5·log10(D_m) + 1.25·log10(t_s) + 2.5·log10(QE/0.5) − 0.05·(RN−1)

with M0 ≈ 17.0 (practical short-exposure mono CMOS under suburban sky).
References:
  https://www.aavso.org/equation-limiting-magnitude-ccd-camera
  ZWO ASI Mini specs (pixel size, QE, read noise, sensor geometry)
"""
from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Dict, Optional, Tuple


@dataclass(frozen=True)
class AsiCamera:
    name: str
    width_px: int
    height_px: int
    pixel_um: float
    qe: float          # peak quantum efficiency 0..1
    read_noise_e: float
    sensor_w_mm: float
    sensor_h_mm: float


# Specs from ZWO ASI Mini manuals / product pages (mono guide cameras).
ASI_CAMERAS: Dict[str, AsiCamera] = {
    "ASI120MM": AsiCamera("ASI120MM", 1280, 960, 3.75, 0.80, 4.0, 4.8, 3.6),
    "ASI120MM Mini": AsiCamera("ASI120MM Mini", 1280, 960, 3.75, 0.80, 4.0, 4.8, 3.6),
    "ASI290MM": AsiCamera("ASI290MM", 1936, 1096, 2.9, 0.80, 1.0, 5.6, 3.2),
    "ASI290MM Mini": AsiCamera("ASI290MM Mini", 1936, 1096, 2.9, 0.80, 1.0, 5.6, 3.2),
    "ASI174MM": AsiCamera("ASI174MM", 1936, 1216, 5.86, 0.78, 3.5, 11.3, 7.1),
    "ASI174MM Mini": AsiCamera("ASI174MM Mini", 1936, 1216, 5.86, 0.78, 3.5, 11.3, 7.1),
    "ASI220MM": AsiCamera("ASI220MM", 1920, 1080, 4.0, 0.92, 0.6, 7.68, 4.32),
    "ASI220MM Mini": AsiCamera("ASI220MM Mini", 1920, 1080, 4.0, 0.92, 0.6, 7.68, 4.32),
}


def normalize_asi_name(name: str) -> str:
    key = name.strip().upper().replace("_", " ").replace("-", " ")
    key = " ".join(key.split())
    aliases = {
        "ASI120": "ASI120MM",
        "ASI120MM": "ASI120MM",
        "ASI120MM MINI": "ASI120MM Mini",
        "ASI290": "ASI290MM",
        "ASI290MM": "ASI290MM",
        "ASI290MM MINI": "ASI290MM Mini",
        "ASI174": "ASI174MM",
        "ASI174MM": "ASI174MM",
        "ASI174MM MINI": "ASI174MM Mini",
        "ASI220": "ASI220MM",
        "ASI220MM": "ASI220MM",
        "ASI220MM MINI": "ASI220MM Mini",
    }
    return aliases.get(key, name.strip())


def get_asi(name: str) -> AsiCamera:
    n = normalize_asi_name(name)
    if n in ASI_CAMERAS:
        return ASI_CAMERAS[n]
    # case-insensitive fallback
    for k, v in ASI_CAMERAS.items():
        if k.upper() == n.upper():
            return v
    raise KeyError(
        f"Unknown ASI camera '{name}'. Choose one of: {', '.join(ASI_CAMERAS)}"
    )


def focal_length_for_fov_deg(sensor_mm: float, fov_deg: float) -> float:
    """Focal length (mm) that yields the given FOV across sensor_mm."""
    half = math.radians(max(fov_deg, 1e-3) / 2.0)
    return sensor_mm / (2.0 * math.tan(half))


def fov_deg(sensor_mm: float, focal_mm: float) -> float:
    return 2.0 * math.degrees(math.atan((sensor_mm / 2.0) / max(focal_mm, 1e-6)))


def plate_scale_arcsec(pixel_um: float, focal_mm: float) -> float:
    # "/pix = 206.265 * pixel_um / focal_mm
    return 206.265 * pixel_um / max(focal_mm, 1e-6)


def limiting_magnitude(
    aperture_mm: float,
    exposure_s: float,
    qe: float = 0.8,
    read_noise_e: float = 2.0,
    m0: float = 17.0,
) -> float:
    """
    Expected guide-star limiting magnitude (atmosphere-limited CCD form).

    AAVSO discussion: LM = M + 5 log10(D_m) + 1.25 log10(t)
    with M ~ 15–17. We use M0=17.0 and fold in QE / read-noise terms for
    modern mono CMOS guide cameras.
    """
    d_m = max(aperture_mm, 1.0) / 1000.0
    t = max(exposure_s, 1e-3)
    qe = min(max(qe, 0.05), 1.0)
    lm = (
        m0
        + 5.0 * math.log10(d_m)
        + 1.25 * math.log10(t)
        + 2.5 * math.log10(qe / 0.5)
        - 0.05 * (read_noise_e - 1.0)
    )
    # Clamp to a useful sim range (catalog depth / tetra3 practicality)
    return max(6.0, min(14.0, lm))


@dataclass
class GuideOptics:
    camera: AsiCamera
    focal_mm: float
    aperture_mm: float
    exposure_s: float
    width_px: int
    height_px: int
    scale_arcsec: float
    fov_w_deg: float
    fov_h_deg: float
    limiting_mag: float

    @property
    def fov_deg(self) -> float:
        return self.fov_w_deg


def resolve_optics(
    camera_name: str = "ASI120MM Mini",
    focal_mm: Optional[float] = None,
    aperture_mm: float = 50.0,
    exposure_s: float = 1.0,
    target_fov_deg: float = 1.0,
    binning: int = 1,
    mag_limit_override: Optional[float] = None,
) -> GuideOptics:
    """
    Build geometry for the simulator.

    If focal_mm is omitted, choose FL so the long sensor axis ≈ target_fov_deg
    (default 1° for a typical eFinder / guide field).
    """
    cam = get_asi(camera_name)
    binning = max(1, int(binning))
    if focal_mm is None or focal_mm <= 0:
        focal_mm = focal_length_for_fov_deg(cam.sensor_w_mm, target_fov_deg)
    w = max(1, cam.width_px // binning)
    h = max(1, cam.height_px // binning)
    pix = cam.pixel_um * binning
    scale = plate_scale_arcsec(pix, focal_mm)
    fov_w = fov_deg(cam.sensor_w_mm, focal_mm)
    fov_h = fov_deg(cam.sensor_h_mm, focal_mm)
    lm = (
        float(mag_limit_override)
        if mag_limit_override is not None
        else limiting_magnitude(aperture_mm, exposure_s, cam.qe, cam.read_noise_e)
    )
    # Keep a margin so the field includes stars near the detection floor
    lm = min(14.0, lm + 0.5)
    return GuideOptics(
        camera=cam,
        focal_mm=float(focal_mm),
        aperture_mm=float(aperture_mm),
        exposure_s=float(exposure_s),
        width_px=w,
        height_px=h,
        scale_arcsec=scale,
        fov_w_deg=fov_w,
        fov_h_deg=fov_h,
        limiting_mag=lm,
    )


def optics_from_param(param: dict) -> GuideOptics:
    cam_name = param.get("Sim_ASI", param.get("ASI_camera", "ASI120MM Mini"))
    fl = param.get("Sim_focal_mm", param.get("Lens_focal_length", ""))
    try:
        focal = float(fl) if str(fl).strip() else None
    except ValueError:
        focal = None
    # Legacy 25/50 mm lens keys meant wide finder lenses, not 1° guiders —
    # if the user still has those and no Sim_focal_mm, prefer 1° default FL.
    if focal is not None and focal <= 60 and "Sim_focal_mm" not in param:
        focal = None
    aperture = float(param.get("Sim_aperture_mm", param.get("Aperture_mm", "50")))
    exposure = float(param.get("Exposure", "1.0"))
    target_fov = float(param.get("Sim_fov_deg", "1.0"))
    binning = int(float(param.get("Sim_binning", "1")))
    override = param.get("Sim_mag_limit")
    mag_override = float(override) if override not in (None, "") else None
    return resolve_optics(
        camera_name=str(cam_name),
        focal_mm=focal,
        aperture_mm=aperture,
        exposure_s=exposure,
        target_fov_deg=target_fov,
        binning=binning,
        mag_limit_override=mag_override,
    )


def describe_optics(o: GuideOptics) -> str:
    return (
        f"{o.camera.name}  {o.width_px}x{o.height_px}  "
        f"FL={o.focal_mm:.1f}mm  D={o.aperture_mm:.1f}mm  "
        f"FOV={o.fov_w_deg:.2f}°x{o.fov_h_deg:.2f}°  "
        f"scale={o.scale_arcsec:.2f}\"/px  "
        f"LM≈{o.limiting_mag:.1f} (t={o.exposure_s:g}s)"
    )
