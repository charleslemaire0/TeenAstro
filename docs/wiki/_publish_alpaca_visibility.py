#!/usr/bin/env python3
"""Make ASCOM Alpaca visible on Home + _Sidebar without rewriting the Alpaca pages."""
from __future__ import annotations

import os
import sys
from pathlib import Path

# Reuse helpers from the main publish script.
sys.path.insert(0, str(Path(__file__).resolve().parent))
os.environ.setdefault("IOGROUP", os.environ.get("IOGROUP", ""))

import _publish_alpaca_and_s3 as pub  # noqa: E402

PAGE_ALPACA = 43151


def main() -> None:
    if not pub.TOKEN or len(pub.TOKEN) != 64:
        raise SystemExit("IOGROUP missing or wrong length")
    pub.patch_home(PAGE_ALPACA)
    pub.patch_sidebar(PAGE_ALPACA)
    print(f"https://groups.io/g/TeenAstro/wiki/{PAGE_ALPACA}")
    print("https://groups.io/g/TeenAstro/wiki")


if __name__ == "__main__":
    main()
