"""Firmware file naming and board mapping."""

from __future__ import annotations

from pathlib import Path

from .paths import firmware_dir


def firmware_file_list(version: str) -> list[str]:
    return [
        f"TeenAstroFocuser_{version}_220_TMC2130.hex",
        f"TeenAstroFocuser_{version}_230_TMC2130.hex",
        f"TeenAstroFocuser_{version}_240_TMC2130.hex",
        f"TeenAstroFocuser_{version}_240_TMC5160.hex",
        f"TeenAstroSHC_{version}_English.bin",
        f"TeenAstroSHC_{version}_French.bin",
        f"TeenAstroSHC_{version}_German.bin",
        f"TeenAstro_{version}_220_TMC260.hex",
        f"TeenAstro_{version}_230_TMC260.hex",
        f"TeenAstro_{version}_240_TMC2130.hex",
        f"TeenAstro_{version}_240_TMC5160.hex",
        f"TeenAstro_{version}_250_TMC2130.hex",
        f"TeenAstro_{version}_250_TMC5160.hex",
    ]


def github_firmware_url(version: str, latest: bool, filename: str) -> str:
    verdir = f"{version}_latest" if latest else version
    return (
        f"https://github.com/charleslemaire0/TeenAstro/raw/Release_{version}/"
        f"TeenAstroUploader/TeenAstroUploader/{verdir}/{filename}"
    )


def mainunit_hex_stem(version: str, pcb: str) -> str | None:
    mapping = {
        "2.2 TMC260": f"TeenAstro_{version}_220_TMC260",
        "2.3 TMC260": f"TeenAstro_{version}_230_TMC260",
        "2.4 TMC2130": f"TeenAstro_{version}_240_TMC2130",
        "2.4 TMC5160": f"TeenAstro_{version}_240_TMC5160",
        "2.5 TMC2130": f"TeenAstro_{version}_250_TMC2130",
        "2.5 TMC5160": f"TeenAstro_{version}_250_TMC5160",
    }
    return mapping.get(pcb)


def focuser_hex_stem(version: str, pcb: str) -> str | None:
    mapping = {
        "2.2 TMC2130": f"TeenAstroFocuser_{version}_220_TMC2130",
        "2.3 TMC2130": f"TeenAstroFocuser_{version}_230_TMC2130",
        "2.4 TMC2130": f"TeenAstroFocuser_{version}_240_TMC2130",
        "2.4 TMC5160": f"TeenAstroFocuser_{version}_240_TMC5160",
    }
    return mapping.get(pcb)


def mainunit_mcu(pcb: str) -> str:
    if pcb.startswith("2.5"):
        return "TEENSY40"
    return "TEENSY31"


def focuser_mcu(_pcb: str) -> str:
    return "TEENSY31"


def resolve_hex(version: str, latest: bool, stem: str) -> Path:
    path = firmware_dir(version, latest) / f"{stem}.hex"
    if not path.is_file():
        raise FileNotFoundError(f"{stem}.hex not found!\nExpected at:\n{path}")
    return path


def resolve_shc_bin(version: str, latest: bool, language: str) -> Path:
    path = firmware_dir(version, latest) / f"TeenAstroSHC_{version}_{language}.bin"
    if not path.is_file():
        raise FileNotFoundError(f"{path} not found!")
    return path
