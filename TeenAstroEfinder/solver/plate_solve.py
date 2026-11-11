"""
Capture + tetra3 / cedar-detect plate solve for TeenAstroEfinder.
Logic adapted from AstroKeith eFinder Lite (eFinder_Lite_4_1.py).
"""
from __future__ import annotations

import logging
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional, Tuple

from .coordinates import Coordinates
from .teenastro_link import SolveOutcome

LOG = logging.getLogger("plate_solve")


@dataclass
class CamGeom:
    width: int
    height: int
    scale_arcsec: float
    fov_deg: float


def cam_for(camera: str, lens_mm: str, test_mode: bool) -> Tuple[CamGeom, str]:
    if test_mode:
        return CamGeom(960, 760, 50.8, 13.5), "t3_fov14_mag8"
    if camera.upper() == "ASI":
        if lens_mm == "50":
            return CamGeom(1280, 960, 15.4, 5.5), "t3_fov5_mag8"
        return CamGeom(1280, 960, 30.8, 11.0), "t3_fov11_mag8"
    # RPI default
    if lens_mm == "50":
        return CamGeom(960, 760, 25.4, 6.8), "t3_fov7_mag8"
    return CamGeom(960, 760, 50.8, 13.5), "t3_fov14_mag8"


def pixel2dxdy(cam: CamGeom, pix_x: float, pix_y: float) -> Tuple[float, float]:
    deg_x = (float(pix_x) - cam.width / 2) * cam.scale_arcsec / 3600
    deg_y = (cam.height / 2 - float(pix_y)) * cam.scale_arcsec / 3600
    return deg_x, deg_y


def dxdy2pixel(cam: CamGeom, dx: float, dy: float) -> Tuple[float, float]:
    pix_x = dx * 3600 / cam.scale_arcsec + cam.width / 2
    pix_y = cam.height / 2 - dy * 3600 / cam.scale_arcsec
    return pix_x, pix_y


class PlateSolver:
    def __init__(
        self,
        param: dict,
        coordinates: Coordinates,
        camera: Any,
        data_dir: Path,
    ) -> None:
        self.param = param
        self.coordinates = coordinates
        self.camera = camera
        self.data_dir = data_dir
        self.test_mode = param.get("Test_mode", "0") in ("1", "True", "true")
        self.cam, self.db_name = cam_for(
            param.get("Camera", "RPI"),
            param.get("Lens_focal_length", "25"),
            self.test_mode,
        )
        self.t3 = None
        self.cedar = None
        self._load_solver()
        dx = float(param.get("d_x", "0") or 0) / 60.0
        dy = float(param.get("d_y", "0") or 0) / 60.0
        self.offset = dxdy2pixel(self.cam, dx, dy)
        self.offset = (self.offset[1], self.offset[0])  # tetra3 target_pixel is (y, x)
        self.last_solution = None
        self.first_star = None

    def _load_solver(self) -> None:
        try:
            from tetra3 import Tetra3, cedar_detect_client  # type: ignore

            self.cedar = cedar_detect_client.CedarDetectClient()
            self.t3 = Tetra3(self.db_name)
            LOG.info("tetra3 database %s loaded", self.db_name)
        except Exception as exc:  # noqa: BLE001
            LOG.warning("tetra3 not available (%s) — test/fake solves only", exc)
            self.t3 = None
            self.cedar = None

    def dest_path(self) -> str:
        if self.param.get("Ramdisk", "true").lower() == "true":
            p = Path("/var/tmp/solve")
        else:
            p = Path.home() / "TeenAstroEfinder" / "images"
        p.mkdir(parents=True, exist_ok=True)
        return str(p) + "/"

    def capture(self, polaris: bool = False) -> None:
        exp_s = float(self.param.get("Exposure", "1.0"))
        gain = float(self.param.get("Gain", "50"))
        m13 = self.test_mode and not polaris
        self.camera.capture(
            int(exp_s * 1_000_000),
            gain,
            m13,
            polaris and self.test_mode,
            self.dest_path(),
        )

    def solve(self, lat: float, lon_east: float, utc=None) -> SolveOutcome:
        t0 = time.time()
        if self.t3 is None or self.cedar is None:
            if self.test_mode:
                return SolveOutcome(
                    ok=True, ra_hours=5.5, dec_deg=22.0, stars=42, ms=int((time.time() - t0) * 1000)
                )
            return SolveOutcome(ok=False, fail_code="C", ms=int((time.time() - t0) * 1000))

        from PIL import Image  # type: ignore

        capture_file = self.dest_path() + "capture.png"
        with Image.open(capture_file).convert("L") as img:
            centroids = self.cedar.extract_centroids(
                img, max_size=10, sigma=8, use_binned=False
            )
            stars = len(centroids)
            if stars < 30:
                return SolveOutcome(ok=False, fail_code="S", stars=stars, ms=int((time.time() - t0) * 1000))
            solution = self.t3.solve_from_centroids(
                centroids,
                (img.size[1], img.size[0]),
                fov_estimate=self.cam.fov_deg,
                fov_max_error=1,
                match_max_error=0.002,
                target_pixel=self.offset,
                return_matches=True,
            )
        if solution.get("RA") is None:
            return SolveOutcome(ok=False, fail_code="U", stars=stars, ms=int((time.time() - t0) * 1000))

        self.last_solution = solution
        self.first_star = centroids[0]
        ra_j2000 = solution["RA_target"] / 15.0
        dec_j2000 = solution["Dec_target"]
        ra_h, dec_d = self.coordinates.j2000_to_jnow(ra_j2000, dec_j2000, lat, lon_east, utc)
        return SolveOutcome(
            ok=True,
            ra_hours=ra_h,
            dec_deg=dec_d,
            stars=stars,
            ms=int((time.time() - t0) * 1000),
        )

    def measure_offset(self, lat: float, lon_east: float, utc=None) -> SolveOutcome:
        self.capture(polaris=True)
        outcome = self.solve(lat, lon_east, utc)
        if not outcome.ok or self.first_star is None:
            return SolveOutcome(ok=False, fail_code=outcome.fail_code or "O", is_offset=True)
        scope_y, scope_x = self.first_star[0], self.first_star[1]
        self.offset = (scope_y, scope_x)
        d_x, d_y = pixel2dxdy(self.cam, scope_x, scope_y)
        self.param["d_x"] = "{: .2f}".format(float(60 * d_x))
        self.param["d_y"] = "{: .2f}".format(float(60 * d_y))
        name = ""
        try:
            hip = str(self.last_solution["matched_catID"][0]) if self.last_solution else ""
            csv_path = self.data_dir / "starnames.csv"
            if csv_path.exists():
                import csv

                with csv_path.open() as f:
                    for row in csv.reader(f):
                        if str(row[1]) == hip:
                            name = row[0].strip()
                            break
        except Exception:  # noqa: BLE001
            name = ""
        return SolveOutcome(
            ok=True,
            is_offset=True,
            offset_dx=float(self.param["d_x"]),
            offset_dy=float(self.param["d_y"]),
            offset_name=name,
            ra_hours=outcome.ra_hours,
            dec_deg=outcome.dec_deg,
            stars=outcome.stars,
            ms=outcome.ms,
        )

    def reset_offset(self) -> None:
        self.param["d_x"] = "0"
        self.param["d_y"] = "0"
        self.offset = (self.cam.height / 2, self.cam.width / 2)
