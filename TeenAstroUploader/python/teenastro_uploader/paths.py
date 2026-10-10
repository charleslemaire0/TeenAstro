"""Filesystem helpers for firmware storage and bundled tools."""

from __future__ import annotations

import os
import sys
from pathlib import Path


APP_NAME = "TeenAstro Firmware Uploader"
FIRMWARE_VERSIONS = ("1.6",)

MAINUNIT_PCBS = (
    "2.2 TMC260",
    "2.3 TMC260",
    "2.4 TMC2130",
    "2.4 TMC5160",
    "2.5 TMC2130",
    "2.5 TMC5160",
)

FOCUSER_PCBS = (
    "2.2 TMC2130",
    "2.3 TMC2130",
    "2.4 TMC2130",
    "2.4 TMC5160",
)

SHC_PCBS = ("0.x",)
LANGUAGES = ("English", "French", "German")


def _app_data_root() -> Path:
    if sys.platform == "win32":
        return Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support"
    return Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local" / "share"))


def firmware_base_path() -> Path:
    """Writable firmware cache: %LocalAppData%/TeenAstro/Firmware (or XDG on Unix)."""
    base = _app_data_root() / "TeenAstro" / "Firmware"
    base.mkdir(parents=True, exist_ok=True)
    return base


def config_base_path() -> Path:
    """Writable folder for mount/focuser parameter backups (.json)."""
    base = _app_data_root() / "TeenAstro" / "Config"
    base.mkdir(parents=True, exist_ok=True)
    return base


def firmware_dir(version: str, latest: bool) -> Path:
    name = f"{version}_latest" if latest else version
    path = firmware_base_path() / name
    path.mkdir(parents=True, exist_ok=True)
    return path


def app_dir() -> Path:
    """Directory containing the running app / frozen executable."""
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parent.parent


def tools_dir() -> Path:
    """Bundled native tools (teensy_*, esptool on Windows legacy layout)."""
    candidates = (
        app_dir() / "tools",
        app_dir(),
        Path(__file__).resolve().parent.parent / "tools",
    )
    for candidate in candidates:
        if candidate.is_dir():
            return candidate
    return app_dir()


def which_tool(name: str) -> Path | None:
    """Find a tool next to the app, in ./tools, or on PATH."""
    exe = name if sys.platform != "win32" or name.lower().endswith(".exe") else f"{name}.exe"
    for folder in (tools_dir(), app_dir()):
        candidate = folder / exe
        if candidate.is_file():
            return candidate
    from shutil import which

    found = which(name) or which(exe)
    return Path(found) if found else None


def app_icon_path() -> Path | None:
    """TeenAstro window/exe icon (.ico)."""
    meipass = getattr(sys, "_MEIPASS", None)
    candidates = [
        app_dir() / "icon.ico",
        Path(__file__).resolve().parent.parent / "packaging" / "icon.ico",
    ]
    if meipass:
        candidates.insert(0, Path(meipass) / "icon.ico")
    for path in candidates:
        if path.is_file():
            return path
    return None
