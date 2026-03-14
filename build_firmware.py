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
RELEASE_VERSION = "1.6"

# Repo root is the directory containing this script
REPO_ROOT = Path(__file__).resolve().parent

# Uploader uses X.Y only: folder is {X.Y}_latest (rolling) or {X.Y} (stable after promote).
# E.g. 1.6.1 builds go to 1.6_latest; filenames may still be TeenAstro_1.6.1_...
def _release_line(version: str) -> str:
    """Return X.Y from X.Y or X.Y.Z (folder name for uploader)."""
    parts = version.strip().split(".")
    return ".".join(parts[:2]) if len(parts) >= 2 else version

# Distribution folder = rolling "latest" for this release line (X.Y_latest)
DIST_DIR = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader" / f"{_release_line(RELEASE_VERSION)}_latest"


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
def get_manifest_for_version(version: str) -> tuple[Path, list[FirmwareBuild]]:
    """Return (dist_dir, list of FirmwareBuild) for the given version.
    Uploader uses X.Y only: folder is X.Y_latest (or X.Y after promote), filenames use X.Y.
    """
    rl = _release_line(version)
    dist_dir = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader" / f"{rl}_latest"
    manifest = [
        FirmwareBuild("main", "TeenAstroMainUnit", "220", f"TeenAstro_{rl}_220_TMC260", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "230", f"TeenAstro_{rl}_230_TMC260", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "240_2130", f"TeenAstro_{rl}_240_TMC2130", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "240_5160", f"TeenAstro_{rl}_240_TMC5160", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "250_2130", f"TeenAstro_{rl}_250_TMC2130", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "250_5160", f"TeenAstro_{rl}_250_TMC5160", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "220_2130", f"TeenAstroFocuser_{rl}_220_TMC2130", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "230_2130", f"TeenAstroFocuser_{rl}_230_TMC2130", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "240_2130", f"TeenAstroFocuser_{rl}_240_TMC2130", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "240_5160", f"TeenAstroFocuser_{rl}_240_TMC5160", ".hex"),
        FirmwareBuild("shc", "TeenAstroSHC", "ENGLISH", f"TeenAstroSHC_{rl}_English", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "FRENCH", f"TeenAstroSHC_{rl}_French", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "GERMAN", f"TeenAstroSHC_{rl}_German", ".bin"),
    ]
    return dist_dir, manifest


def build_and_distribute(fw: FirmwareBuild, clean: bool = False, dist_dir: Path | None = None) -> bool:
    """Build one firmware variant and copy it to the distribution folder."""
    out_dir = dist_dir if dist_dir is not None else DIST_DIR
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
    out_dir.mkdir(parents=True, exist_ok=True)
    dest = out_dir / f"{fw.dist_name}{fw.extension}"
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
    parser.add_argument(
        "--version", "-v",
        metavar="X.Y[.Z]",
        help="Override release version (e.g. 1.6.1). Used by publish script."
    )
    args = parser.parse_args()

    version = (args.version if args.version else RELEASE_VERSION).strip()
    if args.version:
        dist_dir, manifest = get_manifest_for_version(version)
        builds = manifest if args.target == "all" else [fw for fw in manifest if fw.target == args.target]
    else:
        dist_dir = DIST_DIR
        builds = FIRMWARE_MANIFEST if args.target == "all" else [fw for fw in FIRMWARE_MANIFEST if fw.target == args.target]

    # List mode
    if args.list:
        print(f"TeenAstro Firmware Manifest  (release {version})")
        print(f"Distribution folder: {dist_dir.relative_to(REPO_ROOT)}\n")
        for fw in builds:
            print(f"  [{fw.target:8s}]  {fw.project_dir:20s}  env={fw.pio_env:12s}  -> {fw.dist_name}{fw.extension}")
        print(f"\nTotal: {len(builds)} variants")
        return

    # Build
    print(f"TeenAstro Firmware Builder  (release {version})")
    print(f"Distribution folder: {dist_dir}")
    print(f"Variants to build: {len(builds)}")

    succeeded = 0
    failed = 0
    failures = []

    for fw in builds:
        if build_and_distribute(fw, clean=args.clean, dist_dir=dist_dir):
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
    print(f"  Output: {dist_dir}")
    print(f"{'='*60}")

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
