"""SimCamera projects catalog + faint fill around mount pointing (~1° FOV)."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from solver.optics import resolve_optics
from solver.sim_camera import SimCamera


def _cam(**kw) -> SimCamera:
    optics = resolve_optics(
        camera_name=kw.get("asi", "ASI120MM Mini"),
        focal_mm=kw.get("focal"),
        aperture_mm=kw.get("aperture", 50.0),
        exposure_s=kw.get("exposure", 1.0),
        target_fov_deg=kw.get("fov", 1.0),
    )
    return SimCamera(ROOT / "data" / "bright_stars.csv", optics)


def test_one_degree_field_has_many_stars():
    cam = _cam(fov=1.0, aperture=50.0)
    assert abs(cam.geom.fov_deg - 1.0) < 0.05
    assert cam.mag_limit >= 9.5
    cam.set_pointing(5.5, -5.0)
    field = cam.stars_in_fov()
    # ~0.75 deg² to mag ~11 should yield a usable guide / solve field
    assert len(field) >= 12


def test_field_near_polaris_has_stars():
    cam = _cam()
    cam.set_pointing(2.53, 89.26)
    field = cam.stars_in_fov()
    assert len(field) >= 5


def test_capture_writes_png(tmp_path):
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        return
    cam = _cam()
    cam.set_pointing(5.5, -5.0)
    dest = str(tmp_path) + "/"
    cam.capture(1_000_000, 50, False, False, dest)
    out = tmp_path / "capture.png"
    assert out.exists()
    assert cam.last_count >= 10
    ra, dec, stars = cam.solved_from_pointing()
    assert abs(ra - 5.5) < 0.01
    assert abs(dec + 5.0) < 0.01
    assert stars == cam.last_count
