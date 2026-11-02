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
        f"TeenAstroSHC_{version}_S3_English.bin",
        f"TeenAstroSHC_{version}_S3_French.bin",
        f"TeenAstroSHC_{version}_S3_German.bin",
        f"TeenAstroSHC_{version}_S3_English_OTA.bin",
        f"TeenAstroSHC_{version}_S3_French_OTA.bin",
        f"TeenAstroSHC_{version}_S3_German_OTA.bin",
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


def shc_is_s3(pcb: str) -> bool:
    return pcb == "S3"


def shc_chip(pcb: str) -> str:
    """esptool --chip value for a hand-controller board choice."""
    return "esp32s3" if shc_is_s3(pcb) else "esp8266"


def shc_bin_filename(version: str, language: str, pcb: str = "0.x") -> str:
    """USB flash image. S3 is the merged image written at 0x0, not the _OTA app."""
    if shc_is_s3(pcb):
        return f"TeenAstroSHC_{version}_S3_{language}.bin"
    return f"TeenAstroSHC_{version}_{language}.bin"


def shc_ota_filename(version: str, language: str) -> str:
    """ESP32-S3 application image for the web /update page."""
    return f"TeenAstroSHC_{version}_S3_{language}_OTA.bin"


def resolve_shc_bin(version: str, latest: bool, language: str, pcb: str = "0.x") -> Path:
    path = firmware_dir(version, latest) / shc_bin_filename(version, language, pcb)
    if not path.is_file():
        raise FileNotFoundError(f"{path} not found!")
    return path
