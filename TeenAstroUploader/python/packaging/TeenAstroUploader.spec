# -*- mode: python ; coding: utf-8 -*-
# PyInstaller spec for TeenAstro Firmware Uploader
# Run from TeenAstroUploader/python: pyinstaller packaging/TeenAstroUploader.spec

import sys
from pathlib import Path

block_cipher = None
root = Path(SPECPATH).resolve().parent
icon_ico = root / "packaging" / "icon.ico"

a = Analysis(
    [str(root / "run_uploader.py")],
    pathex=[str(root)],
    binaries=[],
    datas=[(str(icon_ico), ".")] if icon_ico.is_file() else [],
    hiddenimports=[
        "serial",
        "serial.tools.list_ports",
        "esptool",
        "reedsolo",
        "bitstring",
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name="TeenAstroUploader",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=str(icon_ico) if icon_ico.is_file() else None,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name="TeenAstroUploader",
)
