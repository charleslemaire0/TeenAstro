"""Camera interface for TeenAstroEfinder (ported from AstroKeith eFinder Lite)."""


class CameraInterface:
    """All cameras should implement this interface."""

    def initialize(self) -> None:
        pass

    def capture(
        self,
        exposure_time: float,
        gain: float,
        m13: bool,
        polaris: bool,
        dest_path: str,
    ) -> None:
        raise NotImplementedError

    def get_cam_type(self) -> str:
        return "none"
