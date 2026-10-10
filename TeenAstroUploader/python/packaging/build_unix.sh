#!/usr/bin/env bash
# Build a frozen TeenAstroUploader folder for macOS or Linux (PyInstaller).
# Usage: ./packaging/build_unix.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PYTHON="${PYTHON:-python3}"
if ! "$PYTHON" -c "import tkinter" 2>/dev/null; then
  echo "Python with tkinter required (e.g. python3-tk on Debian/Ubuntu)." >&2
  exit 1
fi

"$PYTHON" -m venv .venv
# shellcheck disable=SC1091
source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements-dev.txt

rm -rf build dist
pyinstaller --noconfirm packaging/TeenAstroUploader.spec

OUT="dist/TeenAstroUploader"
echo ""
echo "Built: $OUT"
echo "Run:   $OUT/TeenAstroUploader"
echo "Optional: copy teensy_loader_cli into $OUT/ or tools/"
