"""
MainUnit (mount) parameter backup/restore.

Compatible with TeenAstroConfig/TAConfig JSON layout:
  [ mount_dict, [ site0, site1, site2, site3 ] ]

Uses individual :GX* / :SX* commands so it works on TeenAstro 1.5 and 1.6.
Mount type / motor-enable writes reboot the board and are optional.
"""

from __future__ import annotations

import base64
import json
import math
import time
from copy import deepcopy
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .serial_lx200 import SerialSession

MOUNT_READ_CMD = {
    "mType": "GXAS",
    "DefaultR": "GXRD",
    "MaxR": "GXRX",
    "GuideR": "GXR0",
    "Acc": "GXRA",
    "SlowR": "GXR1",
    "MediumR": "GXR2",
    "FastR": "GXR3",
    "mrot1": "GXMRR",
    "mge1": "GXMGR",
    "mst1": "GXMSR",
    "mmu1": "GXMMR",
    "mbl1": "GXMBR",
    "mlc1": "GXMcR",
    "mhc1": "GXMCR",
    "msil1": "GXMmR",
    "mrot2": "GXMRD",
    "mge2": "GXMGD",
    "mst2": "GXMSD",
    "mmu2": "GXMMD",
    "mbl2": "GXMBD",
    "mlc2": "GXMcD",
    "mhc2": "GXMCD",
    "msil2": "GXMmD",
    "hl": "GXLH",
    "ol": "GXLO",
    "el": "GXLE",
    "wl": "GXLW",
    "ul": "GXLU",
    "a1min": "GXLA",
    "a1max": "GXLB",
    "a2min": "GXLC",
    "a2max": "GXLD",
    "mEn": "GXJm",
}

DEFAULT_MOUNT: dict[str, Any] = {
    "mType": "Eq-German",
    "DefaultR": "Guide",
    "MaxR": 800,
    "GuideR": 0.5,
    "Acc": "1.0",
    "SlowR": 4,
    "MediumR": 16,
    "FastR": 64,
    "mrot1": "Direct",
    "mge1": 1,
    "mge1f": 0,
    "mst1": "200",
    "mmu1": 16,
    "mbl1": "0",
    "mlc1": 1000,
    "mhc1": 1000,
    "msil1": "0",
    "mrot2": "Direct",
    "mge2": 1,
    "mge2f": 0,
    "mst2": "200",
    "mmu2": 16,
    "mbl2": "0",
    "mlc2": 1000,
    "mhc2": 1000,
    "msil2": "0",
    "hl": "-10",
    "ol": "+90",
    "el": 15,
    "wl": 15,
    "ul": 12,
    "a1min": -360,
    "a1max": 360,
    "a2min": -360,
    "a2max": 360,
    "mEn": True,
}

MTYPE_TO_CMD = {
    "Eq-German": "1",
    "Eq-Fork": "2",
    "AltAz-Tee": "3",
    "AltAz-Fork": "4",
}


@dataclass
class Site:
    name: str = ""
    latitude: list[int] = field(default_factory=lambda: [0, 0])
    longitude: list[int] = field(default_factory=lambda: [0, 0])
    elevation: int = 0
    currentSite: int = 0
    timeZone: float = 0.0

    def serialize(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "latitude": list(self.latitude),
            "longitude": list(self.longitude),
            "elevation": self.elevation,
            "currentSite": self.currentSite,
            "timeZone": self.timeZone,
        }

    def unserialize(self, obj: dict[str, Any]) -> None:
        self.name = obj.get("name", "")
        self.latitude = list(obj.get("latitude", [0, 0]))
        self.longitude = list(obj.get("longitude", [0, 0]))
        self.elevation = int(obj.get("elevation", 0))
        self.currentSite = int(obj.get("currentSite", 0))
        self.timeZone = float(obj.get("timeZone", 0.0))


@dataclass
class MountConfig:
    mount: dict[str, Any] = field(default_factory=lambda: deepcopy(DEFAULT_MOUNT))
    sites: list[Site] = field(default_factory=lambda: [Site() for _ in range(4)])
    meta: dict[str, str] = field(default_factory=dict)
    warnings: list[str] = field(default_factory=list)

    def to_taconfig_json(self) -> list[Any]:
        """TAConfig-compatible object (mount + sites)."""
        return [self.mount, [s.serialize() for s in self.sites]]

    def to_file_dict(self) -> dict[str, Any]:
        return {
            "kind": "TeenAstroMainUnit",
            "format": "taconfig-compatible",
            "meta": self.meta,
            "mount": self.mount,
            "sites": [s.serialize() for s in self.sites],
        }


def _dms_split(dms: str) -> list[int]:
    dms = dms.replace("*", " ").replace("'", " ").replace(":", " ")
    parts = dms.split()
    return [int(x) for x in parts[:2]] if parts else [0, 0]


def _parse_mtype_from_gxas(b64: str) -> str | None:
    if len(b64) < 136:
        return None
    try:
        pkt = base64.b64decode(b64)
        if len(pkt) < 2:
            return None
        mt = (pkt[1] >> 4) & 0x7
        return {
            1: "Eq-German",
            2: "Eq-Fork",
            3: "AltAz-Tee",
            4: "AltAz-Fork",
        }.get(mt)
    except Exception:  # noqa: BLE001
        return None


def read_mount(sess: SerialSession) -> MountConfig:
    cfg = MountConfig()
    cfg.meta["product"] = sess.query("GVP")
    cfg.meta["firmware"] = sess.query("GVN")
    cfg.meta["board"] = sess.query("GVB")
    cfg.meta["driver"] = sess.query("GVb")

    for tag, cmd in MOUNT_READ_CMD.items():
        try:
            resp = sess.query(cmd, max_len=220 if cmd == "GXAS" else 64)
        except Exception as exc:  # noqa: BLE001
            cfg.warnings.append(f"{tag}: {exc}")
            continue
        if resp in ("", "?"):
            cfg.warnings.append(f"{tag}: empty reply ({cmd})")
            continue
        try:
            _apply_read(cfg.mount, tag, resp)
        except Exception as exc:  # noqa: BLE001
            cfg.warnings.append(f"{tag}: parse error ({exc})")

    # Sites (0..2 like TAConfig; slot 3 often unused)
    try:
        cur = sess.query("W?")
        cur_i = int(cur) if cur.isdigit() else 0
        if cur_i not in (0, 1, 2, 3):
            cur_i = 0
        for i in range(3):
            _read_site(sess, cfg.sites[i], i)
        sess.query(f"W{cur_i}")  # restore; no reply expected
        time.sleep(0.1)
    except Exception as exc:  # noqa: BLE001
        cfg.warnings.append(f"sites: {exc}")

    return cfg


def _apply_read(mount: dict[str, Any], tag: str, resp: str) -> None:
    if tag == "mType":
        parsed = _parse_mtype_from_gxas(resp)
        if parsed:
            mount[tag] = parsed
        return
    if tag in ("mlc1", "mlc2", "mhc1", "mhc2"):
        mount[tag] = int(float(resp))
    elif tag in ("mrot1", "mrot2"):
        mount[tag] = "Direct" if resp == "0" else "Reverse"
    elif tag in ("msil1", "msil2"):
        mount[tag] = resp
    elif tag in ("mmu1", "mmu2"):
        mount[tag] = int(math.pow(2, int(resp)))
    elif tag in ("mge1", "mge1f"):
        mount["mge1"] = int(float(resp) / 1000)
        mount["mge1f"] = int(float(resp) % 1000)
    elif tag in ("mge2", "mge2f"):
        mount["mge2"] = int(float(resp) / 1000)
        mount["mge2f"] = int(float(resp) % 1000)
    elif tag in ("el", "wl"):
        mount[tag] = int(int(resp) / 4)
    elif tag == "ul":
        mount[tag] = int(float(resp) / 10)
    elif tag in ("hl", "ol"):
        mount[tag] = resp[:-1] if resp.endswith("*") else resp
    elif tag == "MaxR":
        mount[tag] = int(float(resp))
    elif tag == "GuideR":
        mount[tag] = float(resp)
    elif tag in ("a1min", "a1max", "a2min", "a2max"):
        # Signed degrees (tenths on the wire), same as ASCOM / EEPROM editor.
        mount[tag] = int(round(float(resp) / 10.0))
    elif tag == "DefaultR":
        mount[tag] = {
            "0": "Guide",
            "1": "Slow",
            "2": "Medium",
            "3": "Fast",
            "4": "Max",
        }.get(resp, "Guide")
    elif tag in ("SlowR", "MediumR", "FastR"):
        mount[tag] = int(float(resp))
    elif tag == "mEn":
        mount[tag] = resp == "1"
    else:
        mount[tag] = resp


def _read_site(sess: SerialSession, site: Site, i: int) -> None:
    sess._ser.write(f":W{i}#".encode("ascii"))  # noqa: SLF001
    time.sleep(0.1)
    site.latitude = _dms_split(sess.query("Gt"))
    site.longitude = _dms_split(sess.query("Gg"))
    elev = sess.query("Ge")
    site.elevation = int(elev) if elev.lstrip("+-").isdigit() else 0
    tz = sess.query("GG")
    try:
        site.timeZone = -float(tz)
    except ValueError:
        site.timeZone = 0.0
    name_cmd = {0: "GM", 1: "GN", 2: "GO", 3: "GP"}.get(i, "GM")
    site.name = sess.query(name_cmd)


def write_mount(
    sess: SerialSession,
    cfg: MountConfig,
    *,
    write_mount_type: bool = False,
    write_motor_enable: bool = False,
) -> list[str]:
    """Write mount (+ sites). Returns list of failed commands."""
    errors: list[str] = []
    mount = cfg.mount

    for tag in MOUNT_READ_CMD:
        if tag == "mType":
            continue
        if tag == "mEn":
            continue
        if tag in ("mge1f", "mge2f"):
            continue  # written with mge1/mge2
        try:
            cmd = _build_set_cmd(mount, tag)
            if not cmd:
                continue
            ack = sess.send_ack(cmd)
            if ack and ack[0] not in "01":
                errors.append(f"{tag}: unexpected ack {ack!r}")
        except Exception as exc:  # noqa: BLE001
            errors.append(f"{tag}: {exc}")

    for i, site in enumerate(cfg.sites[:3]):
        try:
            _write_site(sess, site, i)
        except Exception as exc:  # noqa: BLE001
            errors.append(f"site{i}: {exc}")

    if write_motor_enable:
        try:
            sess.send_ack(":SXME,y#" if mount.get("mEn") else ":SXME,n#")
        except Exception as exc:  # noqa: BLE001
            errors.append(f"mEn: {exc}")

    if write_mount_type:
        code = MTYPE_TO_CMD.get(str(mount.get("mType", "")), "")
        if code:
            try:
                # Reboots the board — no reliable ack.
                sess._ser.write(f":S!{code}#".encode("ascii"))  # noqa: SLF001
            except Exception as exc:  # noqa: BLE001
                errors.append(f"mType: {exc}")

    return errors


def _build_set_cmd(mount: dict[str, Any], tag: str) -> str | None:
    prefix = {
        "DefaultR": "SXRD",
        "MaxR": "SXRX",
        "GuideR": "SXR0",
        "Acc": "SXRA",
        "SlowR": "SXR1",
        "MediumR": "SXR2",
        "FastR": "SXR3",
        "mrot1": "SXMRR",
        "mge1": "SXMGR",
        "mst1": "SXMSR",
        "mmu1": "SXMMR",
        "mbl1": "SXMBR",
        "mlc1": "SXMcR",
        "mhc1": "SXMCR",
        "msil1": "SXMmR",
        "mrot2": "SXMRD",
        "mge2": "SXMGD",
        "mst2": "SXMSD",
        "mmu2": "SXMMD",
        "mbl2": "SXMBD",
        "mlc2": "SXMcD",
        "mhc2": "SXMCD",
        "msil2": "SXMmD",
        "hl": "SXLH",
        "ol": "SXLO",
        "el": "SXLE",
        "wl": "SXLW",
        "ul": "SXLU",
        "a1min": "SXLA",
        "a1max": "SXLB",
        "a2min": "SXLC",
        "a2max": "SXLD",
    }.get(tag)
    if not prefix:
        return None

    if tag in ("mlc1", "mlc2", "mhc1", "mhc2", "msil1", "msil2", "mbl1", "mbl2", "mst1", "mst2"):
        val = str(int(mount[tag]))
    elif tag in ("mrot1", "mrot2"):
        val = "0" if mount[tag] == "Direct" else "1"
    elif tag in ("mmu1", "mmu2"):
        val = str(int(math.log(int(mount[tag]), 2)))
    elif tag == "mge1":
        val = str(int(int(mount["mge1"]) * 1000) + int(mount.get("mge1f", 0)))
    elif tag == "mge2":
        val = str(int(int(mount["mge2"]) * 1000) + int(mount.get("mge2f", 0)))
    elif tag in ("el", "wl"):
        val = str(int(int(mount[tag]) * 4))
    elif tag in ("hl", "ol"):
        val = str(int(float(str(mount[tag]).replace("+", ""))))
    elif tag == "ul":
        val = str(int(float(mount[tag]) * 10))
    elif tag == "Acc":
        val = str(int(float(mount[tag]) * 10))
    elif tag in ("a1min", "a1max", "a2min", "a2max"):
        # Signed tenths of a degree (firmware EEPROM short).
        val = str(10 * int(mount[tag]))
    elif tag == "DefaultR":
        val = str({"Guide": 0, "Slow": 1, "Medium": 2, "Fast": 3, "Max": 4}.get(mount[tag], 0))
    elif tag == "GuideR":
        val = str(int(float(mount[tag]) * 100))
    elif tag in ("SlowR", "MediumR", "FastR", "MaxR"):
        val = str(int(mount[tag]))
    else:
        val = str(mount[tag])

    return f":{prefix},{val}#"


def _write_site(sess: SerialSession, site: Site, i: int) -> None:
    sess._ser.write(f":W{i}#".encode("ascii"))  # noqa: SLF001
    time.sleep(0.1)
    lat_s = site.latitude[0]
    lat_m = site.latitude[1] if len(site.latitude) > 1 else 0
    lon_s = site.longitude[0]
    lon_m = site.longitude[1] if len(site.longitude) > 1 else 0
    sess.send_ack(":St%+03d*%02d#" % (lat_s, lat_m))
    sess.send_ack(":Sg%+04d*%02d#" % (lon_s, lon_m))
    sess.send_ack(":Se%+04d#" % int(site.elevation))
    sess.send_ack(":SG%+02.1f#" % (-float(site.timeZone)))
    name_cmd = {0: "SM", 1: "SN", 2: "SO", 3: "SP"}.get(i, "SM")
    sess.send_ack(f":{name_cmd}{site.name}#")


def save_mount(path: Path, cfg: MountConfig, *, taconfig_compat: bool = True) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if taconfig_compat:
        path.write_text(json.dumps(cfg.to_taconfig_json(), indent=4), encoding="utf-8")
    else:
        path.write_text(json.dumps(cfg.to_file_dict(), indent=4), encoding="utf-8")


def _normalize_legacy_axis_mins(mount: dict[str, Any]) -> None:
    """Convert TeenAstroConfig / old-uploader axis mins to signed degrees.

    Legacy code stored ``-(eeprom_tenths/10)`` for a1min/a2min (so a device
    value of -180° became JSON ``180``) and wrote ``10*abs(json)``. That
    flips the sign of axis2 min on restore. ASCOM and the EEPROM editor use
    signed degrees end-to-end.

    Heuristic: axis1 min is almost always negative in EEPROM; a positive
    ``a1min`` in JSON indicates the legacy encoding.
    """
    try:
        a1 = int(mount.get("a1min", 0))
        a2 = int(mount.get("a2min", 0))
    except (TypeError, ValueError):
        return
    if a1 > 0:
        mount["a1min"] = -abs(a1)
        mount["a2min"] = -abs(a2)


def load_mount(path: Path) -> MountConfig:
    data = json.loads(path.read_text(encoding="utf-8"))
    cfg = MountConfig()
    if isinstance(data, list) and len(data) >= 2:
        cfg.mount = {**DEFAULT_MOUNT, **data[0]}
        for i, sobj in enumerate(data[1][:4]):
            cfg.sites[i].unserialize(sobj)
    elif isinstance(data, dict) and "mount" in data:
        cfg.meta = dict(data.get("meta") or {})
        cfg.mount = {**DEFAULT_MOUNT, **data["mount"]}
        for i, sobj in enumerate((data.get("sites") or [])[:4]):
            cfg.sites[i].unserialize(sobj)
    else:
        raise ValueError("Unrecognized mount config JSON")
    _normalize_legacy_axis_mins(cfg.mount)
    return cfg


def verify_mount(sess: SerialSession, expected: MountConfig) -> list[str]:
    """Re-read mount config and return mismatches (used after Auto restore)."""
    got = read_mount(sess)
    bad: list[str] = []
    skip = {"mType", "mEn"}  # type write is optional / may reboot
    for tag in MOUNT_READ_CMD:
        if tag in skip:
            continue
        ev, av = expected.mount.get(tag), got.mount.get(tag)
        try:
            if abs(float(ev) - float(av)) > 1e-2:
                bad.append(f"{tag}: wrote {ev!r}, read {av!r}")
        except (TypeError, ValueError):
            if ev != av:
                bad.append(f"{tag}: wrote {ev!r}, read {av!r}")
    return bad


def format_mount_preview(cfg: MountConfig) -> str:
    lines = [
        f"Product: {cfg.meta.get('product', '?')}",
        f"Firmware: {cfg.meta.get('firmware', '?')}",
        f"Board: {cfg.meta.get('board', '?')}  driver: {cfg.meta.get('driver', '?')}",
        "",
        "--- Mount ---",
    ]
    for k in sorted(cfg.mount.keys()):
        lines.append(f"{k} = {cfg.mount[k]}")
    lines.append("")
    lines.append("--- Sites ---")
    for i, s in enumerate(cfg.sites[:3]):
        lines.append(
            f"[{i}] {s.name!r} lat={s.latitude} lon={s.longitude} "
            f"elev={s.elevation} tz={s.timeZone}"
        )
    if cfg.warnings:
        lines.append("")
        lines.append("--- Warnings ---")
        lines.extend(cfg.warnings)
    return "\n".join(lines)
