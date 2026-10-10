"""Axis limit encode/decode and legacy JSON normalization."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from teenastro_uploader import config_mount


class AxisLimitsCodecTests(unittest.TestCase):
    def test_read_signed_tenths(self) -> None:
        m: dict = {}
        config_mount._apply_read(m, "a1min", "-1800")
        config_mount._apply_read(m, "a1max", "1800")
        config_mount._apply_read(m, "a2min", "-900")
        config_mount._apply_read(m, "a2max", "2700")
        self.assertEqual(m["a1min"], -180)
        self.assertEqual(m["a1max"], 180)
        self.assertEqual(m["a2min"], -90)
        self.assertEqual(m["a2max"], 270)

    def test_write_signed_tenths(self) -> None:
        m = {
            "a1min": -180,
            "a1max": 180,
            "a2min": -90,
            "a2max": 270,
        }
        self.assertEqual(config_mount._build_set_cmd(m, "a1min"), ":SXLA,-1800#")
        self.assertEqual(config_mount._build_set_cmd(m, "a1max"), ":SXLB,1800#")
        self.assertEqual(config_mount._build_set_cmd(m, "a2min"), ":SXLC,-900#")
        self.assertEqual(config_mount._build_set_cmd(m, "a2max"), ":SXLD,2700#")

    def test_positive_a2min_roundtrip_cmd(self) -> None:
        m = {"a2min": 90, "a2max": 270}
        self.assertEqual(config_mount._build_set_cmd(m, "a2min"), ":SXLC,900#")
        out: dict = {}
        config_mount._apply_read(out, "a2min", "900")
        self.assertEqual(out["a2min"], 90)

    def test_legacy_json_user_backup(self) -> None:
        # Same shape as auto_mount_* from the broken encoder
        data = [
            {
                **config_mount.DEFAULT_MOUNT,
                "a1min": 180,
                "a1max": 180,
                "a2min": 90,
                "a2max": 270,
            },
            [],
        ]
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "legacy.json"
            path.write_text(__import__("json").dumps(data), encoding="utf-8")
            cfg = config_mount.load_mount(path)
        self.assertEqual(cfg.mount["a1min"], -180)
        self.assertEqual(cfg.mount["a2min"], -90)
        self.assertEqual(cfg.mount["a2max"], 270)
        self.assertEqual(config_mount._build_set_cmd(cfg.mount, "a2min"), ":SXLC,-900#")

    def test_new_json_positive_a2min_not_flipped(self) -> None:
        data = [
            {
                **config_mount.DEFAULT_MOUNT,
                "a1min": -180,
                "a1max": 180,
                "a2min": 90,
                "a2max": 270,
            },
            [],
        ]
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "new.json"
            path.write_text(__import__("json").dumps(data), encoding="utf-8")
            cfg = config_mount.load_mount(path)
        self.assertEqual(cfg.mount["a2min"], 90)


if __name__ == "__main__":
    unittest.main()
