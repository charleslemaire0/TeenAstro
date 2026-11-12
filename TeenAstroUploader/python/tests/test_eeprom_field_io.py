"""Input/output of every MainUnit EEPROM value the uploader writes.

The firmware model is independent of eeprom_mount's parser. Each case sends the
uploader's real command, applies the firmware rule, and parses the get-reply
with the same function the editor uses.
"""

from __future__ import annotations

import unittest

from teenastro_uploader.eeprom_mount import (
    EepromMount,
    apply_read_value,
    clone,
    iter_set_commands,
    split_dms,
)

# Sentinel: the command is not stored, so the previous value remains.
KEEP = object()


def _baseline() -> EepromMount:
    cfg = EepromMount()
    cfg.mount_name = "Base"
    cfg.mount_name_0 = "Zero"
    cfg.mount_name_1 = "One"
    cfg.slew_settle = 5
    cfg.axis1.gear = 100.0
    cfg.axis1.steps = 200
    cfg.axis1.micro = 16
    cfg.axis1.low_curr = 1000
    cfg.axis1.high_curr = 1200
    cfg.axis1.backlash_rate = 16
    cfg.axis2.gear = 180.0
    cfg.axis2.steps = 400
    cfg.axis2.micro = 32
    cfg.axis2.reverse = True
    cfg.axis2.low_curr = 800
    cfg.axis2.high_curr = 1600
    cfg.axis2.backlash = 10
    cfg.axis2.backlash_rate = 32
    cfg.axis2.silent = True
    cfg.guide_rate = 0.5
    cfg.rate1 = 10
    cfg.rate2 = 20
    cfg.rate3 = 30
    cfg.max_rate = 800
    cfg.default_rate = 1
    cfg.deg_acc = 20
    cfg.horizon = 0
    cfg.overhead = 80
    cfg.axis1_min = -90
    cfg.axis1_max = 90
    cfg.axis2_min = -45
    cfg.axis2_max = 180
    cfg.meridian_e = 10
    cfg.meridian_w = 10
    cfg.under_pole = 10.0
    cfg.dist_from_pole = 100
    cfg.enc1_pulse = 1500
    cfg.enc2_pulse = 1500
    cfg.enc2_reverse = True
    cfg.enc_sync = 1
    cfg.refr_tracking = True
    cfg.latitude = "+10*00:00"
    cfg.longitude = "+010*00:00"
    cfg.elevation = 100
    cfg.timezone = 1.0
    return cfg


_MOTOR = {
    "gear": "GXMG",
    "steps": "GXMS",
    "micro": "GXMM",
    "reverse": "GXMR",
    "low_curr": "GXMc",
    "high_curr": "GXMC",
    "backlash": "GXMB",
    "backlash_rate": "GXMb",
    "silent": "GXMm",
}

_FIELD_KEY = {
    "mount_name": "GXOA",
    "mount_name_0": "GXOB",
    "mount_name_1": "GXOC",
    "slew_settle": "GXOS",
    "guide_rate": "GXR0",
    "rate1": "GXR1",
    "rate2": "GXR2",
    "rate3": "GXR3",
    "max_rate": "GXRX",
    "default_rate": "GXRD",
    "deg_acc": "GXRA",
    "horizon": "GXLH",
    "overhead": "GXLO",
    "axis1_min": "GXLA",
    "axis1_max": "GXLB",
    "axis2_min": "GXLC",
    "axis2_max": "GXLD",
    "meridian_e": "GXLE",
    "meridian_w": "GXLW",
    "under_pole": "GXLU",
    "dist_from_pole": "GXLS",
    "enc1_pulse": "GXEPR",
    "enc1_reverse": "GXErR",
    "enc2_pulse": "GXEPD",
    "enc2_reverse": "GXErD",
    "enc_sync": "GXEO",
    "refr_goto": "GXrg",
    "refr_pole": "GXrp",
    "refr_tracking": "GXrt",
    "latitude": "Gtf",
    "longitude": "Ggf",
    "elevation": "Ge",
    "timezone": "GG",
}


def _key(name: str) -> str:
    if name.startswith("axis1."):
        return _MOTOR[name.split(".", 1)[1]] + "R"
    if name.startswith("axis2."):
        return _MOTOR[name.split(".", 1)[1]] + "D"
    return _FIELD_KEY[name]


def _set(cfg: EepromMount, name: str, value: object) -> None:
    if name.startswith("axis1."):
        setattr(cfg.axis1, name.split(".", 1)[1], value)
    elif name.startswith("axis2."):
        setattr(cfg.axis2, name.split(".", 1)[1], value)
    else:
        setattr(cfg, name, value)


def _get(cfg: EepromMount, name: str) -> object:
    if name.startswith("axis1."):
        return getattr(cfg.axis1, name.split(".", 1)[1])
    if name.startswith("axis2."):
        return getattr(cfg.axis2, name.split(".", 1)[1])
    return getattr(cfg, name)


def _same(name: str, actual: object, expected: object) -> bool:
    if name in ("latitude", "longitude"):
        return split_dms(str(actual)) == split_dms(str(expected))
    if isinstance(expected, float) or isinstance(actual, float):
        return abs(float(actual) - float(expected)) <= 0.051
    return actual == expected


class _Firmware:
    """MainUnit store/reply rules for the commands the uploader sends."""

    def __init__(self) -> None:
        self.reply: dict[str, str] = {}

    def apply(self, key: str, cmd: str) -> bool:
        body = cmd[1:-1] if cmd.endswith("#") else cmd[1:]
        if body.startswith("St"):
            return self._lat(body[2:])
        if body.startswith("Sg"):
            return self._lon(body[2:])
        if body.startswith("Se"):
            return self._elev(body[2:])
        if body.startswith("SG"):
            return self._tz(body[2:])
        if not body.startswith("SX") or "," not in body:
            return False
        head, val = body[2:].split(",", 1)
        stored = self._sx(head, val)
        if stored is None:
            return False
        self.reply[key] = stored
        return True

    def _sx(self, head: str, val: str) -> str | None:
        if head in ("OA", "OB", "OC"):
            return val if len(val) <= 15 else None
        if head == "OS":
            n = int(val)
            return str(n) if 0 <= n < 20 else None
        if len(head) == 3 and head[0] == "M":
            return self._motor(head[1], val)
        if head == "R0":
            n = int(val)
            stored = n if 0 < n < 256 else 1
            return f"{stored / 100:.2f}"
        if head in ("R1", "R2", "R3"):
            n = int(val)
            default = {1: 4, 2: 16, 3: 64}[int(head[1])]
            stored = n if 0 < n < 256 else default
            return f"{stored:.2f}"
        if head == "RX":
            n = int(val)
            return str(n) if 0 <= n <= 65535 else None
        if head == "RD":
            n = int(val)
            return str(n if 0 <= n <= 4 else 3)
        if head == "RA":
            deg = min(max(0.1 * int(val), 0.1), 25.0)
            tenths = int(deg * 10)
            return f"{tenths / 10:.1f}"
        if head == "LH":
            n = int(val)
            return f"{n:+d}*" if -30 <= n <= 30 else None
        if head == "LO":
            n = int(val)
            return f"{n:+d}*" if 45 <= n <= 91 else None
        if head in ("LA", "LB", "LC", "LD"):
            return str(int(val))
        if head in ("LE", "LW"):
            minutes = int(val)
            minutes = min(180, max(-180, minutes))
            return str(minutes)
        if head == "LU":
            hours = int(val) / 10.0
            hours = min(12.0, max(9.0, hours))
            return str(int(round(hours * 10)))
        if head == "LS":
            n = int(val)
            return f"{n:02d}*" if 0 <= n <= 181 else None
        if head in ("EPR", "EPD"):
            if not val or len(val) >= 7:
                return None
            n = int(val)
            return str(n) if 0 < n <= 360000 else None
        if head in ("ErR", "ErD", "EO"):
            if head == "EO":
                n = int(val)
                return str(n) if 0 <= n <= 7 else None
            return val if val in ("0", "1") else None
        if head in ("rg", "rp", "rt"):
            return val if val in ("y", "n") else None
        return None

    def _motor(self, head: str, val: str) -> str | None:
        kind = head[0]
        if kind == "G":
            if not (1 < len(val) < 11) or not val.isdigit():
                return None
            return str(int(val))
        if kind == "S":
            if not (1 < len(val) < 11) or not val.isdigit():
                return None
            return str(int(val))
        if kind == "M":
            if len(val) != 1 or not val.isdigit():
                return None
            n = int(val)
            return str(n) if 1 <= n <= 8 else None
        if kind in ("R", "m"):
            return val if val in ("0", "1") else None
        if kind in ("c", "C"):
            quant = (int(val) // 100) * 100
            return str(quant) if quant >= 100 else None
        if kind == "B":
            n = int(val)
            return str(n) if 0 <= n <= 999 else None
        if kind == "b":
            n = int(val)
            return str(n) if 16 <= n <= 64 else None
        return None

    def _dms(self, text: str, deg_digits: int, limit: int) -> tuple[int, int, int] | None:
        if len(text) != 1 + deg_digits + 1 + 2 + 1 + 2:
            return None
        if text[0] not in "+-":
            return None
        if text[1 + deg_digits] != "*" or text[1 + deg_digits + 3] != ":":
            return None
        try:
            deg = int(text[1 : 1 + deg_digits])
            minute = int(text[2 + deg_digits : 4 + deg_digits])
            sec = int(text[5 + deg_digits : 7 + deg_digits])
        except ValueError:
            return None
        if deg > limit or minute > 59 or sec > 59:
            return None
        sign = -1 if text[0] == "-" else 1
        return sign * deg, minute, sec

    def _lat(self, text: str) -> bool:
        parts = self._dms(text, 2, 90)
        if parts is None:
            return False
        deg, minute, sec = parts
        self.reply["Gtf"] = f"{deg:+03d}*{minute:02d}:{sec:02d}"
        return True

    def _lon(self, text: str) -> bool:
        # Firmware strips the sign, then reads three degree digits.
        if not text or text[0] not in "+-":
            return False
        parts = self._dms(text, 3, 360)
        if parts is None:
            return False
        deg, minute, sec = parts
        self.reply["Ggf"] = f"{deg:+04d}*{minute:02d}:{sec:02d}"
        return True

    def _elev(self, text: str) -> bool:
        try:
            n = int(text)
        except ValueError:
            return False
        if n < -200 or n > 8000 or n < -32768 or n > 32767:
            return False
        self.reply["Ge"] = f"{n:+04d}"
        return True

    def _tz(self, text: str) -> bool:
        try:
            toff = float(text)
        except ValueError:
            return False
        if toff < -12.0 or toff > 12.0:
            return False
        self.reply["GG"] = f"{toff:+05.1f}"
        return True


def _materialize(fw: _Firmware) -> EepromMount:
    cfg = EepromMount()
    for key, reply in fw.reply.items():
        apply_read_value(cfg, key, reply)
    return cfg


# (field, value written, value read back). KEEP means the mount rejects it.
_CASES: list[tuple[str, object, object]] = [
    ("mount_name", "Pier", "Pier"),
    ("mount_name", "N" * 15, "N" * 15),
    ("mount_name", "N" * 16, KEEP),
    ("mount_name_0", "Home", "Home"),
    ("mount_name_1", "Field", "Field"),
    ("slew_settle", 0, 0),
    ("slew_settle", 19, 19),
    ("slew_settle", 20, KEEP),
    ("axis1.gear", 1.0, 1.0),
    ("axis1.gear", 360.125, 360.125),
    ("axis1.gear", 1.0004, 1.0),
    ("axis1.gear", 0.001, KEEP),
    ("axis2.gear", 90.5, 90.5),
    ("axis1.steps", 10, 10),
    ("axis1.steps", 400, 400),
    ("axis1.steps", 5, KEEP),
    ("axis2.steps", 48, 48),
    ("axis1.micro", 2, 2),
    ("axis1.micro", 4, 4),
    ("axis1.micro", 8, 8),
    ("axis1.micro", 256, 256),
    ("axis2.micro", 64, 64),
    ("axis1.reverse", True, True),
    ("axis2.reverse", False, False),
    ("axis1.low_curr", 100, 100),
    ("axis1.low_curr", 250, 200),
    ("axis1.low_curr", 50, KEEP),
    ("axis2.high_curr", 2800, 2800),
    ("axis1.backlash", 0, 0),
    ("axis1.backlash", 999, 999),
    ("axis1.backlash", 1000, KEEP),
    ("axis1.backlash_rate", 16, 16),
    ("axis1.backlash_rate", 64, 64),
    ("axis1.backlash_rate", 15, KEEP),
    ("axis1.backlash_rate", 65, KEEP),
    ("axis1.silent", True, True),
    ("axis2.silent", False, False),
    ("guide_rate", 0.01, 0.01),
    ("guide_rate", 2.55, 2.55),
    ("guide_rate", 0.0, 0.01),
    ("guide_rate", 2.56, 0.01),
    ("rate1", 1, 1),
    ("rate1", 255, 255),
    ("rate1", 0, 4),
    ("rate1", 256, 4),
    ("rate2", 1, 1),
    ("rate2", 256, 16),
    ("rate3", 255, 255),
    ("rate3", 256, 64),
    ("max_rate", 32, 32),
    ("max_rate", 4000, 4000),
    ("default_rate", 0, 0),
    ("default_rate", 4, 4),
    ("default_rate", 5, 3),
    ("default_rate", -1, 3),
    ("deg_acc", 1, 1),
    ("deg_acc", 10, 10),
    ("deg_acc", 250, 250),
    ("deg_acc", 0, 1),
    ("deg_acc", 300, 250),
    ("horizon", -30, -30),
    ("horizon", 30, 30),
    ("horizon", -31, KEEP),
    ("horizon", 31, KEEP),
    ("overhead", 45, 45),
    ("overhead", 91, 91),
    ("overhead", 44, KEEP),
    ("overhead", 92, KEEP),
    ("axis1_min", -360, -360),
    ("axis1_min", -180, -180),
    ("axis1_max", 360, 360),
    ("axis2_min", 90, 90),
    ("axis2_max", 270, 270),
    ("meridian_e", -45, -45),
    ("meridian_e", 15, 15),
    ("meridian_e", 45, 45),
    ("meridian_e", 46, 45),
    ("meridian_w", -46, -45),
    ("under_pole", 9.0, 9.0),
    ("under_pole", 10.5, 10.5),
    ("under_pole", 12.0, 12.0),
    ("under_pole", 8.0, 9.0),
    ("under_pole", 13.0, 12.0),
    ("dist_from_pole", 0, 0),
    ("dist_from_pole", 181, 181),
    ("dist_from_pole", 182, KEEP),
    ("enc1_pulse", 1, 1),
    ("enc1_pulse", 360000, 360000),
    ("enc1_pulse", 0, KEEP),
    ("enc1_pulse", 360001, KEEP),
    ("enc2_pulse", 2500, 2500),
    ("enc1_reverse", True, True),
    ("enc2_reverse", False, False),
    ("enc_sync", 0, 0),
    ("enc_sync", 7, 7),
    ("enc_sync", 8, KEEP),
    ("refr_goto", True, True),
    ("refr_pole", True, True),
    ("refr_tracking", False, False),
    ("latitude", "+48*51:30", "+48*51:30"),
    ("latitude", "-33*52:00", "-33*52:00"),
    ("latitude", "+90*00:00", "+90*00:00"),
    ("latitude", "+91*00:00", KEEP),
    ("longitude", "+122*25:00", "+122*25:00"),
    ("longitude", "-180*00:00", "-180*00:00"),
    ("longitude", "-12*00:00", "-012*00:00"),
    ("longitude", "+361*00:00", KEEP),
    ("elevation", -200, -200),
    ("elevation", 0, 0),
    ("elevation", 8000, 8000),
    ("elevation", -201, KEEP),
    ("elevation", 8001, KEEP),
    ("timezone", -12.0, -12.0),
    ("timezone", -5.5, -5.5),
    ("timezone", 0.0, 0.0),
    ("timezone", 5.5, 5.5),
    ("timezone", 12.0, 12.0),
    ("timezone", 12.5, KEEP),
    ("timezone", -12.5, KEEP),
    ("timezone", 14.0, KEEP),
]

_COMMANDS = {
    ("timezone", 12.0): ":SG-12.0#",
    ("timezone", -5.5): ":SG+5.5#",
    ("timezone", 0.0): ":SG+0.0#",
    ("deg_acc", 10): ":SXRA,10#",
    ("meridian_e", 15): ":SXLE,60#",
    ("guide_rate", 0.5): ":SXR0,050#",
    ("axis1_min", -180): ":SXLA,-1800#",
    ("under_pole", 12.0): ":SXLU,120#",
    ("latitude", "+48*51:30"): ":St+48*51:30#",
    ("longitude", "+122*25:00"): ":Sg+122*25:00#",
    ("longitude", "-12*00:00"): ":Sg-012*00:00#",
    ("elevation", -200): ":Se-200#",
    ("elevation", 8000): ":Se+8000#",
}


class EepromFieldIoTests(unittest.TestCase):
    def setUp(self) -> None:
        self.base = _baseline()
        self.fw = _Firmware()
        for key, cmd, _tag in iter_set_commands(self.base):
            self.assertTrue(self.fw.apply(key, cmd), cmd)
        got = _materialize(self.fw)
        for name in _FIELD_KEY:
            self.assertTrue(
                _same(name, _get(got, name), _get(self.base, name)),
                f"baseline {name}: wrote {_get(self.base, name)!r}, read {_get(got, name)!r}",
            )
        for name in ("gear", "steps", "micro", "reverse", "low_curr", "high_curr", "backlash", "backlash_rate", "silent"):
            for axis in ("axis1", "axis2"):
                field = f"{axis}.{name}"
                self.assertTrue(
                    _same(field, _get(got, field), _get(self.base, field)),
                    f"baseline {field}: wrote {_get(self.base, field)!r}, read {_get(got, field)!r}",
                )

    def test_each_value(self) -> None:
        for name, value, expected in _CASES:
            with self.subTest(field=name, value=value):
                cfg = clone(self.base)
                _set(cfg, name, value)
                trial = _Firmware()
                trial.reply = dict(self.fw.reply)
                matched = [item for item in iter_set_commands(cfg) if item[0] == _key(name)]
                if (name, value) in _COMMANDS:
                    self.assertEqual(matched[0][1], _COMMANDS[(name, value)])
                if not matched:
                    accepted = False
                else:
                    accepted = trial.apply(matched[0][0], matched[0][1])
                got = _materialize(trial)
                if expected is KEEP:
                    self.assertFalse(accepted, f"{name}={value!r} was stored")
                    want = _get(self.base, name)
                else:
                    self.assertTrue(accepted, f"{name}={value!r} was rejected")
                    want = expected
                actual = _get(got, name)
                self.assertTrue(
                    _same(name, actual, want),
                    f"{name}: wrote {value!r}, read {actual!r}, expected {want!r}",
                )

    def test_guide_rate_command_from_baseline(self) -> None:
        cmds = {key: cmd for key, cmd, _tag in iter_set_commands(self.base)}
        self.assertEqual(cmds["GXR0"], ":SXR0,050#")
        self.assertEqual(cmds["GG"], ":SG-1.0#")


if __name__ == "__main__":
    unittest.main()
