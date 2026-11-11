"""
Synthetic star-field camera for TeenAstroEfinder.

Projects a bright-star catalog around the mount pointing (RA/Dec from the
main unit via :eC…) into a greyscale PNG that plate_solve can feed to tetra3,
or that Test/SIM mode can use to report a star count without a real sensor.
"""
from __future__ import annotations

import csv
import logging
import math
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple

from .camera_interface import CameraInterface

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
    """Gnomonic projection; returns pixel (x, y) or None if behind the tangent plane."""
    a0 = center_ra_h * 15.0 * DEG
    d0 = center_dec_d * DEG
    a = ra_h * 15.0 * DEG
    d = dec_d * DEG
    cos_c = math.sin(d0) * math.sin(d) + math.cos(d0) * math.cos(d) * math.cos(a - a0)
    if cos_c <= 0.05:
        return None
    # Standard coordinates (radians) → degrees
    xi = math.cos(d) * math.sin(a - a0) / cos_c * RAD
    eta = (math.cos(d0) * math.sin(d) - math.sin(d0) * math.cos(d) * math.cos(a - a0)) / cos_c * RAD
    x = width / 2.0 + xi * 3600.0 / scale_arcsec
    y = height / 2.0 - eta * 3600.0 / scale_arcsec
    return x, y


def load_catalog(path: Path, mag_limit: float = 7.5) -> List[Star]:
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
    def __init__(
        self,
        catalog_path: Path,
        width: int = 960,
        height: int = 760,
        scale_arcsec: float = 50.8,
        mag_limit: float = 7.5,
        noise_arcsec: float = 5.0,
    ) -> None:
        self.geom = CamGeom(width, height, scale_arcsec, width * scale_arcsec / 3600.0)
        self.stars = load_catalog(catalog_path, mag_limit)
        self.mag_limit = mag_limit
        self.noise_arcsec = noise_arcsec
        self.ra_h = 5.5
        self.dec_d = 22.0
        self.last_count = 0
        self.last_brightest: Optional[Star] = None
        self._pointing_set = False

    @classmethod
    def from_param(cls, param: dict, data_dir: Path) -> "SimCamera":
        # Match plate_solve.test_mode / 25 mm RPI FOV defaults
        lens = param.get("Lens_focal_length", "25")
        if lens == "50":
            w, h, scale = 960, 760, 25.4
        else:
            w, h, scale = 960, 760, 50.8
        return cls(
            catalog_path=data_dir / "bright_stars.csv",
            width=w,
            height=h,
            scale_arcsec=scale,
            mag_limit=float(param.get("Sim_mag_limit", "7.5")),
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

    def stars_in_fov(self, ra_h: Optional[float] = None, dec_d: Optional[float] = None) -> List[Tuple[Star, float, float]]:
        ra = self.ra_h if ra_h is None else ra_h
        dec = self.dec_d if dec_d is None else dec_d
        # Search radius = FOV diagonal (poles need a wider sky cone than mid-latitude)
        half_w = self.geom.width * self.geom.scale_arcsec / 3600.0 / 2.0
        half_h = self.geom.height * self.geom.scale_arcsec / 3600.0 / 2.0
        half = (half_w * half_w + half_h * half_h) ** 0.5 * 1.35
        out: List[Tuple[Star, float, float]] = []
        for s in self.stars:
            if _angular_sep_deg(ra, dec, s.ra_h, s.dec_d) > half:
                continue
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
        for s, x, y in field:
            # Brighter = larger / whiter
            brightness = max(20, min(255, int(255 * 10 ** (-0.4 * (s.mag - 1.0)))))
            r = 1 if s.mag > 5.5 else 2 if s.mag > 3.5 else 3 if s.mag > 1.5 else 4
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
        # Legacy Test_mode flags still select fixed regions when pointing was never set
        if polaris and not self._pointing_set:
            self.set_pointing(2.5303, 89.2641)  # Polaris approx J2000
        elif m13 and not self._pointing_set:
            self.set_pointing(16.694, -1.867)  # near M14 / Ophiuchus (legacy test field)

        # Tiny pointing jitter so successive captures are not bit-identical
        jitter_deg = (self.noise_arcsec / 3600.0) * 0.2
        ra = (self.ra_h + jitter_deg / 15.0) % 24.0
        dec = self.dec_d

        field = self.stars_in_fov(ra, dec)
        self.last_count = len(field)
        self.last_brightest = min((s for s, _, _ in field), key=lambda s: s.mag, default=None)
        # plate_solve.dest_path() already ends with /
        out = Path(dest_path + "capture.png")
        self._render_png(out, field)
        LOG.info(
            "sim capture RA=%.4fh Dec=%.3f° → %d stars (exp=%s gain=%s) → %s",
            self.ra_h,
            self.dec_d,
            self.last_count,
            exposure_time,
            gain,
            out,
        )

    def solved_from_pointing(self) -> Tuple[float, float, int]:
        """Return (ra_h, dec_d, stars) for SIM mode without tetra3."""
        # Apply small noise so GoTo++ still has something to refine against
        noise_h = (self.noise_arcsec / 3600.0) / 15.0
        noise_d = self.noise_arcsec / 3600.0
        return (self.ra_h + noise_h) % 24.0, self.dec_d + noise_d, self.last_count
