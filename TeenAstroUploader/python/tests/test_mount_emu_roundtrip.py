"""
Live MainUnit emulator round-trip for Auto! mount config restore.

Requires mainunit_emu listening on TCP 9997:

  pio run -d TeenAstroEmulator -e emu_mainunit
  TeenAstroEmulator/.pio/build/emu/mainunit_emu.exe

Skip if the port is closed.
"""

from __future__ import annotations

import copy
import socket
import unittest

from teenastro_uploader import config_mount, eeprom_mount
from teenastro_uploader.serial_lx200 import SerialSession

EMU = "tcp:127.0.0.1:9997"


def _emu_up() -> bool:
    try:
        with socket.create_connection(("127.0.0.1", 9997), timeout=0.4):
            return True
    except OSError:
        return False


@unittest.skipUnless(_emu_up(), "mainunit_emu not running on 127.0.0.1:9997")
class MountEmuRoundtripTests(unittest.TestCase):
    def setUp(self) -> None:
        self.sess = SerialSession(EMU, baud=57600, timeout=1.5)

    def tearDown(self) -> None:
        self.sess.close()

    def test_identity(self) -> None:
        self.assertEqual(self.sess.query("GVP"), "TeenAstro")

    def test_axis_limit_combinations(self) -> None:
        """Write/read sign combinations within GEM soft-limit geometry."""
        # Typical GEM user limits (must stay inside geo LimMin/LimMax).
        cases = [
            (-180, 180, -90, 270),  # user Auto! failure case (signed)
            (-180, 180, 90, 270),  # positive axis2 min
            (-120, 120, -60, 200),
            (-90, 90, -30, 180),
        ]
        orig = {
            "a1min": self.sess.query("GXLA"),
            "a1max": self.sess.query("GXLB"),
            "a2min": self.sess.query("GXLC"),
            "a2max": self.sess.query("GXLD"),
        }
        try:
            for a1min, a1max, a2min, a2max in cases:
                with self.subTest(a1min=a1min, a1max=a1max, a2min=a2min, a2max=a2max):
                    # Widen max before shrinking min (firmware requires min < max).
                    for cmd in (
                        f":SXLB,{10 * a1max}#",
                        f":SXLD,{10 * a2max}#",
                        f":SXLA,{10 * a1min}#",
                        f":SXLC,{10 * a2min}#",
                    ):
                        ack = self.sess.send_ack(cmd)
                        self.assertTrue(
                            ack[:1] in "01",
                            f"bad ack for {cmd!r}: {ack!r}",
                        )
                        self.assertEqual(
                            ack[:1],
                            "1",
                            f"setter rejected {cmd!r} (outside geo limits?)",
                        )

                    m: dict = {}
                    config_mount._apply_read(m, "a1min", self.sess.query("GXLA"))
                    config_mount._apply_read(m, "a1max", self.sess.query("GXLB"))
                    config_mount._apply_read(m, "a2min", self.sess.query("GXLC"))
                    config_mount._apply_read(m, "a2max", self.sess.query("GXLD"))
                    self.assertEqual(
                        (m["a1min"], m["a1max"], m["a2min"], m["a2max"]),
                        (a1min, a1max, a2min, a2max),
                    )
        finally:
            for tag, raw in orig.items():
                prefix = {
                    "a1min": "SXLA",
                    "a1max": "SXLB",
                    "a2min": "SXLC",
                    "a2max": "SXLD",
                }[tag]
                self.sess.send_ack(f":{prefix},{raw}#")

    def test_config_mount_full_roundtrip(self) -> None:
        before = config_mount.read_mount(self.sess)
        # Mutate a copy using the user's failing pattern
        cfg = copy.deepcopy(before)
        cfg.mount["a1min"] = -180
        cfg.mount["a1max"] = 180
        cfg.mount["a2min"] = -90
        cfg.mount["a2max"] = 270
        cfg.mount["MaxR"] = 500
        cfg.mount["GuideR"] = 1.0
        cfg.mount["Acc"] = "3.0"
        cfg.mount["DefaultR"] = "Fast"
        cfg.mount["hl"] = "-10"
        cfg.mount["ol"] = "+91"
        cfg.mount["el"] = 0
        cfg.mount["wl"] = 0
        cfg.mount["ul"] = 12

        errors = config_mount.write_mount(self.sess, cfg)
        # Motor gear writes may fail on emu fixed gears — ignore those tags
        soft = {e.split(":")[0] for e in errors}
        hard = soft - {
            "mge1",
            "mge2",
            "mst1",
            "mst2",
            "mmu1",
            "mmu2",
            "mlc1",
            "mlc2",
            "mhc1",
            "mhc2",
            "mbl1",
            "mbl2",
            "msil1",
            "msil2",
            "mrot1",
            "mrot2",
        }
        self.assertEqual(hard, set(), f"hard write errors: {errors}")

        mismatches = config_mount.verify_mount(self.sess, cfg)
        # Filter motor fields that emu may not accept
        mismatches = [
            m
            for m in mismatches
            if not m.split(":")[0].startswith("m")
            or m.split(":")[0]
            in ("MaxR",)  # keep MaxR etc. — m* motors only
        ]
        # Keep only limit / rate / Acc style tags we care about
        critical = [
            m
            for m in mismatches
            if m.split(":")[0]
            in {
                "a1min",
                "a1max",
                "a2min",
                "a2max",
                "MaxR",
                "GuideR",
                "Acc",
                "DefaultR",
                "hl",
                "ol",
                "el",
                "wl",
                "ul",
                "SlowR",
                "MediumR",
                "FastR",
            }
        ]
        self.assertEqual(critical, [], "critical mismatches:\n" + "\n".join(critical))

    def test_eeprom_axis_roundtrip(self) -> None:
        cfg = eeprom_mount.read_eeprom(self.sess)
        cfg.axis1_min = -180
        cfg.axis1_max = 180
        cfg.axis2_min = -90
        cfg.axis2_max = 270
        errors = eeprom_mount.write_eeprom(self.sess, cfg)
        soft_prefixes = ("gear", "steps", "micro", "rev", "lc", "hc", "bl", "sil")
        hard = [e for e in errors if not e.startswith(soft_prefixes)]
        self.assertEqual(hard, [], hard)
        bad = eeprom_mount.verify_eeprom(self.sess, cfg)
        axis = [b for b in bad if "axis" in b]
        self.assertEqual(axis, [], axis)

    def test_legacy_backup_restore_like_auto(self) -> None:
        """Simulate Auto! restore of the user's broken JSON encoding."""
        legacy = config_mount.MountConfig()
        legacy.mount.update(
            {
                "a1min": 180,  # legacy = -(eeprom/10) for -180°
                "a1max": 180,
                "a2min": 90,
                "a2max": 270,
                "MaxR": 500,
                "GuideR": 1.0,
                "Acc": "3.0",
                "DefaultR": "Fast",
                "hl": "-10",
                "ol": "+91",
                "el": 0,
                "wl": 0,
                "ul": 12,
            }
        )
        config_mount._normalize_legacy_axis_mins(legacy.mount)
        self.assertEqual(legacy.mount["a2min"], -90)
        errors = config_mount.write_mount(self.sess, legacy)
        hard = {
            e.split(":")[0]
            for e in errors
            if e.split(":")[0]
            in {"a1min", "a1max", "a2min", "a2max", "MaxR", "GuideR", "Acc", "ul", "el", "wl"}
        }
        self.assertEqual(hard, set(), errors)
        mismatches = [
            m
            for m in config_mount.verify_mount(self.sess, legacy)
            if m.split(":")[0] in {"a1min", "a1max", "a2min", "a2max"}
        ]
        self.assertEqual(mismatches, [], mismatches)


if __name__ == "__main__":
    unittest.main()
