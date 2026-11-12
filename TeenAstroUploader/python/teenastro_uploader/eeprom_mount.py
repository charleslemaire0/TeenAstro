"""
MainUnit EEPROM read / write / verify (ASCOM + Webserver field set).

Groups match TeenAstroWifi pages: Mount, Motors, Speed, Limits, Encoders, Site, Tracking.
Supports two stored mounts (index 0/1); switching index reboots the board.
"""

from __future__ import annotations

import math
import time
from copy import deepcopy
from dataclasses import dataclass, field
from typing import Any

from .serial_lx200 import SerialSession

# Firmware stores the base-2 exponent (1..8 → 2..256 microsteps).
MICROSTEPS = (2, 4, 8, 16, 32, 64, 128, 256)
MTYPE_LABELS = ("Eq-German", "Eq-Fork", "AltAz-Tee", "AltAz-Fork")
MTYPE_TO_CMD = {"Eq-German": "1", "Eq-Fork": "2", "AltAz-Tee": "3", "AltAz-Fork": "4"}


def _strip_star(val: str) -> str:
    v = (val or "").strip()
    return v[:-1] if v.endswith("*") else v


def _as_int(val: str, default: int = 0) -> int:
    try:
        return int(float(_strip_star(val)))
    except (TypeError, ValueError):
        return default


def _as_float(val: str, default: float = 0.0) -> float:
    try:
        return float(_strip_star(val))
    except (TypeError, ValueError):
        return default


@dataclass
class MotorAxis:
    gear: float = 1.0  # true gear ratio (wire = gear * 1000)
    steps: int = 200
    micro: int = 16
    reverse: bool = False
    low_curr: int = 1000
    high_curr: int = 1000
    backlash: int = 0
    backlash_rate: int = 16
    silent: bool = False

    def to_dict(self) -> dict[str, Any]:
        return {
            "gear": self.gear,
            "steps": self.steps,
            "micro": self.micro,
            "reverse": self.reverse,
            "low_curr": self.low_curr,
            "high_curr": self.high_curr,
            "backlash": self.backlash,
            "backlash_rate": self.backlash_rate,
            "silent": self.silent,
        }

    @classmethod
    def from_dict(cls, d: dict[str, Any]) -> MotorAxis:
        m = cls()
        for k, v in d.items():
            if hasattr(m, k):
                setattr(m, k, v)
        return m


@dataclass
class EepromMount:
    # Mount / identity
    mount_index: int = 0
    mount_name: str = ""
    mount_name_0: str = ""
    mount_name_1: str = ""
    mtype: str = "Eq-German"
    slew_settle: int = 0

    # Motors
    axis1: MotorAxis = field(default_factory=MotorAxis)
    axis2: MotorAxis = field(default_factory=MotorAxis)

    # Speed (rates)
    guide_rate: float = 0.5
    rate1: int = 4
    rate2: int = 16
    rate3: int = 64
    max_rate: int = 800
    default_rate: int = 0  # 0..4
    deg_acc: int = 10  # tenths of a degree; :GXRA# replies in degrees

    # Limits (UI units, same as ASCOM / Webserver)
    horizon: int = -10
    overhead: int = 90
    axis1_min: int = -360
    axis1_max: int = 360
    axis2_min: int = -360
    axis2_max: int = 360
    meridian_e: int = 15  # degrees past meridian
    meridian_w: int = 15
    under_pole: float = 12.0  # hours
    dist_from_pole: int = 181

    # Encoders
    enc1_pulse: int = 0
    enc1_reverse: bool = False
    enc2_pulse: int = 0
    enc2_reverse: bool = False
    enc_sync: int = 0

    # Tracking / refraction
    refr_goto: bool = False
    refr_pole: bool = False
    refr_tracking: bool = False

    # Site (current)
    latitude: str = "+00*00"
    longitude: str = "+000*00"
    elevation: int = 0
    timezone: float = 0.0

    meta: dict[str, str] = field(default_factory=dict)
    warnings: list[str] = field(default_factory=list)

    def to_file_dict(self) -> dict[str, Any]:
        return {
            "kind": "TeenAstroMainUnitEEPROM",
            "format": 1,
            "meta": self.meta,
            "mount_index": self.mount_index,
            "mount_name": self.mount_name,
            "mount_name_0": self.mount_name_0,
            "mount_name_1": self.mount_name_1,
            "mtype": self.mtype,
            "slew_settle": self.slew_settle,
            "axis1": self.axis1.to_dict(),
            "axis2": self.axis2.to_dict(),
            "guide_rate": self.guide_rate,
            "rate1": self.rate1,
            "rate2": self.rate2,
            "rate3": self.rate3,
            "max_rate": self.max_rate,
            "default_rate": self.default_rate,
            "deg_acc": self.deg_acc,
            "horizon": self.horizon,
            "overhead": self.overhead,
            "axis1_min": self.axis1_min,
            "axis1_max": self.axis1_max,
            "axis2_min": self.axis2_min,
            "axis2_max": self.axis2_max,
            "meridian_e": self.meridian_e,
            "meridian_w": self.meridian_w,
            "under_pole": self.under_pole,
            "dist_from_pole": self.dist_from_pole,
            "enc1_pulse": self.enc1_pulse,
            "enc1_reverse": self.enc1_reverse,
            "enc2_pulse": self.enc2_pulse,
            "enc2_reverse": self.enc2_reverse,
            "enc_sync": self.enc_sync,
            "refr_goto": self.refr_goto,
            "refr_pole": self.refr_pole,
            "refr_tracking": self.refr_tracking,
            "latitude": self.latitude,
            "longitude": self.longitude,
            "elevation": self.elevation,
            "timezone": self.timezone,
        }

    @classmethod
    def from_file_dict(cls, data: dict[str, Any]) -> EepromMount:
        cfg = cls()
        if data.get("kind") not in (None, "TeenAstroMainUnitEEPROM"):
            pass
        cfg.meta = dict(data.get("meta") or {})
        for key in (
            "mount_index",
            "mount_name",
            "mount_name_0",
            "mount_name_1",
            "mtype",
            "slew_settle",
            "guide_rate",
            "rate1",
            "rate2",
            "rate3",
            "max_rate",
            "default_rate",
            "deg_acc",
            "horizon",
            "overhead",
            "axis1_min",
            "axis1_max",
            "axis2_min",
            "axis2_max",
            "meridian_e",
            "meridian_w",
            "under_pole",
            "dist_from_pole",
            "enc1_pulse",
            "enc1_reverse",
            "enc2_pulse",
            "enc2_reverse",
            "enc_sync",
            "refr_goto",
            "refr_pole",
            "refr_tracking",
            "latitude",
            "longitude",
            "elevation",
            "timezone",
        ):
            if key in data:
                setattr(cfg, key, data[key])
        if isinstance(data.get("axis1"), dict):
            cfg.axis1 = MotorAxis.from_dict(data["axis1"])
        if isinstance(data.get("axis2"), dict):
            cfg.axis2 = MotorAxis.from_dict(data["axis2"])
        return cfg


def split_dms(text: str) -> tuple[int, int, int]:
    """Signed degrees, minutes, seconds. Missing fields are 0."""
    raw = (text or "").strip().replace("*", " ").replace(":", " ").replace("'", " ")
    nums: list[int] = []
    for part in raw.split()[:3]:
        try:
            nums.append(int(float(part)))
        except ValueError:
            break
    while len(nums) < 3:
        nums.append(0)
    return nums[0], abs(nums[1]), abs(nums[2])


def _utc_offset(timezone_hours: float) -> float:
    """LX200 :SG# offset. Positive zero stays positive so 0 h is not sent as -0.0."""
    offset = -float(timezone_hours)
    return 0.0 if offset == 0.0 else offset


def format_site_angle(prefix: str, text: str, deg_digits: int) -> str:
    """High-precision site setter. Lat is +DD*MM:SS, longitude is +DDD*MM:SS."""
    deg, minute, sec = split_dms(text)
    width = deg_digits + 1  # sign counts in the width
    return f":{prefix}{deg:+0{width}d}*{minute:02d}:{sec:02d}#"


def _query_angle(sess: SerialSession, precise: str, plain: str) -> str:
    resp = sess.query(precise)
    if any(ch.isdigit() for ch in resp):
        return resp
    return sess.query(plain)


def apply_read_value(cfg: EepromMount, key: str, reply: str) -> None:
    """Parse one get-reply into the same units write_eeprom sends."""
    if key == "GXOA":
        cfg.mount_name = reply
    elif key == "GXOB":
        cfg.mount_name_0 = reply
    elif key == "GXOC":
        cfg.mount_name_1 = reply
    elif key == "GXOS":
        cfg.slew_settle = _as_int(reply)
    elif key.startswith("GXM") and len(key) == 5:
        ax = cfg.axis1 if key[-1] == "R" else cfg.axis2
        field = key[3]
        if field == "G":
            ax.gear = round(_as_int(reply) / 1000.0, 3)
        elif field == "S":
            ax.steps = _as_int(reply, 200)
        elif field == "M":
            exp = _as_int(reply, 4)
            # Exponent 1..8 is 2..256 microsteps. Anything else is not a stored setting.
            ax.micro = (1 << exp) if 1 <= exp <= 8 else 16
        elif field == "R":
            ax.reverse = reply.strip() != "0"
        elif field == "c":
            ax.low_curr = _as_int(reply, 1000)
        elif field == "C":
            ax.high_curr = _as_int(reply, 1000)
        elif field == "B":
            ax.backlash = _as_int(reply)
        elif field == "b":
            ax.backlash_rate = _as_int(reply, 16)
        elif field == "m":
            ax.silent = reply.strip() != "0"
    elif key == "GXR0":
        cfg.guide_rate = _as_float(reply, 0.5)
    elif key == "GXR1":
        cfg.rate1 = _as_int(reply, 4)
    elif key == "GXR2":
        cfg.rate2 = _as_int(reply, 16)
    elif key == "GXR3":
        cfg.rate3 = _as_int(reply, 64)
    elif key == "GXRX":
        cfg.max_rate = _as_int(reply, 800)
    elif key == "GXRD":
        cfg.default_rate = _as_int(reply)
    elif key == "GXRA":
        # Reply is degrees ("1.0"). The setter payload is tenths.
        cfg.deg_acc = int(round(_as_float(reply, 1.0) * 10))
    elif key == "GXLH":
        cfg.horizon = _as_int(reply, -10)
    elif key == "GXLO":
        cfg.overhead = _as_int(reply, 90)
    elif key == "GXLA":
        cfg.axis1_min = int(round(_as_int(reply) / 10.0))
    elif key == "GXLB":
        cfg.axis1_max = int(round(_as_int(reply) / 10.0))
    elif key == "GXLC":
        cfg.axis2_min = int(round(_as_int(reply) / 10.0))
    elif key == "GXLD":
        cfg.axis2_max = int(round(_as_int(reply) / 10.0))
    elif key == "GXLE":
        cfg.meridian_e = int(round(_as_int(reply) / 4.0))
    elif key == "GXLW":
        cfg.meridian_w = int(round(_as_int(reply) / 4.0))
    elif key == "GXLU":
        cfg.under_pole = round(_as_int(reply) / 10.0, 1)
    elif key == "GXLS":
        cfg.dist_from_pole = _as_int(reply, 181)
    elif key == "GXEPR":
        cfg.enc1_pulse = _as_int(reply)
    elif key == "GXErR":
        cfg.enc1_reverse = reply.strip() != "0"
    elif key == "GXEPD":
        cfg.enc2_pulse = _as_int(reply)
    elif key == "GXErD":
        cfg.enc2_reverse = reply.strip() != "0"
    elif key == "GXEO":
        cfg.enc_sync = _as_int(reply)
    elif key == "GXrg":
        cfg.refr_goto = reply.lower().startswith("y")
    elif key == "GXrp":
        cfg.refr_pole = reply.lower().startswith("y")
    elif key == "GXrt":
        cfg.refr_tracking = reply.lower().startswith("y")
    elif key in ("Gt", "Gtf"):
        cfg.latitude = reply or "+00*00"
    elif key in ("Gg", "Ggf"):
        cfg.longitude = reply or "+000*00"
    elif key == "Ge":
        cfg.elevation = _as_int(reply)
    elif key == "GG":
        cfg.timezone = -_as_float(reply)
    else:
        raise KeyError(key)


def iter_set_commands(
    cfg: EepromMount, *, write_mount_type: bool = False
) -> list[tuple[str, str, str]]:
    """(read key, set command, error tag) for every value write_eeprom stores.

    Encoder pulse of 0 is omitted: the firmware rejects it, and a cleared
    pulse does not survive the next boot.
    """
    cmds: list[tuple[str, str, str]] = [
        ("GXOA", f":SXOA,{cfg.mount_name}#", "mount_name"),
        ("GXOB", f":SXOB,{cfg.mount_name_0}#", "mount_name_0"),
        ("GXOC", f":SXOC,{cfg.mount_name_1}#", "mount_name_1"),
        ("GXOS", f":SXOS,{int(cfg.slew_settle)}#", "slew_settle"),
    ]
    for ax, suf, tag in (
        (cfg.axis1, "R", "1"),
        (cfg.axis2, "D", "2"),
    ):
        gear_raw = int(round(float(ax.gear) * 1000))
        micro = int(ax.micro) if int(ax.micro) >= 1 else 1
        exp = int(round(math.log(micro, 2)))
        cmds.extend(
            [
                (f"GXMG{suf}", f":SXMG{suf},{gear_raw}#", f"gear{tag}"),
                (f"GXMS{suf}", f":SXMS{suf},{int(ax.steps)}#", f"steps{tag}"),
                (f"GXMM{suf}", f":SXMM{suf},{exp}#", f"micro{tag}"),
                (f"GXMR{suf}", f":SXMR{suf},{'1' if ax.reverse else '0'}#", f"rev{tag}"),
                (f"GXMc{suf}", f":SXMc{suf},{int(ax.low_curr)}#", f"lc{tag}"),
                (f"GXMC{suf}", f":SXMC{suf},{int(ax.high_curr)}#", f"hc{tag}"),
                (f"GXMB{suf}", f":SXMB{suf},{int(ax.backlash)}#", f"bl{tag}"),
                (f"GXMb{suf}", f":SXMb{suf},{int(ax.backlash_rate)}#", f"blr{tag}"),
                (
                    f"GXMm{suf}",
                    f":SXMm{suf},{'1' if ax.silent else '0'}#",
                    f"sil{tag}",
                ),
            ]
        )
    guide_enc = int(round(float(cfg.guide_rate) * 100))
    cmds.extend(
        [
            ("GXR0", f":SXR0,{guide_enc:03d}#", "guide"),
            ("GXR1", f":SXR1,{int(cfg.rate1)}#", "rate1"),
            ("GXR2", f":SXR2,{int(cfg.rate2)}#", "rate2"),
            ("GXR3", f":SXR3,{int(cfg.rate3)}#", "rate3"),
            ("GXRX", f":SXRX,{int(cfg.max_rate)}#", "max_rate"),
            ("GXRD", f":SXRD,{int(cfg.default_rate)}#", "default_rate"),
            ("GXRA", f":SXRA,{int(cfg.deg_acc)}#", "deg_acc"),
            ("GXLH", f":SXLH,{int(cfg.horizon)}#", "horizon"),
            ("GXLO", f":SXLO,{int(cfg.overhead)}#", "overhead"),
            ("GXLA", f":SXLA,{10 * int(cfg.axis1_min)}#", "a1min"),
            ("GXLB", f":SXLB,{10 * int(cfg.axis1_max)}#", "a1max"),
            ("GXLC", f":SXLC,{10 * int(cfg.axis2_min)}#", "a2min"),
            ("GXLD", f":SXLD,{10 * int(cfg.axis2_max)}#", "a2max"),
            ("GXLE", f":SXLE,{int(cfg.meridian_e) * 4}#", "mer_e"),
            ("GXLW", f":SXLW,{int(cfg.meridian_w) * 4}#", "mer_w"),
            ("GXLU", f":SXLU,{int(round(float(cfg.under_pole) * 10))}#", "under_pole"),
            ("GXLS", f":SXLS,{int(cfg.dist_from_pole)}#", "dist_pole"),
        ]
    )
    if int(cfg.enc1_pulse) > 0:
        cmds.append(("GXEPR", f":SXEPR,{int(cfg.enc1_pulse)}#", "enc1p"))
    cmds.append(("GXErR", f":SXErR,{'1' if cfg.enc1_reverse else '0'}#", "enc1r"))
    if int(cfg.enc2_pulse) > 0:
        cmds.append(("GXEPD", f":SXEPD,{int(cfg.enc2_pulse)}#", "enc2p"))
    cmds.append(("GXErD", f":SXErD,{'1' if cfg.enc2_reverse else '0'}#", "enc2r"))
    cmds.extend(
        [
            ("GXEO", f":SXEO,{int(cfg.enc_sync)}#", "enc_sync"),
            ("GXrg", f":SXrg,{'y' if cfg.refr_goto else 'n'}#", "refr_g"),
            ("GXrp", f":SXrp,{'y' if cfg.refr_pole else 'n'}#", "refr_p"),
            ("GXrt", f":SXrt,{'y' if cfg.refr_tracking else 'n'}#", "refr_t"),
            ("Gtf", format_site_angle("St", cfg.latitude, 2), "latitude"),
            ("Ggf", format_site_angle("Sg", cfg.longitude, 3), "longitude"),
            ("Ge", f":Se{int(cfg.elevation):+04d}#", "elevation"),
            ("GG", ":SG%+02.1f#" % _utc_offset(cfg.timezone), "timezone"),
        ]
    )
    if write_mount_type:
        code = MTYPE_TO_CMD.get(cfg.mtype, "")
        if code:
            cmds.append(("mtype", f":S!{code}#", "mtype"))
    return cmds


_FIELD_KEYS = (
    "GXOA",
    "GXOB",
    "GXOC",
    "GXOS",
    "GXMGR",
    "GXMSR",
    "GXMMR",
    "GXMRR",
    "GXMcR",
    "GXMCR",
    "GXMBR",
    "GXMbR",
    "GXMmR",
    "GXMGD",
    "GXMSD",
    "GXMMD",
    "GXMRD",
    "GXMcD",
    "GXMCD",
    "GXMBD",
    "GXMbD",
    "GXMmD",
    "GXR0",
    "GXR1",
    "GXR2",
    "GXR3",
    "GXRX",
    "GXRD",
    "GXRA",
    "GXLH",
    "GXLO",
    "GXLA",
    "GXLB",
    "GXLC",
    "GXLD",
    "GXLE",
    "GXLW",
    "GXLU",
    "GXLS",
    "GXEPR",
    "GXErR",
    "GXEPD",
    "GXErD",
    "GXEO",
    "GXrg",
    "GXrp",
    "GXrt",
    "Gtf",
    "Ggf",
    "Ge",
    "GG",
)


def _q(sess: SerialSession, cmd: str, max_len: int = 64) -> str:
    return sess.query(cmd, max_len=max_len)


def read_eeprom(sess: SerialSession) -> EepromMount:
    cfg = EepromMount()
    cfg.meta["product"] = _q(sess, "GVP")
    cfg.meta["firmware"] = _q(sess, "GVN")
    cfg.meta["board"] = _q(sess, "GVB")
    cfg.meta["driver"] = _q(sess, "GVb")

    idx = _as_int(_q(sess, "GXOI"))
    cfg.mount_index = 0 if idx not in (0, 1) else idx

    # Mount type from GXAS packet nibble (same as TAConfig)
    try:
        import base64

        b64 = _q(sess, "GXAS", max_len=220)
        pkt = base64.b64decode(b64)
        if len(pkt) >= 2:
            mt = (pkt[1] >> 4) & 0x7
            cfg.mtype = {
                1: "Eq-German",
                2: "Eq-Fork",
                3: "AltAz-Tee",
                4: "AltAz-Fork",
            }.get(mt, cfg.mtype)
    except Exception as exc:  # noqa: BLE001
        cfg.warnings.append(f"mtype: {exc}")

    for key in _FIELD_KEYS:
        if key == "Gtf":
            reply = _query_angle(sess, "Gtf", "Gt")
        elif key == "Ggf":
            reply = _query_angle(sess, "Ggf", "Gg")
        else:
            reply = _q(sess, key)
        apply_read_value(cfg, key, reply)

    return cfg


def _ack(sess: SerialSession, cmd: str, errors: list[str], tag: str) -> None:
    try:
        resp = sess.send_ack(cmd)
        if resp and resp[0] not in "01":
            errors.append(f"{tag}: unexpected ack {resp!r}")
    except Exception as exc:  # noqa: BLE001
        errors.append(f"{tag}: {exc}")


def write_eeprom(
    sess: SerialSession,
    cfg: EepromMount,
    *,
    write_mount_type: bool = False,
) -> list[str]:
    """Write EEPROM fields. Returns list of errors (empty = OK)."""
    errors: list[str] = []
    for _key, cmd, tag in iter_set_commands(cfg, write_mount_type=write_mount_type):
        if cmd.startswith(":S!"):
            try:
                # Reboots the board — no reliable ack.
                sess._ser.write(cmd.encode("ascii"))  # noqa: SLF001
            except Exception as exc:  # noqa: BLE001
                errors.append(f"{tag}: {exc}")
            continue
        _ack(sess, cmd, errors, tag)
    return errors


def switch_mount_index(sess: SerialSession, index: int) -> None:
    """Select mount 0 or 1. Firmware reboots after this command."""
    if index not in (0, 1):
        raise ValueError("Mount index must be 0 or 1")
    sess.send_ack(f":SXOI,{index}#")


def _close(a: Any, b: Any, tol: float = 1e-3) -> bool:
    try:
        return abs(float(a) - float(b)) <= tol
    except (TypeError, ValueError):
        return a == b


def verify_eeprom(sess: SerialSession, expected: EepromMount) -> list[str]:
    """Re-read device and return human-readable mismatches."""
    got = read_eeprom(sess)
    bad: list[str] = []

    def chk(label: str, e: Any, a: Any, *, tol: float | None = None) -> None:
        if tol is not None:
            if not _close(e, a, tol):
                bad.append(f"{label}: wrote {e!r}, read {a!r}")
        elif e != a:
            bad.append(f"{label}: wrote {e!r}, read {a!r}")

    chk("mount_index", expected.mount_index, got.mount_index)
    chk("mount_name", expected.mount_name, got.mount_name)
    chk("mount_name_0", expected.mount_name_0, got.mount_name_0)
    chk("mount_name_1", expected.mount_name_1, got.mount_name_1)
    chk("slew_settle", expected.slew_settle, got.slew_settle)

    for label, e_ax, g_ax in (("axis1", expected.axis1, got.axis1), ("axis2", expected.axis2, got.axis2)):
        chk(f"{label}.gear", e_ax.gear, g_ax.gear, tol=0.002)
        chk(f"{label}.steps", e_ax.steps, g_ax.steps)
        chk(f"{label}.micro", e_ax.micro, g_ax.micro)
        chk(f"{label}.reverse", e_ax.reverse, g_ax.reverse)
        chk(f"{label}.low_curr", e_ax.low_curr, g_ax.low_curr)
        chk(f"{label}.high_curr", e_ax.high_curr, g_ax.high_curr)
        chk(f"{label}.backlash", e_ax.backlash, g_ax.backlash)
        chk(f"{label}.backlash_rate", e_ax.backlash_rate, g_ax.backlash_rate)
        chk(f"{label}.silent", e_ax.silent, g_ax.silent)

    chk("guide_rate", expected.guide_rate, got.guide_rate, tol=0.02)
    chk("rate1", expected.rate1, got.rate1)
    chk("rate2", expected.rate2, got.rate2)
    chk("rate3", expected.rate3, got.rate3)
    chk("max_rate", expected.max_rate, got.max_rate)
    chk("default_rate", expected.default_rate, got.default_rate)
    chk("deg_acc", expected.deg_acc, got.deg_acc)

    chk("horizon", expected.horizon, got.horizon)
    chk("overhead", expected.overhead, got.overhead)
    chk("axis1_min", expected.axis1_min, got.axis1_min)
    chk("axis1_max", expected.axis1_max, got.axis1_max)
    chk("axis2_min", expected.axis2_min, got.axis2_min)
    chk("axis2_max", expected.axis2_max, got.axis2_max)
    chk("meridian_e", expected.meridian_e, got.meridian_e)
    chk("meridian_w", expected.meridian_w, got.meridian_w)
    chk("under_pole", expected.under_pole, got.under_pole, tol=0.15)
    chk("dist_from_pole", expected.dist_from_pole, got.dist_from_pole)

    chk("enc1_pulse", expected.enc1_pulse, got.enc1_pulse)
    chk("enc1_reverse", expected.enc1_reverse, got.enc1_reverse)
    chk("enc2_pulse", expected.enc2_pulse, got.enc2_pulse)
    chk("enc2_reverse", expected.enc2_reverse, got.enc2_reverse)
    chk("enc_sync", expected.enc_sync, got.enc_sync)

    chk("refr_goto", expected.refr_goto, got.refr_goto)
    chk("refr_pole", expected.refr_pole, got.refr_pole)
    chk("refr_tracking", expected.refr_tracking, got.refr_tracking)

    if split_dms(expected.latitude) != split_dms(got.latitude):
        bad.append(
            f"latitude: wrote {expected.latitude!r}, read {got.latitude!r}"
        )
    if split_dms(expected.longitude) != split_dms(got.longitude):
        bad.append(
            f"longitude: wrote {expected.longitude!r}, read {got.longitude!r}"
        )
    chk("elevation", int(expected.elevation), int(got.elevation))
    chk("timezone", expected.timezone, got.timezone, tol=0.05)

    return bad


def clone(cfg: EepromMount) -> EepromMount:
    return deepcopy(cfg)
