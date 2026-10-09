"""
Focuser parameter backup/restore via :F~# / :FM# and :F0..:F8 / :Fc / :Fm / :Fr.
Works across TeenAstro Focuser 1.5 and 1.6 text commands.
"""

from __future__ import annotations

import json
from copy import deepcopy
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .serial_lx200 import SerialSession

DEFAULT_FOCUSER: dict[str, Any] = {
    "parkPos": 0,
    "maxPos": 50000,
    "minSpeed": 1,
    "maxSpeed": 20,
    "cmdAcc": 10,
    "manAcc": 10,
    "manDec": 10,
    "reverse": 0,
    "micro": 16,
    "resolution": 1,
    "current": 100,
    "steprot": 200,
}


@dataclass
class FocuserConfig:
    settings: dict[str, Any] = field(default_factory=lambda: deepcopy(DEFAULT_FOCUSER))
    meta: dict[str, str] = field(default_factory=dict)
    warnings: list[str] = field(default_factory=list)

    def to_file_dict(self) -> dict[str, Any]:
        return {
            "kind": "TeenAstroFocuser",
            "meta": self.meta,
            "settings": self.settings,
        }


def read_focuser(sess: SerialSession) -> FocuserConfig:
    cfg = FocuserConfig()
    fv = sess.query("FV")
    cfg.meta["fv"] = fv
    if fv and "TeenAstro Focuser" in fv:
        parts = fv.split()
        for i, p in enumerate(parts):
            if p == "Focuser" and i + 1 < len(parts):
                cfg.meta["board"] = parts[i + 1]
                if i + 2 < len(parts):
                    cfg.meta["firmware"] = parts[i + 2]
                break

    tilde = sess.query("F~")
    if tilde.startswith("~"):
        parts = tilde[1:].split()
        # ~park max lowS highS cmdAcc manAcc [manDec]
        keys = ["parkPos", "maxPos", "minSpeed", "maxSpeed", "cmdAcc", "manAcc", "manDec"]
        for i, key in enumerate(keys):
            if i < len(parts):
                try:
                    cfg.settings[key] = int(float(parts[i]))
                except ValueError:
                    cfg.warnings.append(f"{key}: bad value {parts[i]!r}")
    else:
        cfg.warnings.append(f"F~: unexpected reply {tilde!r}")

    motor = sess.query("FM")
    if motor.startswith("M"):
        parts = motor[1:].split()
        # Mrev micro resolution current [steprot]
        try:
            if len(parts) >= 1:
                cfg.settings["reverse"] = int(parts[0])
            if len(parts) >= 2:
                cfg.settings["micro"] = int(parts[1])
            if len(parts) >= 3:
                cfg.settings["resolution"] = int(parts[2])
            if len(parts) >= 4:
                cfg.settings["current"] = int(parts[3])
            if len(parts) >= 5:
                cfg.settings["steprot"] = int(parts[4])
        except ValueError as exc:
            cfg.warnings.append(f"FM: parse error ({exc})")
    else:
        cfg.warnings.append(f"FM: unexpected reply {motor!r}")

    return cfg


def write_focuser(sess: SerialSession, cfg: FocuserConfig) -> list[str]:
    s = cfg.settings
    errors: list[str] = []
    writes = [
        ("0", s["parkPos"]),
        ("1", s["maxPos"]),
        ("2", s["minSpeed"]),
        ("3", s["maxSpeed"]),
        ("4", s["cmdAcc"]),
        ("5", s["manAcc"]),
        ("6", s.get("manDec", s["manAcc"])),
        ("7", int(s["reverse"])),
        ("8", s["resolution"]),
        ("c", s["current"]),
        ("m", s["micro"]),
        ("r", s.get("steprot", 200)),
    ]
    for index, value in writes:
        cmd = f":F{index},{int(value)}#"
        try:
            ack = sess.send_ack(cmd)
            if ack and ack[0] != "1":
                errors.append(f"F{index}: ack={ack!r}")
        except Exception as exc:  # noqa: BLE001
            errors.append(f"F{index}: {exc}")
    return errors


def save_focuser(path: Path, cfg: FocuserConfig) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(cfg.to_file_dict(), indent=4), encoding="utf-8")


def load_focuser(path: Path) -> FocuserConfig:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or data.get("kind") not in (None, "TeenAstroFocuser"):
        # Allow bare settings dict
        if isinstance(data, dict) and "parkPos" in data:
            cfg = FocuserConfig(settings={**DEFAULT_FOCUSER, **data})
            return cfg
        if isinstance(data, dict) and "settings" in data:
            pass
        else:
            raise ValueError("Unrecognized focuser config JSON")
    cfg = FocuserConfig()
    cfg.meta = dict(data.get("meta") or {})
    cfg.settings = {**DEFAULT_FOCUSER, **(data.get("settings") or {})}
    return cfg


def verify_focuser(sess: SerialSession, expected: FocuserConfig) -> list[str]:
    """Re-read focuser and return mismatches."""
    got = read_focuser(sess)
    bad: list[str] = []
    for key in DEFAULT_FOCUSER:
        if expected.settings.get(key) != got.settings.get(key):
            bad.append(
                f"{key}: wrote {expected.settings.get(key)!r}, "
                f"read {got.settings.get(key)!r}"
            )
    return bad


def format_focuser_preview(cfg: FocuserConfig) -> str:
    lines = [
        f"FV: {cfg.meta.get('fv', '?')}",
        f"Board: {cfg.meta.get('board', '?')}  Firmware: {cfg.meta.get('firmware', '?')}",
        "",
        "--- Focuser ---",
    ]
    for k in (
        "parkPos",
        "maxPos",
        "minSpeed",
        "maxSpeed",
        "cmdAcc",
        "manAcc",
        "manDec",
        "reverse",
        "micro",
        "resolution",
        "current",
        "steprot",
    ):
        lines.append(f"{k} = {cfg.settings.get(k)}")
    if cfg.warnings:
        lines.append("")
        lines.append("--- Warnings ---")
        lines.extend(cfg.warnings)
    return "\n".join(lines)
