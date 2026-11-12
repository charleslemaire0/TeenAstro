"""
Synthetic star-field camera for TeenAstroEfinder.

Geometry and limiting magnitude come from the chosen ASI guide camera,
guider focal length and aperture (see optics.py). Real catalog stars
(data/bright_stars.csv, Hipparcos mag≤9) are projected into the FOV;
fainter stars up to the computed limiting magnitude are filled with a
deterministic density model so a ~1° field still has a realistic count.
"""
from __future__ import annotations

import csv
import hashlib
import logging
import math
import random
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple

from .camera_interface import CameraInterface
from .optics import GuideOptics, describe_optics, optics_from_param

LOG = logging.getLogger("sim_camera")

DEG = math.pi / 180.0
RAD = 180.0 / math.pi


@dataclass
class CamGeom:
    width: int
    height: int
    scale_arcsec: float
    fov_deg: float


@dataclass
class Star:
    hip: str
    ra_h: float
    dec_d: float
    mag: float
    name: str = ""


def _angular_sep_deg(ra1_h: float, dec1_d: float, ra2_h: float, dec2_d: float) -> float:
    a1 = ra1_h * 15.0 * DEG
    d1 = dec1_d * DEG
    a2 = ra2_h * 15.0 * DEG
    d2 = dec2_d * DEG
    cos_c = math.sin(d1) * math.sin(d2) + math.cos(d1) * math.cos(d2) * math.cos(a1 - a2)
    cos_c = max(-1.0, min(1.0, cos_c))
    return math.acos(cos_c) * RAD


def _project(
    ra_h: float,
    dec_d: float,
    center_ra_h: float,
    center_dec_d: float,
    scale_arcsec: float,
    width: int,
    height: int,
) -> Optional[Tuple[float, float]]:
    a0 = center_ra_h * 15.0 * DEG
    d0 = center_dec_d * DEG
    a = ra_h * 15.0 * DEG
    d = dec_d * DEG
    cos_c = math.sin(d0) * math.sin(d) + math.cos(d0) * math.cos(d) * math.cos(a - a0)
    if cos_c <= 0.05:
        return None
    xi = math.cos(d) * math.sin(a - a0) / cos_c * RAD
    eta = (math.cos(d0) * math.sin(d) - math.sin(d0) * math.cos(d) * math.cos(a - a0)) / cos_c * RAD
    x = width / 2.0 + xi * 3600.0 / scale_arcsec
    y = height / 2.0 - eta * 3600.0 / scale_arcsec
    return x, y


def cumulative_stars_per_sqdeg(mag: float) -> float:
    """
    Mean cumulative star density N(<m) / deg² (all-sky average).

    Anchored to the Hipparcos mag≤9 catalog density (~2.0 / deg²) and
    extrapolated with ~0.45 dex per magnitude (Allen / common AAVSO tables).
    """
    return 2.0 * (10.0 ** (0.45 * (mag - 9.0)))


def load_catalog(path: Path, mag_limit: float = 9.0) -> List[Star]:
    stars: List[Star] = []
    if not path.exists():
        LOG.warning("star catalog missing: %s", path)
        return stars
    with path.open(newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            try:
                mag = float(row["mag"])
                if mag > mag_limit:
                    continue
                stars.append(
                    Star(
                        hip=row.get("hip", ""),
                        ra_h=float(row["ra_hours"]),
                        dec_d=float(row["dec_deg"]),
                        mag=mag,
                        name=row.get("name", "") or "",
                    )
                )
            except (KeyError, ValueError):
                continue
    LOG.info("loaded %d catalog stars (mag<=%.1f) from %s", len(stars), mag_limit, path.name)
    return stars


class SimCamera(CameraInterface):
    CATALOG_DEPTH = 9.0  # real Hipparcos file depth

    def __init__(
        self,
        catalog_path: Path,
        optics: GuideOptics,
        noise_arcsec: float = 5.0,
    ) -> None:
        self.optics = optics
        self.geom = CamGeom(
            optics.width_px,
            optics.height_px,
            optics.scale_arcsec,
            optics.fov_w_deg,
        )
        self.mag_limit = optics.limiting_mag
        self.stars = load_catalog(catalog_path, min(self.CATALOG_DEPTH, self.mag_limit))
        self.noise_arcsec = noise_arcsec
        self.ra_h = 5.5
        self.dec_d = 22.0
        self.last_count = 0
        self.last_brightest: Optional[Star] = None
        self._pointing_set = False
        LOG.info("SimCamera %s", describe_optics(optics))

    @classmethod
    def from_param(cls, param: dict, data_dir: Path) -> "SimCamera":
        optics = optics_from_param(param)
        return cls(
            catalog_path=data_dir / "bright_stars.csv",
            optics=optics,
            noise_arcsec=float(param.get("Sim_noise_arcsec", "5")),
        )

    def set_pointing(self, ra_hours: float, dec_deg: float) -> None:
        self.ra_h = float(ra_hours) % 24.0
        self.dec_d = max(-90.0, min(90.0, float(dec_deg)))
        self._pointing_set = True

    def initialize(self) -> None:
        pass

    def get_cam_type(self) -> str:
        return "SIM"

    def _fov_area_sqdeg(self) -> float:
        return self.geom.width * self.geom.scale_arcsec / 3600.0 * (
            self.geom.height * self.geom.scale_arcsec / 3600.0
        )

    def _synthetic_faint_stars(self, ra_h: float, dec_d: float) -> List[Star]:
        """Deterministic faint-star fill from catalog depth → limiting magnitude."""
        if self.mag_limit <= self.CATALOG_DEPTH + 0.05:
            return []
        area = self._fov_area_sqdeg()
        n_deep = cumulative_stars_per_sqdeg(self.mag_limit) * area
        n_cat = cumulative_stars_per_sqdeg(self.CATALOG_DEPTH) * area
        n_extra = max(0, int(round(n_deep - n_cat)))
        if n_extra <= 0:
            return []

        seed_src = f"{ra_h:.4f}:{dec_d:.4f}:{self.mag_limit:.2f}:{self.geom.width}x{self.geom.height}"
        seed = int(hashlib.sha1(seed_src.encode("ascii")).hexdigest()[:8], 16)
        rng = random.Random(seed)

        half_w = self.geom.width * self.geom.scale_arcsec / 3600.0 / 2.0
        half_h = self.geom.height * self.geom.scale_arcsec / 3600.0 / 2.0
        out: List[Star] = []
        for i in range(n_extra):
            # Uniform in FOV tangent plane (degrees)
            dx = (rng.random() * 2.0 - 1.0) * half_w
            dy = (rng.random() * 2.0 - 1.0) * half_h
            # Inverse gnomonic (small-angle)
            d0 = dec_d * DEG
            ra = (ra_h + (dx / math.cos(max(0.05, abs(math.cos(d0)))) / 15.0)) % 24.0
            dec = max(-90.0, min(90.0, dec_d + dy))
            # Magnitude drawn from differential density ∝ 10^(0.45 m)
            u = rng.random()
            # CDF between m0 and m1 for N∝10^(0.45m)
            m0, m1 = self.CATALOG_DEPTH, self.mag_limit
            a = 0.45 * math.log(10.0)
            mag = m0 + math.log1p(u * (math.exp(a * (m1 - m0)) - 1.0)) / a
            out.append(Star(hip=f"SIM{i}", ra_h=ra, dec_d=dec, mag=mag, name=""))
        return out

    def stars_in_fov(self, ra_h: Optional[float] = None, dec_d: Optional[float] = None) -> List[Tuple[Star, float, float]]:
        ra = self.ra_h if ra_h is None else ra_h
        dec = self.dec_d if dec_d is None else dec_d
        half_w = self.geom.width * self.geom.scale_arcsec / 3600.0 / 2.0
        half_h = self.geom.height * self.geom.scale_arcsec / 3600.0 / 2.0
        half = (half_w * half_w + half_h * half_h) ** 0.5 * 1.35

        candidates = [s for s in self.stars if _angular_sep_deg(ra, dec, s.ra_h, s.dec_d) <= half]
        candidates.extend(self._synthetic_faint_stars(ra, dec))

        out: List[Tuple[Star, float, float]] = []
        for s in candidates:
            pix = _project(s.ra_h, s.dec_d, ra, dec, self.geom.scale_arcsec, self.geom.width, self.geom.height)
            if pix is None:
                continue
            x, y = pix
            if 0 <= x < self.geom.width and 0 <= y < self.geom.height:
                out.append((s, x, y))
        return out

    def _render_png(self, path: Path, field: List[Tuple[Star, float, float]]) -> None:
        try:
            from PIL import Image, ImageDraw  # type: ignore
        except ImportError as exc:  # pragma: no cover
            raise RuntimeError("Pillow required for SimCamera PNG output") from exc

        img = Image.new("L", (self.geom.width, self.geom.height), color=8)
        draw = ImageDraw.Draw(img)
        # Scale brightness to limiting magnitude so faint guide stars remain visible
        for s, x, y in field:
            dm = self.mag_limit - s.mag
            brightness = max(18, min(255, int(30 + 40 * dm)))
            r = 1 if s.mag > self.mag_limit - 2 else 2 if s.mag > self.mag_limit - 4 else 3
            draw.ellipse((x - r, y - r, x + r, y + r), fill=brightness)
        path.parent.mkdir(parents=True, exist_ok=True)
        img.save(path)

    def capture(
        self,
        exposure_time: float,
        gain: float,
        m13: bool,
        polaris: bool,
        dest_path: str,
    ) -> None:
        if polaris and not self._pointing_set:
            self.set_pointing(2.5303, 89.2641)
        elif m13 and not self._pointing_set:
            self.set_pointing(16.694, -1.867)

        jitter_deg = (self.noise_arcsec / 3600.0) * 0.2
        ra = (self.ra_h + jitter_deg / 15.0) % 24.0
        dec = self.dec_d

        field = self.stars_in_fov(ra, dec)
        self.last_count = len(field)
        self.last_brightest = min((s for s, _, _ in field), key=lambda s: s.mag, default=None)
        out = Path(dest_path + "capture.png")
        self._render_png(out, field)
        LOG.info(
            "sim capture RA=%.4fh Dec=%.3f° FOV=%.2f° LM=%.1f → %d stars → %s",
            self.ra_h,
            self.dec_d,
            self.geom.fov_deg,
            self.mag_limit,
            self.last_count,
            out,
        )

    def solved_from_pointing(self) -> Tuple[float, float, int]:
        noise_h = (self.noise_arcsec / 3600.0) / 15.0
        noise_d = self.noise_arcsec / 3600.0
        return (self.ra_h + noise_h) % 24.0, self.dec_d + noise_d, self.last_count
