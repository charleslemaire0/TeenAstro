#!/usr/bin/env python3
"""
publish_firmware_for_uploader.py  -  Build firmware into Uploader "latest" folder

Runs the procedure so the Uploader can offer the new build as "Latest":
  1. Build all firmware variants into TeenAstroUploader/.../X.Y_latest/
  2. Print the git commands to commit the updated 1.6_latest (or other X.Y_latest)

The Uploader uses only X.Y (e.g. 1.6) in the version dropdown. Stable = folder 1.6,
Latest = folder 1.6_latest. New builds go to X.Y_latest; promoting to stable is a
separate step (see promote_firmware_to_stable.py and FIRMWARE_PUBLISH_PROCEDURE.md).

You must follow the procedure in scripts/FIRMWARE_PUBLISH_PROCEDURE.md and only that procedure.

Usage (from repository root):
    python scripts/publish_firmware_for_uploader.py <X.Y>
    python scripts/publish_firmware_for_uploader.py 1.6
"""

import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def release_line(version: str) -> str:
    """Return X.Y from X.Y or X.Y.Z."""
    parts = version.strip().split(".")
    return ".".join(parts[:2]) if len(parts) >= 2 else version


def main() -> None:
    if len(sys.argv) != 2:
        print("Usage: python scripts/publish_firmware_for_uploader.py <X.Y>", file=sys.stderr)
        print("Example: python scripts/publish_firmware_for_uploader.py 1.6", file=sys.stderr)
        sys.exit(1)

    version = sys.argv[1].strip()
    if not version:
        print("Error: VERSION must be non-empty (e.g. 1.6)", file=sys.stderr)
        sys.exit(1)

    rl = release_line(version)

    # Build firmware into X.Y_latest (build_firmware uses release line for folder and filenames)
    print("=" * 60)
    print("Building firmware into", rl + "_latest")
    print("=" * 60)
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "build_firmware.py"), "--version", version],
        cwd=str(REPO_ROOT),
    )
    if result.returncode != 0:
        print("Firmware build failed. Fix errors and run this script again.", file=sys.stderr)
        sys.exit(result.returncode)

    dist_folder = f"TeenAstroUploader/TeenAstroUploader/{rl}_latest"
    print()
    print("=" * 60)
    print("Commit the updated latest folder (run these commands)")
    print("=" * 60)
    print()
    print('  git add "' + dist_folder + '/"')
    print(f'  git commit -m "Build firmware {rl} into latest"')
    print("  git push")
    print()
    print("To release as Stable later, run: python scripts/promote_firmware_to_stable.py", rl)
    print("See scripts/FIRMWARE_PUBLISH_PROCEDURE.md.")


if __name__ == "__main__":
    main()
