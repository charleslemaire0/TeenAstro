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
    python build_firmware.py --target shc     # only SHC (ESP8266 + ESP32-S3)
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
# Published file names use X_Y: TeenAstro_1_6_....hex and TeenAstroSHC_1_6_....bin.
# 1.6 and 1.6.1 both land in 1.6_latest.
def _release_line(version: str) -> str:
    """Return X.Y from X.Y or X.Y.Z (folder name for uploader)."""
    parts = version.strip().split(".")
    return ".".join(parts[:2]) if len(parts) >= 2 else version


def _file_token(version: str) -> str:
    """X_Y token for every published firmware name. 1.6 and 1.6.15 both become 1_6."""
    return _release_line(version).replace(".", "_")

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


# Firmware manifest is get_manifest_for_version(). Every dist name uses _file_token.


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

    PlatformIO places build artefacts by default under  project/.pio/<env>/.
    We simply glob for *.<ext> in that env build directory.

    ESP32-S3 builds produce a ``*_merged.bin`` (bootloader + partitions + app
    at 0x0) via rename_shc.py — that is what TeenAstroUploader flashes over USB.
    Web OTA needs the separate application image (see find_ota_app_file).
    """
    build_dir = project_path / ".pio" / pio_env
    if not build_dir.is_dir():
        return None
    candidates = list(build_dir.glob(f"*{ext}"))
    if not candidates:
        return None
    if "esp32s3" in pio_env.lower():
        merged = [p for p in candidates if p.name.endswith("_merged" + ext)]
        if merged:
            return max(merged, key=lambda p: p.stat().st_size)
    # Return the largest file (the actual firmware, not the bootloader)
    return max(candidates, key=lambda p: p.stat().st_size)


def _platformio_esptool() -> list[str] | None:
    """PlatformIO's Python plus esptool.py. The post-action in rename_shc.py
    uses the same pair; calling it from here does not depend on that hook."""
    packages = Path.home() / ".platformio" / "packages"
    esptool = packages / "tool-esptoolpy" / "esptool.py"
    python = Path.home() / ".platformio" / "penv" / "Scripts" / "python.exe"
    if not python.is_file():
        python = Path(sys.executable)
    if esptool.is_file():
        return [str(python), str(esptool)]
    return None


def _boot_app0() -> Path | None:
    path = (
        Path.home()
        / ".platformio"
        / "packages"
        / "framework-arduinoespressif32"
        / "tools"
        / "partitions"
        / "boot_app0.bin"
    )
    return path if path.is_file() else None


def refresh_s3_merged_image(project_path: Path, pio_env: str) -> bool:
    """Rewrite *_merged.bin from the application just linked.

    rename_shc.py registers the merge as a post-action on a ``pre:`` script,
    so a successful ``pio run`` can leave the October merged image in place.
    Publishing that file is how the USB .bin stays older than the _OTA image.
    """
    if "esp32s3" not in pio_env.lower():
        return True
    build_dir = project_path / ".pio" / pio_env
    if not build_dir.is_dir():
        print(f"  !! S3 build dir missing: {build_dir}")
        return False
    apps = [
        p for p in build_dir.glob("*.bin")
        if not p.name.endswith("_merged.bin")
        and p.name not in ("bootloader.bin", "partitions.bin")
    ]
    if not apps:
        print(f"  !! S3 application .bin not found in {build_dir}")
        return False
    app = max(apps, key=lambda p: p.stat().st_mtime)
    bootloader = build_dir / "bootloader.bin"
    partitions = build_dir / "partitions.bin"
    boot_app0 = _boot_app0()
    esptool = _platformio_esptool()
    merged = build_dir / f"{app.stem}_merged.bin"
    if esptool is None or boot_app0 is None or not bootloader.is_file() or not partitions.is_file():
        print("  !! S3 merge tools or bootloader/partitions missing")
        return False
    cmd = esptool + [
        "--chip", "esp32s3",
        "merge_bin",
        "-o", str(merged),
        "--flash_mode", "dio",
        "--flash_freq", "80m",
        "--flash_size", "4MB",
        "0x0", str(bootloader),
        "0x8000", str(partitions),
        "0xe000", str(boot_app0),
        "0x10000", str(app),
    ]
    print(f"  >> merging {app.name} -> {merged.name}")
    if not run(cmd, cwd=project_path):
        print(f"  !! S3 merge failed for {pio_env}")
        return False
    return True


def find_ota_app_file(project_path: Path, pio_env: str, ext: str) -> Path | None:
    """Locate the ESP32-S3 application .bin for HTTP OTA (/update).

    The merged flash image is too large for the OTA app partition (~1.25 MiB)
    and is not a valid app image (it starts with the bootloader).
    """
    if "esp32s3" not in pio_env.lower():
        return None
    build_dir = project_path / ".pio" / pio_env
    if not build_dir.is_dir():
        return None
    apps = [
        p for p in build_dir.glob(f"*{ext}")
        if not p.name.endswith("_merged" + ext)
        and p.name not in ("bootloader.bin", "partitions.bin")
    ]
    if not apps:
        return None
    return max(apps, key=lambda p: p.stat().st_size)


# ---------------------------------------------------------------------------
# Build logic
# ---------------------------------------------------------------------------
def get_manifest_for_version(version: str) -> tuple[Path, list[FirmwareBuild]]:
    """Return (dist_dir, firmware variants) for this release.

    Folder is X.Y_latest. Every published file name uses X_Y
    (TeenAstro_1_6_....hex, TeenAstroSHC_1_6_....bin).
    """
    rl = _release_line(version)
    token = _file_token(version)
    dist_dir = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader" / f"{rl}_latest"
    manifest = [
        FirmwareBuild("main", "TeenAstroMainUnit", "220", f"TeenAstro_{token}_220_TMC260", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "230", f"TeenAstro_{token}_230_TMC260", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "240_2130", f"TeenAstro_{token}_240_TMC2130", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "240_5160", f"TeenAstro_{token}_240_TMC5160", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "250_2130", f"TeenAstro_{token}_250_TMC2130", ".hex"),
        FirmwareBuild("main", "TeenAstroMainUnit", "250_5160", f"TeenAstro_{token}_250_TMC5160", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "220_2130", f"TeenAstroFocuser_{token}_220_TMC2130", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "230_2130", f"TeenAstroFocuser_{token}_230_TMC2130", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "240_2130", f"TeenAstroFocuser_{token}_240_TMC2130", ".hex"),
        FirmwareBuild("focuser", "TeenAstroFocuser", "240_5160", f"TeenAstroFocuser_{token}_240_TMC5160", ".hex"),
        FirmwareBuild("shc", "TeenAstroSHC", "ENGLISH", f"TeenAstroSHC_{token}_English", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "FRENCH", f"TeenAstroSHC_{token}_French", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "GERMAN", f"TeenAstroSHC_{token}_German", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "esp32s3", f"TeenAstroSHC_{token}_S3_English", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "esp32s3_FRENCH", f"TeenAstroSHC_{token}_S3_French", ".bin"),
        FirmwareBuild("shc", "TeenAstroSHC", "esp32s3_GERMAN", f"TeenAstroSHC_{token}_S3_German", ".bin"),
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

    # The S3 post-action often does not rebuild *_merged.bin. Make it here,
    # from the app this run just linked, before the USB image is copied.
    if not refresh_s3_merged_image(project_path, fw.pio_env):
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

    # ESP32-S3: also publish the app-only image for web OTA (/update).
    # The merged USB image does not fit in the OTA partition and will FAIL.
    ota_src = find_ota_app_file(project_path, fw.pio_env, fw.extension)
    if ota_src is not None:
        ota_dest = out_dir / f"{fw.dist_name}_OTA{fw.extension}"
        shutil.copy2(str(ota_src), str(ota_dest))
        ota_kb = ota_dest.stat().st_size / 1024
        print(f"  -> Copied to {ota_dest.relative_to(REPO_ROOT)}  ({ota_kb:.1f} KB)  [web OTA]")

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
    dist_dir, manifest = get_manifest_for_version(version)
    builds = manifest if args.target == "all" else [fw for fw in manifest if fw.target == args.target]

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
