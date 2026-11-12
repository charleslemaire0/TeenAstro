"""
ASI camera backend. Ported from AstroKeith eFinder Lite (ASICamera_Lite.py).
Copyright (C) 2022 Keith Venables — GPL-3.0.
"""
from __future__ import annotations

import logging
from pathlib import Path
from shutil import copyfile

from .camera_interface import CameraInterface

LOG = logging.getLogger("asi_camera")


class ASICamera(CameraInterface):
    def __init__(self, home_path: str | None = None) -> None:
        self.home_path = home_path or str(Path.home())
        self.cam_type = "not found"
        self._camera = None
        try:
            import zwoasi as asi  # type: ignore

            asi.init("/lib/zwoasi/armv8/libASICamera2.so")
            if asi.get_num_cameras() == 0:
                LOG.error("no ASI camera found")
                return
            self.cam_type = "ZWO"
            self._asi = asi
            self.initialize()
            LOG.info("ZWO camera found")
        except Exception as exc:  # noqa: BLE001
            LOG.error("ASI init failed: %s", exc)
            self.cam_type = "not found"

    def initialize(self) -> None:
        if self.cam_type == "not found":
            return
        asi = self._asi
        self._camera = asi.Camera(0)
        self._camera.set_control_value(
            asi.ASI_BANDWIDTHOVERLOAD, self._camera.get_controls()["BandWidth"]["MinValue"]
        )
        self._camera.disable_dark_subtract()
        self._camera.set_control_value(asi.ASI_WB_B, 99)
        self._camera.set_control_value(asi.ASI_WB_R, 75)
        self._camera.set_control_value(asi.ASI_GAMMA, 50)
        self._camera.set_control_value(asi.ASI_BRIGHTNESS, 50)
        self._camera.set_control_value(asi.ASI_FLIP, 0)
        self._camera.set_image_type(asi.ASI_IMG_RAW8)

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
        if self.cam_type == "not found" or self._camera is None:
            raise RuntimeError("ASI camera not found")
        asi = self._asi
        self._camera.set_control_value(asi.ASI_GAIN, int(gain))
        self._camera.set_control_value(asi.ASI_EXPOSURE, int(exposure_time))
        self._camera.capture(filename=out)

    def get_cam_type(self) -> str:
        return self.cam_type
