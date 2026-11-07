"""Firmware file naming and board mapping."""

from __future__ import annotations

from pathlib import Path

from .paths import firmware_dir


def hex_version_token(version: str) -> str:
    """X_Y token used in Teensy hex names.

    teensy_post_compile cuts -file at the first '.'. The uploader version
    stays 1.6; the file is TeenAstro_1_6_.... A patch such as 1.6.15 uses 1_6.
    """
    parts = version.strip().split(".")
    line = ".".join(parts[:2]) if len(parts) >= 2 else version.strip()
    return line.replace(".", "_")


def firmware_file_list(version: str) -> list[str]:
    token = hex_version_token(version)
    return [
        f"TeenAstroFocuser_{token}_220_TMC2130.hex",
        f"TeenAstroFocuser_{token}_230_TMC2130.hex",
        f"TeenAstroFocuser_{token}_240_TMC2130.hex",
        f"TeenAstroFocuser_{token}_240_TMC5160.hex",
        f"TeenAstroSHC_{version}_English.bin",
        f"TeenAstroSHC_{version}_French.bin",
        f"TeenAstroSHC_{version}_German.bin",
        f"TeenAstroSHC_{version}_S3_English.bin",
        f"TeenAstroSHC_{version}_S3_French.bin",
        f"TeenAstroSHC_{version}_S3_German.bin",
        f"TeenAstroSHC_{version}_S3_English_OTA.bin",
        f"TeenAstroSHC_{version}_S3_French_OTA.bin",
        f"TeenAstroSHC_{version}_S3_German_OTA.bin",
        f"TeenAstro_{token}_220_TMC260.hex",
        f"TeenAstro_{token}_230_TMC260.hex",
        f"TeenAstro_{token}_240_TMC2130.hex",
        f"TeenAstro_{token}_240_TMC5160.hex",
        f"TeenAstro_{token}_250_TMC2130.hex",
        f"TeenAstro_{token}_250_TMC5160.hex",
    ]


def github_firmware_url(version: str, latest: bool, filename: str) -> str:
    verdir = f"{version}_latest" if latest else version
    return (
        f"https://github.com/charleslemaire0/TeenAstro/raw/Release_{version}/"
        f"TeenAstroUploader/TeenAstroUploader/{verdir}/{filename}"
    )


def mainunit_hex_stem(version: str, pcb: str) -> str | None:
    token = hex_version_token(version)
    mapping = {
        "2.2 TMC260": f"TeenAstro_{token}_220_TMC260",
        "2.3 TMC260": f"TeenAstro_{token}_230_TMC260",
        "2.4 TMC2130": f"TeenAstro_{token}_240_TMC2130",
        "2.4 TMC5160": f"TeenAstro_{token}_240_TMC5160",
        "2.5 TMC2130": f"TeenAstro_{token}_250_TMC2130",
        "2.5 TMC5160": f"TeenAstro_{token}_250_TMC5160",
    }
    return mapping.get(pcb)


def focuser_hex_stem(version: str, pcb: str) -> str | None:
    token = hex_version_token(version)
    mapping = {
        "2.2 TMC2130": f"TeenAstroFocuser_{token}_220_TMC2130",
        "2.3 TMC2130": f"TeenAstroFocuser_{token}_230_TMC2130",
        "2.4 TMC2130": f"TeenAstroFocuser_{token}_240_TMC2130",
        "2.4 TMC5160": f"TeenAstroFocuser_{token}_240_TMC5160",
    }
    return mapping.get(pcb)


def mainunit_mcu(pcb: str) -> str:
    if pcb.startswith("2.5"):
        return "TEENSY40"
    return "TEENSY31"


def focuser_mcu(_pcb: str) -> str:
    return "TEENSY31"


def resolve_hex(version: str, latest: bool, stem: str) -> Path:
    folder = firmware_dir(version, latest)
    path = folder / f"{stem}.hex"
    if path.is_file():
        return path
    # Files published before the underscore token: TeenAstro_1.6_....
    legacy = stem.replace(hex_version_token(version), version.strip(), 1)
    legacy_path = folder / f"{legacy}.hex"
    if legacy != stem and legacy_path.is_file():
        return legacy_path
    raise FileNotFoundError(f"{stem}.hex not found!\nExpected at:\n{path}")


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
