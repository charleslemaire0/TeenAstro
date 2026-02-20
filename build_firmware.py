#!/usr/bin/env python3
"""
build_firmware.py  -  TeenAstro Firmware Builder & Distribution Script

Replaces the legacy arduino-cli batch scripts (build_all.bat,
TeenAstroBuilder.bat, TeenAstroSHC_Maker.bat) with a single
PlatformIO-based build that produces correctly-named firmware files
ready for the TeenAstro Uploader.

Usage:
    python build_firmware.py                  # build everything
    python build_firmware.py --target main    # only MainUnit
    python build_firmware.py --target focuser # only Focuser
    python build_firmware.py --target shc     # only SHC
    python build_firmware.py --list           # list all firmware variants
    python build_firmware.py --clean          # clean build dirs first
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
RELEASE_VERSION = "1.5"

# Repo root is the directory containing this script
REPO_ROOT = Path(__file__).resolve().parent

# Distribution folder consumed by the TeenAstro Uploader
DIST_DIR = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader" / f"{RELEASE_VERSION}_latest"


@dataclass
class FirmwareBuild:
    """Describes one firmware variant to build."""
    target: str          # logical group: "main", "focuser", "shc"
    project_dir: str     # PlatformIO project folder relative to REPO_ROOT
    pio_env: str         # PlatformIO environment name
    dist_name: str       # distribution filename (without extension)
    extension: str       # ".hex" or ".bin"


# ---------------------------------------------------------------------------
# Firmware manifest  -  single source of truth
# ---------------------------------------------------------------------------
FIRMWARE_MANIFEST: list[FirmwareBuild] = [
    # ── MainUnit (Teensy .hex) ─────────────────────────────────────────────
    FirmwareBuild("main", "TeenAstroMainUnit", "220",
                  f"TeenAstro_{RELEASE_VERSION}_220_TMC260", ".hex"),
    FirmwareBuild("main", "TeenAstroMainUnit", "230",
                  f"TeenAstro_{RELEASE_VERSION}_230_TMC260", ".hex"),
    FirmwareBuild("main", "TeenAstroMainUnit", "240_2130",
                  f"TeenAstro_{RELEASE_VERSION}_240_TMC2130", ".hex"),
    FirmwareBuild("main", "TeenAstroMainUnit", "240_5160",
                  f"TeenAstro_{RELEASE_VERSION}_240_TMC5160", ".hex"),
    FirmwareBuild("main", "TeenAstroMainUnit", "250_2130",
                  f"TeenAstro_{RELEASE_VERSION}_250_TMC2130", ".hex"),
    FirmwareBuild("main", "TeenAstroMainUnit", "250_5160",
                  f"TeenAstro_{RELEASE_VERSION}_250_TMC5160", ".hex"),

    # ── Focuser (Teensy .hex) ──────────────────────────────────────────────
    FirmwareBuild("focuser", "TeenAstroFocuser", "220_2130",
                  f"TeenAstroFocuser_{RELEASE_VERSION}_220_TMC2130", ".hex"),
    FirmwareBuild("focuser", "TeenAstroFocuser", "230_2130",
                  f"TeenAstroFocuser_{RELEASE_VERSION}_230_TMC2130", ".hex"),
    FirmwareBuild("focuser", "TeenAstroFocuser", "240_2130",
                  f"TeenAstroFocuser_{RELEASE_VERSION}_240_TMC2130", ".hex"),
    FirmwareBuild("focuser", "TeenAstroFocuser", "240_5160",
                  f"TeenAstroFocuser_{RELEASE_VERSION}_240_TMC5160", ".hex"),

    # ── SHC (ESP8266 .bin) ─────────────────────────────────────────────────
    FirmwareBuild("shc", "TeenAstroSHC", "ENGLISH",
                  f"TeenAstroSHC_{RELEASE_VERSION}_English", ".bin"),
    FirmwareBuild("shc", "TeenAstroSHC", "FRENCH",
                  f"TeenAstroSHC_{RELEASE_VERSION}_French", ".bin"),
    FirmwareBuild("shc", "TeenAstroSHC", "GERMAN",
                  f"TeenAstroSHC_{RELEASE_VERSION}_German", ".bin"),
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def run(cmd: list[str], cwd: Path) -> bool:
    """Run a command, stream output, return True on success."""
    print(f"  >> {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=str(cwd))
    return result.returncode == 0


def find_output_file(project_path: Path, pio_env: str, ext: str) -> Path | None:
    """Locate the PlatformIO output file for a given environment.

    PIO places output in  project/pio/<env>/  and the rename scripts
    produce files named  <Prefix>_<customOption>_<env>.<ext>.
    We simply glob for *.<ext> in the env build dir.
    """
    build_dir = project_path / "pio" / pio_env
    if not build_dir.is_dir():
        return None
    candidates = list(build_dir.glob(f"*{ext}"))
    if not candidates:
        return None
    # Return the largest file (the actual firmware, not the bootloader)
    return max(candidates, key=lambda p: p.stat().st_size)


# ---------------------------------------------------------------------------
# Build logic
# ---------------------------------------------------------------------------
def build_and_distribute(fw: FirmwareBuild, clean: bool = False) -> bool:
    """Build one firmware variant and copy it to the distribution folder."""
    project_path = REPO_ROOT / fw.project_dir
    print(f"\n{'='*60}")
    print(f"  Building: {fw.dist_name}{fw.extension}")
    print(f"  Project:  {fw.project_dir}  env={fw.pio_env}")
    print(f"{'='*60}")

    # Optional clean
    if clean:
        run(["pio", "run", "-e", fw.pio_env, "--target", "clean"], cwd=project_path)

    # Build
    if not run(["pio", "run", "-e", fw.pio_env], cwd=project_path):
        print(f"  !! BUILD FAILED: {fw.dist_name}")
        return False

    # Locate output
    output = find_output_file(project_path, fw.pio_env, fw.extension)
    if output is None:
        print(f"  !! OUTPUT NOT FOUND for env {fw.pio_env}")
        return False

    # Copy to distribution
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    dest = DIST_DIR / f"{fw.dist_name}{fw.extension}"
    shutil.copy2(str(output), str(dest))
    size_kb = dest.stat().st_size / 1024
    print(f"  -> Copied to {dest.relative_to(REPO_ROOT)}  ({size_kb:.1f} KB)")
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Build TeenAstro firmware and distribute to the Uploader folder."
    )
    parser.add_argument(
        "--target", "-t",
        choices=["main", "focuser", "shc", "all"],
        default="all",
        help="Which firmware target to build (default: all)"
    )
    parser.add_argument(
        "--list", "-l",
        action="store_true",
        help="List all firmware variants without building"
    )
    parser.add_argument(
        "--clean", "-c",
        action="store_true",
        help="Clean build directories before building"
    )
    args = parser.parse_args()

    # Filter manifest by target
    if args.target == "all":
        builds = FIRMWARE_MANIFEST
    else:
        builds = [fw for fw in FIRMWARE_MANIFEST if fw.target == args.target]

    # List mode
    if args.list:
        print(f"TeenAstro Firmware Manifest  (release {RELEASE_VERSION})")
        print(f"Distribution folder: {DIST_DIR.relative_to(REPO_ROOT)}\n")
        for fw in builds:
            print(f"  [{fw.target:8s}]  {fw.project_dir:20s}  env={fw.pio_env:12s}  -> {fw.dist_name}{fw.extension}")
        print(f"\nTotal: {len(builds)} variants")
        return

    # Build
    print(f"TeenAstro Firmware Builder  (release {RELEASE_VERSION})")
    print(f"Distribution folder: {DIST_DIR}")
    print(f"Variants to build: {len(builds)}")

    succeeded = 0
    failed = 0
    failures = []

    for fw in builds:
        if build_and_distribute(fw, clean=args.clean):
            succeeded += 1
        else:
            failed += 1
            failures.append(fw.dist_name)

    # Summary
    print(f"\n{'='*60}")
    print(f"  BUILD COMPLETE")
    print(f"  Succeeded: {succeeded}  Failed: {failed}")
    if failures:
        print(f"  Failed builds:")
        for name in failures:
            print(f"    - {name}")
    print(f"  Output: {DIST_DIR}")
    print(f"{'='*60}")

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
