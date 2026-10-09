#!/usr/bin/env python3
"""Capture TeenAstroUploader screenshots for the groups.io wiki."""
from __future__ import annotations

import sys
import time
from pathlib import Path

from PIL import Image, ImageGrab

ROOT = Path(__file__).resolve().parents[2] / "TeenAstroUploader" / "python"
sys.path.insert(0, str(ROOT))

OUT = Path(__file__).resolve().parent / "screenshots"
OUT.mkdir(exist_ok=True)


def grab_widget(widget, path: Path, max_width: int = 560) -> Path:
    widget.update_idletasks()
    widget.update()
    time.sleep(0.25)
    widget.lift()
    widget.attributes("-topmost", True)
    widget.update()
    x = widget.winfo_rootx()
    y = widget.winfo_rooty()
    w = widget.winfo_width()
    h = widget.winfo_height()
    # small padding
    bbox = (x, y, x + w, y + h)
    img = ImageGrab.grab(bbox=bbox)
    if img.width > max_width:
        ratio = max_width / img.width
        img = img.resize((max_width, int(img.height * ratio)), Image.Resampling.LANCZOS)
    # palette-friendly PNG
    img = img.convert("P", palette=Image.Palette.ADAPTIVE, colors=128)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, optimize=True)
    print(f"wrote {path} ({path.stat().st_size} bytes)")
    return path


def main() -> None:
    from teenastro_uploader.gui import UploaderApp
    from teenastro_uploader import eeprom_dialog
    from teenastro_uploader.eeprom_mount import EepromMount

    app = UploaderApp()
    app.update()
    time.sleep(0.4)

    # find notebook
    nb = None
    for child in app.winfo_children():
        if child.winfo_class() == "TNotebook":
            nb = child
            break
    if nb is None:
        raise SystemExit("notebook not found")

    shots = [
        (0, "uploader_telescope.png"),
        (1, "uploader_focuser.png"),
        (2, "uploader_handcontroller.png"),
    ]
    for idx, name in shots:
        nb.select(idx)
        app.update()
        time.sleep(0.2)
        grab_widget(app, OUT / name)

    # EEPROM dialog with fake data (no serial)
    dlg = eeprom_dialog.MountEepromDialog.__new__(eeprom_dialog.MountEepromDialog)
    import tkinter as tk

    tk.Toplevel.__init__(dlg, app)
    dlg.title("Telescope EEPROM Configuration")
    dlg.resizable(False, False)
    dlg.transient(app)
    dlg._port = "COM0"
    dlg._baud = 57600
    dlg._cfg = EepromMount()
    dlg._write_mtype = tk.BooleanVar(value=False)
    dlg._status = tk.StringVar(value="")
    dlg._vars = {}
    dlg._build()
    dlg.update()
    time.sleep(0.3)
    grab_widget(dlg, OUT / "uploader_eeprom_mount.png", max_width=640)

    # Motors tab
    for child in dlg.winfo_children():
        if child.winfo_class() == "TNotebook":
            child.select(1)
            break
    dlg.update()
    time.sleep(0.2)
    grab_widget(dlg, OUT / "uploader_eeprom_motors.png", max_width=640)

    dlg.destroy()
    app.destroy()
    print("done")


if __name__ == "__main__":
    main()
