#!/usr/bin/env python3
"""
promote_firmware_to_stable.py  -  Promote Uploader "latest" to "stable"

Copies the contents of TeenAstroUploader/.../X.Y_latest/ into X.Y/ so that
the Uploader offers the same firmware as "Stable". Run this when the build
in X.Y_latest is the one you want to release.

You must follow the procedure in scripts/FIRMWARE_PUBLISH_PROCEDURE.md.

Usage (from repository root):
    python scripts/promote_firmware_to_stable.py <X.Y>
    python scripts/promote_firmware_to_stable.py 1.6
"""

import shutil
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
UPLOADER_BASE = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader"


def release_line(version: str) -> str:
    """Return X.Y from X.Y or X.Y.Z."""
    parts = version.strip().split(".")
    return ".".join(parts[:2]) if len(parts) >= 2 else version


def main() -> None:
    if len(sys.argv) != 2:
        print("Usage: python scripts/promote_firmware_to_stable.py <X.Y>", file=sys.stderr)
        print("Example: python scripts/promote_firmware_to_stable.py 1.6", file=sys.stderr)
        sys.exit(1)

    version = sys.argv[1].strip()
    if not version:
        print("Error: VERSION must be non-empty (e.g. 1.6)", file=sys.stderr)
        sys.exit(1)

    rl = release_line(version)
    latest_dir = UPLOADER_BASE / f"{rl}_latest"
    stable_dir = UPLOADER_BASE / rl

    if not latest_dir.is_dir():
        print(f"Error: {latest_dir} not found.", file=sys.stderr)
        sys.exit(1)

    stable_dir.mkdir(parents=True, exist_ok=True)
    for f in latest_dir.iterdir():
        if f.is_file():
            shutil.copy2(f, stable_dir / f.name)
            print(f"  {f.name} -> {rl}/")

    print()
    print("=" * 60)
    print("Commit the updated stable folder (run these commands)")
    print("=" * 60)
    print()
    print(f'  git add "TeenAstroUploader/TeenAstroUploader/{rl}/"')
    print(f'  git commit -m "Promote firmware {rl}: latest to stable"')
    print("  git push")


if __name__ == "__main__":
    main()
