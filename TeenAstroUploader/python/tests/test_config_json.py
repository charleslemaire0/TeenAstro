"""JSON save/load round-trips (no hardware)."""

import tempfile
import unittest
from pathlib import Path

from teenastro_uploader import config_focuser, config_mount


class ConfigJsonTests(unittest.TestCase):
    def test_mount_taconfig_roundtrip(self):
        cfg = config_mount.MountConfig()
        cfg.mount["mge1"] = 1720
        cfg.mount["mType"] = "Eq-German"
        cfg.sites[0].name = "Home"
        cfg.sites[0].latitude = [48, 30]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "mount.json"
            config_mount.save_mount(path, cfg, taconfig_compat=True)
            loaded = config_mount.load_mount(path)
        self.assertEqual(loaded.mount["mge1"], 1720)
        self.assertEqual(loaded.sites[0].name, "Home")
        self.assertEqual(loaded.sites[0].latitude, [48, 30])

    def test_load_sample_ap600_shape(self):
        # Minimal TAConfig-shaped list
        sample = [
            {**config_mount.DEFAULT_MOUNT, "MaxR": 900},
            [{"name": "S0", "latitude": [1, 2], "longitude": [3, 4],
              "elevation": 10, "currentSite": 0, "timeZone": 1.0}],
        ]
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "ap.json"
            path.write_text(__import__("json").dumps(sample), encoding="utf-8")
            loaded = config_mount.load_mount(path)
        self.assertEqual(loaded.mount["MaxR"], 900)
        self.assertEqual(loaded.sites[0].name, "S0")

    def test_focuser_roundtrip(self):
        cfg = config_focuser.FocuserConfig()
        cfg.settings["maxPos"] = 12345
        cfg.settings["micro"] = 32
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "foc.json"
            config_focuser.save_focuser(path, cfg)
            loaded = config_focuser.load_focuser(path)
        self.assertEqual(loaded.settings["maxPos"], 12345)
        self.assertEqual(loaded.settings["micro"], 32)


if __name__ == "__main__":
    unittest.main()
