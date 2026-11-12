"""Guide optics + limiting magnitude for the eFinder simulator."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from solver.optics import (
    focal_length_for_fov_deg,
    get_asi,
    limiting_magnitude,
    optics_from_param,
    resolve_optics,
)


def test_asi120_one_degree_fov():
    cam = get_asi("ASI120MM Mini")
    fl = focal_length_for_fov_deg(cam.sensor_w_mm, 1.0)
    o = resolve_optics("ASI120MM Mini", focal_mm=fl, aperture_mm=50, exposure_s=1.0)
    assert abs(o.fov_w_deg - 1.0) < 0.02
    assert o.width_px == 1280
    assert 9.0 <= o.limiting_mag <= 12.5


def test_deeper_with_larger_aperture_or_qe():
    shallow = limiting_magnitude(30, 1.0, qe=0.8, read_noise_e=4.0)
    deep = limiting_magnitude(50, 2.0, qe=0.92, read_noise_e=0.6)
    assert deep > shallow


def test_optics_from_param_defaults_to_one_degree():
    o = optics_from_param(
        {
            "Camera": "SIM",
            "Sim_ASI": "ASI220MM Mini",
            "Sim_aperture_mm": "40",
            "Exposure": "1.0",
            "Sim_fov_deg": "1.0",
        }
    )
    assert abs(o.fov_w_deg - 1.0) < 0.02
    assert o.camera.name.startswith("ASI220")
