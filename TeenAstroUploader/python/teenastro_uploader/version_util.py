"""Firmware version parsing helpers."""

from __future__ import annotations

import re


def parse_firmware_version(text: str) -> tuple[int, int, int] | None:
    """Parse strings like '1.5', '1.5.2', '1.6.0' → (major, minor, patch)."""
    if not text:
        return None
    m = re.search(r"(\d+)\.(\d+)(?:\.(\d+))?", text.strip())
    if not m:
        return None
    return int(m.group(1)), int(m.group(2)), int(m.group(3) or 0)


def firmware_at_least(text: str, major: int, minor: int, patch: int = 0) -> bool:
    ver = parse_firmware_version(text)
    if ver is None:
        return False
    return ver >= (major, minor, patch)


def firmware_supported_for_config(text: str) -> bool:
    """Parameter backup/restore and Auto full loop need firmware ≥ 1.5."""
    return firmware_at_least(text, 1, 5, 0)
