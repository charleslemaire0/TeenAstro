"""SimCamera projects catalog stars around mount pointing."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from solver.sim_camera import SimCamera


def test_field_near_polaris_has_stars():
    cam = SimCamera(ROOT / "data" / "bright_stars.csv", mag_limit=7.5)
    assert len(cam.stars) > 100
    cam.set_pointing(2.53, 89.26)
    field = cam.stars_in_fov()
    assert len(field) >= 3


def test_field_near_orion_has_stars():
    cam = SimCamera(ROOT / "data" / "bright_stars.csv")
    cam.set_pointing(5.5, -5.0)
    field = cam.stars_in_fov()
    assert len(field) >= 10


def test_capture_writes_png(tmp_path):
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        return  # optional on bare CI
    cam = SimCamera(ROOT / "data" / "bright_stars.csv")
    cam.set_pointing(5.5, -5.0)
    dest = str(tmp_path) + "/"
    cam.capture(1_000_000, 50, False, False, dest)
    out = tmp_path / "capture.png"
    assert out.exists()
    assert cam.last_count >= 5
    ra, dec, stars = cam.solved_from_pointing()
    assert abs(ra - 5.5) < 0.01
    assert abs(dec + 5.0) < 0.01
    assert stars == cam.last_count
