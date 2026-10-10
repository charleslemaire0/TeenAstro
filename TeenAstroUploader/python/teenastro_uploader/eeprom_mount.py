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

MICROSTEPS = (8, 16, 32, 64, 128, 256)
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
    deg_acc: int = 10  # tenths of degree (GXRA raw)

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


def _q(sess: SerialSession, cmd: str, max_len: int = 64) -> str:
    return sess.query(cmd, max_len=max_len)


def _read_motor(sess: SerialSession, suffix: str) -> MotorAxis:
    m = MotorAxis()
    raw_gear = _as_int(_q(sess, f"GXMG{suffix}"))
    m.gear = round(raw_gear / 1000.0, 3)
    m.steps = _as_int(_q(sess, f"GXMS{suffix}"), 200)
    micro_exp = _as_int(_q(sess, f"GXMM{suffix}"), 4)
    micro_exp = max(0, min(8, micro_exp))
    m.micro = 1 << micro_exp
    if m.micro not in MICROSTEPS:
        m.micro = 16
    m.reverse = _q(sess, f"GXMR{suffix}") != "0"
    m.low_curr = _as_int(_q(sess, f"GXMc{suffix}"), 1000)
    m.high_curr = _as_int(_q(sess, f"GXMC{suffix}"), 1000)
    m.backlash = _as_int(_q(sess, f"GXMB{suffix}"))
    m.backlash_rate = _as_int(_q(sess, f"GXMb{suffix}"), 16)
    m.silent = _q(sess, f"GXMm{suffix}") != "0"
    return m


def read_eeprom(sess: SerialSession) -> EepromMount:
    cfg = EepromMount()
    cfg.meta["product"] = _q(sess, "GVP")
    cfg.meta["firmware"] = _q(sess, "GVN")
    cfg.meta["board"] = _q(sess, "GVB")
    cfg.meta["driver"] = _q(sess, "GVb")

    idx = _as_int(_q(sess, "GXOI"))
    cfg.mount_index = 0 if idx not in (0, 1) else idx
    cfg.mount_name = _q(sess, "GXOA")
    cfg.mount_name_0 = _q(sess, "GXOB")
    cfg.mount_name_1 = _q(sess, "GXOC")
    cfg.slew_settle = _as_int(_q(sess, "GXOS"))

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

    cfg.axis1 = _read_motor(sess, "R")
    cfg.axis2 = _read_motor(sess, "D")

    cfg.guide_rate = _as_float(_q(sess, "GXR0"), 0.5)
    cfg.rate1 = _as_int(_q(sess, "GXR1"), 4)
    cfg.rate2 = _as_int(_q(sess, "GXR2"), 16)
    cfg.rate3 = _as_int(_q(sess, "GXR3"), 64)
    cfg.max_rate = _as_int(_q(sess, "GXRX"), 800)
    cfg.default_rate = _as_int(_q(sess, "GXRD"))
    cfg.deg_acc = _as_int(_q(sess, "GXRA"), 10)

    cfg.horizon = _as_int(_q(sess, "GXLH"), -10)
    cfg.overhead = _as_int(_q(sess, "GXLO"), 90)
    # Axis limits: stored as tenths of a degree (signed, same as ASCOM)
    cfg.axis1_min = int(round(_as_int(_q(sess, "GXLA")) / 10.0))
    cfg.axis1_max = int(round(_as_int(_q(sess, "GXLB")) / 10.0))
    cfg.axis2_min = int(round(_as_int(_q(sess, "GXLC")) / 10.0))
    cfg.axis2_max = int(round(_as_int(_q(sess, "GXLD")) / 10.0))
    # Meridian: minutes past → degrees (min * 15 / 60 = /4)
    cfg.meridian_e = int(round(_as_int(_q(sess, "GXLE")) / 4.0))
    cfg.meridian_w = int(round(_as_int(_q(sess, "GXLW")) / 4.0))
    cfg.under_pole = round(_as_int(_q(sess, "GXLU")) / 10.0, 1)
    cfg.dist_from_pole = _as_int(_q(sess, "GXLS"), 181)

    cfg.enc1_pulse = _as_int(_q(sess, "GXEPR"))
    cfg.enc1_reverse = _q(sess, "GXErR") != "0"
    cfg.enc2_pulse = _as_int(_q(sess, "GXEPD"))
    cfg.enc2_reverse = _q(sess, "GXErD") != "0"
    cfg.enc_sync = _as_int(_q(sess, "GXEO"))

    cfg.refr_goto = _q(sess, "GXrg").lower().startswith("y")
    cfg.refr_pole = _q(sess, "GXrp").lower().startswith("y")
    cfg.refr_tracking = _q(sess, "GXrt").lower().startswith("y")

    cfg.latitude = _q(sess, "Gt") or "+00*00"
    cfg.longitude = _q(sess, "Gg") or "+000*00"
    cfg.elevation = _as_int(_q(sess, "Ge"))
    try:
        cfg.timezone = -_as_float(_q(sess, "GG"))
    except Exception:  # noqa: BLE001
        cfg.timezone = 0.0

    return cfg


def _ack(sess: SerialSession, cmd: str, errors: list[str], tag: str) -> None:
    try:
        resp = sess.send_ack(cmd)
        if resp and resp[0] not in "01":
            errors.append(f"{tag}: unexpected ack {resp!r}")
    except Exception as exc:  # noqa: BLE001
        errors.append(f"{tag}: {exc}")


def _write_motor(sess: SerialSession, axis: MotorAxis, suffix: str, errors: list[str]) -> None:
    gear_raw = int(round(float(axis.gear) * 1000))
    _ack(sess, f":SXMG{suffix},{gear_raw}#", errors, f"gear{suffix}")
    _ack(sess, f":SXMS{suffix},{int(axis.steps)}#", errors, f"steps{suffix}")
    micro = int(axis.micro)
    if micro < 1:
        micro = 1
    exp = int(round(math.log(micro, 2)))
    _ack(sess, f":SXMM{suffix},{exp}#", errors, f"micro{suffix}")
    _ack(sess, f":SXMR{suffix},{'1' if axis.reverse else '0'}#", errors, f"rev{suffix}")
    _ack(sess, f":SXMc{suffix},{int(axis.low_curr)}#", errors, f"lc{suffix}")
    _ack(sess, f":SXMC{suffix},{int(axis.high_curr)}#", errors, f"hc{suffix}")
    _ack(sess, f":SXMB{suffix},{int(axis.backlash)}#", errors, f"bl{suffix}")
    _ack(sess, f":SXMb{suffix},{int(axis.backlash_rate)}#", errors, f"blr{suffix}")
    _ack(sess, f":SXMm{suffix},{'1' if axis.silent else '0'}#", errors, f"sil{suffix}")


def write_eeprom(
    sess: SerialSession,
    cfg: EepromMount,
    *,
    write_mount_type: bool = False,
) -> list[str]:
    """Write EEPROM fields. Returns list of errors (empty = OK)."""
    errors: list[str] = []

    _ack(sess, f":SXOA,{cfg.mount_name}#", errors, "mount_name")
    _ack(sess, f":SXOS,{int(cfg.slew_settle)}#", errors, "slew_settle")

    _write_motor(sess, cfg.axis1, "R", errors)
    _write_motor(sess, cfg.axis2, "D", errors)

    guide_enc = int(round(float(cfg.guide_rate) * 100))
    _ack(sess, f":SXR0,{guide_enc:03d}#", errors, "guide")
    _ack(sess, f":SXR1,{int(cfg.rate1)}#", errors, "rate1")
    _ack(sess, f":SXR2,{int(cfg.rate2)}#", errors, "rate2")
    _ack(sess, f":SXR3,{int(cfg.rate3)}#", errors, "rate3")
    _ack(sess, f":SXRX,{int(cfg.max_rate)}#", errors, "max_rate")
    _ack(sess, f":SXRD,{int(cfg.default_rate)}#", errors, "default_rate")
    _ack(sess, f":SXRA,{int(cfg.deg_acc)}#", errors, "deg_acc")

    _ack(sess, f":SXLH,{int(cfg.horizon)}#", errors, "horizon")
    _ack(sess, f":SXLO,{int(cfg.overhead)}#", errors, "overhead")
    _ack(sess, f":SXLA,{10 * int(cfg.axis1_min)}#", errors, "a1min")
    _ack(sess, f":SXLB,{10 * int(cfg.axis1_max)}#", errors, "a1max")
    _ack(sess, f":SXLC,{10 * int(cfg.axis2_min)}#", errors, "a2min")
    _ack(sess, f":SXLD,{10 * int(cfg.axis2_max)}#", errors, "a2max")
    _ack(sess, f":SXLE,{int(cfg.meridian_e) * 4}#", errors, "mer_e")
    _ack(sess, f":SXLW,{int(cfg.meridian_w) * 4}#", errors, "mer_w")
    _ack(sess, f":SXLU,{int(round(float(cfg.under_pole) * 10))}#", errors, "under_pole")
    _ack(sess, f":SXLS,{int(cfg.dist_from_pole)}#", errors, "dist_pole")

    _ack(sess, f":SXEPR,{int(cfg.enc1_pulse)}#", errors, "enc1p")
    _ack(sess, f":SXErR,{'1' if cfg.enc1_reverse else '0'}#", errors, "enc1r")
    _ack(sess, f":SXEPD,{int(cfg.enc2_pulse)}#", errors, "enc2p")
    _ack(sess, f":SXErD,{'1' if cfg.enc2_reverse else '0'}#", errors, "enc2r")
    _ack(sess, f":SXEO,{int(cfg.enc_sync)}#", errors, "enc_sync")

    _ack(sess, f":SXrg,{'y' if cfg.refr_goto else 'n'}#", errors, "refr_g")
    _ack(sess, f":SXrp,{'y' if cfg.refr_pole else 'n'}#", errors, "refr_p")
    _ack(sess, f":SXrt,{'y' if cfg.refr_tracking else 'n'}#", errors, "refr_t")

    # Site (current site only — matches ASCOM EEPROM form)
    lat = cfg.latitude.replace(":", "*").replace("'", "").strip()
    lon = cfg.longitude.replace(":", "*").replace("'", "").strip()
    _ack(sess, f":St{lat}#", errors, "latitude")
    _ack(sess, f":Sg{lon}#", errors, "longitude")
    _ack(sess, f":Se{int(cfg.elevation):+04d}#", errors, "elevation")
    _ack(sess, f":SG{-float(cfg.timezone):+02.1f}#", errors, "timezone")

    if write_mount_type:
        code = MTYPE_TO_CMD.get(cfg.mtype, "")
        if code:
            try:
                sess._ser.write(f":S!{code}#".encode("ascii"))  # noqa: SLF001
            except Exception as exc:  # noqa: BLE001
                errors.append(f"mtype: {exc}")

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

    return bad


def clone(cfg: EepromMount) -> EepromMount:
    return deepcopy(cfg)
