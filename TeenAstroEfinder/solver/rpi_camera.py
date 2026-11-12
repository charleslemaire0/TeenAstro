"""
Raspberry Pi camera backend. Ported from AstroKeith eFinder Lite (RPICamera_Lite.py).
Copyright (C) 2022 Keith Venables — GPL-3.0.
"""
from __future__ import annotations

import logging
from pathlib import Path
from shutil import copyfile

from .camera_interface import CameraInterface

LOG = logging.getLogger("rpi_camera")


class RPICamera(CameraInterface):
    def __init__(self, home_path: str | None = None) -> None:
        self.home_path = home_path or str(Path.home())
        self.cam_type = "RPI"
        self.picam2 = None
        try:
            from picamera2 import Picamera2  # type: ignore

            self.picam2 = Picamera2()
            cfg = self.picam2.create_still_configuration(
                main={"size": (960, 760)},
                sensor={"output_size": (2028, 1520)},
                buffer_count=2,
            )
            self.picam2.configure(cfg)
            LOG.info("RPI camera ready")
        except Exception as exc:  # noqa: BLE001
            LOG.error("RPI camera init failed: %s", exc)
            self.cam_type = "not found"

    def capture(
        self,
        exposure_time: float,
        gain: float,
        m13: bool,
        polaris: bool,
        dest_path: str,
    ) -> None:
        out = dest_path + "capture.png"
        if m13:
            copyfile(f"{self.home_path}/Solver/test.png", out)
            return
        if polaris:
            copyfile(f"{self.home_path}/Solver/polaris.png", out)
            return
        if self.cam_type == "not found" or self.picam2 is None:
            raise RuntimeError("RPI camera not found")
        self.picam2.stop()
        self.picam2.set_controls(
            {
                "ExposureTime": int(exposure_time),
                "AnalogueGain": float(gain),
                "AeEnable": False,
            }
        )
        self.picam2.start()
        self.picam2.capture_file(out)

    def get_cam_type(self) -> str:
        return self.cam_type
