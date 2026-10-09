"""Tkinter GUI for TeenAstro Firmware Uploader (classic Windows layout)."""

from __future__ import annotations

import threading
import time
import tkinter as tk
from datetime import datetime
from pathlib import Path
from tkinter import messagebox, ttk

from . import config_focuser, config_mount, detect, download, eeprom_dialog, flash
from .paths import (
    APP_NAME,
    FIRMWARE_VERSIONS,
    FOCUSER_PCBS,
    LANGUAGES,
    MAINUNIT_PCBS,
    SHC_PCBS,
    app_icon_path,
    config_base_path,
)
from .serial_lx200 import SerialSession, probe_device
from .version import __version__
from .version_util import firmware_supported_for_config, parse_firmware_version


class UploaderApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(f"{APP_NAME} {__version__}")
        self.minsize(400, 330)
        self.geometry("420x340")
        self.resizable(False, False)
        self.attributes("-topmost", True)
        self._set_icon()

        self._channel = tk.StringVar(value="stable")
        self._version = tk.StringVar(value=FIRMWARE_VERSIONS[0])
        self._pcb_t = tk.StringVar(value=MAINUNIT_PCBS[0])
        self._pcb_f = tk.StringVar(value=FOCUSER_PCBS[0])
        self._pcb_shc = tk.StringVar(value=SHC_PCBS[0])
        self._language = tk.StringVar(value=LANGUAGES[0])
        self._com_shc = tk.StringVar(value="")
        self._ip = tk.StringVar(value="")
        self._status = tk.StringVar(value="")
        self._downloading = False

        self._build()
        self.protocol("WM_DELETE_WINDOW", self.destroy)

    def _set_icon(self) -> None:
        icon = app_icon_path()
        if icon is None:
            return
        try:
            self.iconbitmap(default=str(icon))
        except tk.TclError:
            try:
                self.iconbitmap(str(icon))
            except tk.TclError:
                pass

    def _use_latest(self) -> bool:
        return self._channel.get() == "latest"

    def _refuse_old_firmware(self, fw: str, action: str) -> bool:
        """Return True if action must stop (firmware < 1.5 or unknown)."""
        if firmware_supported_for_config(fw):
            return False
        ver = parse_firmware_version(fw)
        shown = f"{ver[0]}.{ver[1]}.{ver[2]}" if ver else (fw or "unknown")
        messagebox.showerror(
            APP_NAME,
            f"Cannot {action}: device firmware is {shown}.\n\n"
            "Auto upgrade needs TeenAstro 1.5 or newer.\n"
            "Older release branches are no longer supported.\n\n"
            "Use Upload! to flash firmware only "
            "(save settings manually first).",
            parent=self,
        )
        return True

    def _build(self) -> None:
        pad = {"padx": 8, "pady": 4}

        # Firmware version row (matches original VB form)
        top = ttk.Frame(self, padding=(10, 10, 10, 4))
        top.pack(fill=tk.X)

        ttk.Label(top, text="Firmware Version").grid(row=0, column=0, sticky="w", **pad)
        ttk.Combobox(
            top,
            textvariable=self._version,
            values=list(FIRMWARE_VERSIONS),
            state="readonly",
            width=8,
        ).grid(row=0, column=1, sticky="w", **pad)

        ttk.Radiobutton(top, text="Stable", variable=self._channel, value="stable").grid(
            row=0, column=2, sticky="w", padx=(12, 4)
        )
        ttk.Radiobutton(top, text="Latest", variable=self._channel, value="latest").grid(
            row=0, column=3, sticky="w", padx=(4, 4)
        )

        ttk.Button(top, text="Download!", command=self._on_download, width=12).grid(
            row=1, column=0, columnspan=2, sticky="w", **pad
        )
        ttk.Button(top, text="Open Folder", command=self._on_open_folder, width=12).grid(
            row=1, column=2, columnspan=2, sticky="w", **pad
        )

        # Device tabs
        nb = ttk.Notebook(self)
        nb.pack(fill=tk.BOTH, expand=True, padx=10, pady=(4, 4))

        t_tel = ttk.Frame(nb, padding=10)
        t_foc = ttk.Frame(nb, padding=10)
        t_shc = ttk.Frame(nb, padding=10)
        nb.add(t_tel, text="Telescope")
        nb.add(t_foc, text="Focuser")
        nb.add(t_shc, text="Hand controler")

        self._build_device_tab(
            t_tel,
            pcb_var=self._pcb_t,
            pcb_values=MAINUNIT_PCBS,
            on_upload=self._on_upload_t,
            on_auto=self._on_auto_t,
            on_eeprom=self._on_eeprom_t,
        )
        self._build_device_tab(
            t_foc,
            pcb_var=self._pcb_f,
            pcb_values=FOCUSER_PCBS,
            on_upload=self._on_upload_f,
            on_auto=self._on_auto_f,
            on_eeprom=self._on_eeprom_f,
        )
        self._build_shc_tab(t_shc)

        # Status + progress (bottom)
        foot = ttk.Frame(self, padding=(10, 2, 10, 8))
        foot.pack(fill=tk.X)
        ttk.Label(foot, textvariable=self._status).pack(anchor="w")
        self._progress = ttk.Progressbar(foot, mode="determinate", length=380)
        self._progress.pack(fill=tk.X, pady=(2, 0))

    def _build_device_tab(
        self,
        parent: ttk.Frame,
        *,
        pcb_var: tk.StringVar,
        pcb_values: tuple,
        on_upload,
        on_auto,
        on_eeprom,
    ) -> None:
        ttk.Label(parent, text="PCB Board").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=4)
        ttk.Combobox(
            parent, textvariable=pcb_var, values=list(pcb_values), state="readonly", width=14
        ).grid(row=0, column=1, sticky="w", pady=4)

        ttk.Button(parent, text="Upload!", command=on_upload, width=14).grid(
            row=0, column=2, sticky="w", padx=(16, 0), pady=4
        )
        ttk.Button(parent, text="Auto!", command=on_auto, width=14).grid(
            row=1, column=2, sticky="w", padx=(16, 0), pady=4
        )
        ttk.Button(parent, text="EEPROM", command=on_eeprom, width=14).grid(
            row=2, column=2, sticky="w", padx=(16, 0), pady=4
        )

        ttk.Label(
            parent,
            text="Auto! = backup → flash → restore (1.5+)\n"
            "EEPROM = edit parameters (2 mounts on MainUnit)",
            foreground="#555555",
        ).grid(row=3, column=0, columnspan=3, sticky="w", pady=(12, 0))

    def _on_eeprom_t(self) -> None:
        eeprom_dialog.open_mount_eeprom(self)

    def _on_eeprom_f(self) -> None:
        eeprom_dialog.open_focuser_eeprom(self)

    def _build_shc_tab(self, parent: ttk.Frame) -> None:
        rows = (
            ("PCB Board", self._pcb_shc, list(SHC_PCBS), "combo"),
            ("Language", self._language, list(LANGUAGES), "combo"),
            ("ComPort", self._com_shc, None, "com"),
            ("IP Adress", self._ip, None, "entry"),
        )
        for i, (label, var, values, kind) in enumerate(rows):
            ttk.Label(parent, text=label).grid(row=i, column=0, sticky="w", padx=(0, 8), pady=3)
            if kind == "combo":
                ttk.Combobox(
                    parent, textvariable=var, values=values, state="readonly", width=14
                ).grid(row=i, column=1, sticky="w", pady=3)
            elif kind == "com":
                self._com_combo = ttk.Combobox(parent, textvariable=var, width=14)
                self._com_combo.grid(row=i, column=1, sticky="w", pady=3)
                self._com_combo.bind("<Button-1>", self._refresh_com_ports)
            else:
                ttk.Entry(parent, textvariable=var, width=16).grid(
                    row=i, column=1, sticky="w", pady=3
                )

        ttk.Button(parent, text="Upload over COM!", command=self._on_upload_shc, width=16).grid(
            row=2, column=2, sticky="w", padx=(16, 0), pady=3
        )
        ttk.Button(parent, text="Upload over WIFI!", command=self._on_wifi_shc, width=16).grid(
            row=3, column=2, sticky="w", padx=(16, 0), pady=3
        )

    def _refresh_com_ports(self, _event=None) -> None:
        ports = detect.list_serial_ports()
        self._com_combo["values"] = ports
        if ports and self._com_shc.get() not in ports:
            self._com_shc.set(ports[0])

    def _on_open_folder(self) -> None:
        try:
            flash.open_firmware_folder()
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)

    def _on_download(self) -> None:
        if self._downloading:
            return
        ver = self._version.get()
        if not ver:
            messagebox.showinfo(APP_NAME, "Select a firmware version.", parent=self)
            return
        self._downloading = True
        self._progress["value"] = 0
        self._status.set("Starting download…")

        def work() -> None:
            def progress(current: int, total: int, name: str) -> None:
                self.after(0, lambda: self._update_progress(current, total, name))

            try:
                result = download.download_all(ver, progress=progress)
                self.after(0, lambda: self._download_done(result))
            except Exception as exc:  # noqa: BLE001
                self.after(0, lambda: self._download_failed(str(exc)))

        threading.Thread(target=work, daemon=True).start()

    def _update_progress(self, current: int, total: int, name: str) -> None:
        pct = int(100 * current / total) if total else 0
        self._progress["value"] = max(0, min(100, pct))
        self._status.set(f"Downloading {current} of {total}: {name}")

    def _download_done(self, result: download.DownloadResult) -> None:
        self._downloading = False
        self._progress["value"] = 100
        self._status.set(f"{result.success_count} of {result.total_count} downloaded")
        if result.error_message:
            messagebox.showerror(
                "TeenAstro Firmware Download", result.error_message, parent=self
            )
        messagebox.showinfo(
            APP_NAME,
            f"{result.success_count} of {result.total_count} successfully downloaded!",
            parent=self,
        )

    def _download_failed(self, message: str) -> None:
        self._downloading = False
        self._progress["value"] = 0
        self._status.set("Download failed.")
        messagebox.showerror("TeenAstro Firmware Download", message, parent=self)

    def _on_upload_t(self) -> None:
        try:
            flash.upload_mainunit(self._version.get(), self._use_latest(), self._pcb_t.get())
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)

    def _on_upload_f(self) -> None:
        try:
            flash.upload_focuser(self._version.get(), self._use_latest(), self._pcb_f.get())
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)

    def _on_upload_shc(self) -> None:
        try:
            flash.upload_shc_com(
                self._version.get(),
                self._use_latest(),
                self._language.get(),
                self._com_shc.get(),
            )
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)

    def _on_wifi_shc(self) -> None:
        try:
            flash.open_shc_wifi(self._ip.get())
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)

    def _set_step(self, pct: int, text: str) -> None:
        self._progress["value"] = max(0, min(100, pct))
        self._status.set(text)
        self.update_idletasks()

    def _wait_for_device(self, port_name: str, kind: str, seconds: int = 90):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            remaining = int(deadline - time.monotonic())
            self._set_step(
                70 + int(25 * (1 - remaining / max(seconds, 1))),
                f"Waiting for reboot on {port_name}… ({remaining}s)",
            )
            info, baud = probe_device(port_name)
            if info is not None and info.kind == kind:
                return info, baud
            time.sleep(2)
            self.update()
        return None

    def _on_auto_t(self) -> None:
        self.config(cursor="watch")
        self.update_idletasks()
        try:
            units = detect.find_main_units()
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)
            return
        finally:
            self.config(cursor="")

        if not units:
            messagebox.showinfo(APP_NAME, "No TeenAstro MainUnit found on a COM port.", parent=self)
            return
        if len(units) > 1:
            lines = "\n".join(
                f"{u.port_name}  PCB {u.board}  driver {u.driver}  ({u.pcb})" for u in units
            )
            messagebox.showinfo(
                APP_NAME,
                "Several MainUnits are connected. Unplug the others and try again.\n\n" + lines,
                parent=self,
            )
            return
        one = units[0]
        if one.pcb is None:
            messagebox.showinfo(
                APP_NAME,
                f"MainUnit on {one.port_name} is PCB {one.board}, driver {one.driver}.\n"
                "This uploader has no firmware for that board.",
                parent=self,
            )
            return
        if self._refuse_old_firmware(one.firmware, "run Auto!"):
            return
        self._pcb_t.set(one.pcb)
        fwv = self._version.get() + (" latest" if self._use_latest() else "")
        ask = (
            f"Auto! — MainUnit on {one.port_name}\n"
            f"PCB {one.board}, driver {one.driver} ({one.pcb})\n"
            f"Firmware now: {one.firmware}\n\n"
            f"1) Backup parameters\n"
            f"2) Upload {fwv}\n"
            f"3) Restore parameters after reboot\n\n"
            f"Continue?"
        )
        if not messagebox.askyesno("Auto!", ask, parent=self):
            return
        self._auto_full_loop_mount(one)

    def _on_auto_f(self) -> None:
        self.config(cursor="watch")
        self.update_idletasks()
        try:
            units = detect.find_focusers()
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)
            return
        finally:
            self.config(cursor="")

        if not units:
            messagebox.showinfo(APP_NAME, "No TeenAstro Focuser found on a COM port.", parent=self)
            return
        if len(units) > 1:
            lines = "\n".join(f"{u.port_name}  PCB {u.board_text}" for u in units)
            messagebox.showinfo(
                APP_NAME,
                "Several Focusers are connected. Unplug the others and try again.\n\n" + lines,
                parent=self,
            )
            return
        one = units[0]
        if one.pcb is None and one.board_text.startswith("2.4"):
            pick = messagebox.askyesnocancel(
                "Auto!",
                f"Focuser on {one.port_name} is PCB {one.board_text}.\n"
                "It did not report the stepper driver.\n\n"
                "Yes = TMC5160\nNo = TMC2130",
                parent=self,
            )
            if pick is None:
                return
            one.driver = 3 if pick else 2
            one.pcb = detect.pcb_from_focuser(one.board_text, one.driver)
        if one.pcb is None:
            messagebox.showinfo(
                APP_NAME,
                f"Focuser on {one.port_name} is PCB {one.board_text}.\n"
                "This uploader has no firmware for that board.",
                parent=self,
            )
            return
        if self._refuse_old_firmware(one.firmware, "run Auto!"):
            return
        self._pcb_f.set(one.pcb)
        fwv = self._version.get() + (" latest" if self._use_latest() else "")
        driver_note = "" if one.driver == 0 else f", driver {one.driver}"
        ask = (
            f"Auto! — Focuser on {one.port_name}\n"
            f"PCB {one.board_text}{driver_note} ({one.pcb})\n"
            f"Firmware now: {one.firmware}\n\n"
            f"1) Backup parameters\n"
            f"2) Upload {fwv}\n"
            f"3) Restore parameters after reboot\n\n"
            f"Continue?"
        )
        if not messagebox.askyesno("Auto!", ask, parent=self):
            return
        self._auto_full_loop_focuser(one)

    def _auto_full_loop_mount(self, one: detect.DetectedMainUnit) -> None:
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup = config_base_path() / f"auto_mount_{stamp}.json"
        self.config(cursor="watch")
        try:
            self._set_step(10, "Auto: backing up mount parameters…")
            cfg = None
            last_err: Exception | None = None
            for baud in (57600, 115200):
                try:
                    with SerialSession(one.port_name, baud=baud, timeout=1.0) as sess:
                        cfg = config_mount.read_mount(sess)
                    break
                except Exception as exc:  # noqa: BLE001
                    last_err = exc
            if cfg is None:
                raise last_err or RuntimeError("Mount backup failed")
            config_mount.save_mount(backup, cfg)
            self._set_step(35, f"Auto: saved {backup.name}")

            self._set_step(45, "Auto: uploading firmware…")
            flash.upload_mainunit(self._version.get(), self._use_latest(), one.pcb)
            self._set_step(65, "Auto: flash started — waiting for board…")

            messagebox.showinfo(
                "Auto!",
                "Firmware upload was started.\n\n"
                "When the Teensy Loader has finished and the mount has rebooted, "
                "click OK to restore parameters.",
                parent=self,
            )

            found = self._wait_for_device(one.port_name, "mainunit")
            if found is None:
                messagebox.showwarning(
                    APP_NAME,
                    "Could not reconnect after flash.\n"
                    f"Parameters were saved to:\n{backup}\n\n"
                    "Run Auto! again after the board is back, or restore "
                    "the JSON with TeenAstroConfig.",
                    parent=self,
                )
                self._set_step(100, "Auto: backup OK, restore skipped")
                return
            _info, baud = found
            if self._refuse_old_firmware(_info.firmware, "restore parameters"):
                self._set_step(100, "Auto: backup OK, restore skipped (FW < 1.5)")
                return
            self._set_step(90, "Auto: restoring mount parameters…")
            with SerialSession(one.port_name, baud=baud, timeout=1.0) as sess:
                errors = config_mount.write_mount(sess, cfg, write_mount_type=False)
                time.sleep(0.3)
                self._set_step(95, "Auto: verifying restored parameters…")
                mismatches = config_mount.verify_mount(sess, cfg)
            if errors or mismatches:
                detail = []
                if errors:
                    detail.append("Write issues:\n" + "\n".join(errors[:10]))
                if mismatches:
                    detail.append("Verify mismatches:\n" + "\n".join(mismatches[:12]))
                messagebox.showwarning(
                    APP_NAME,
                    "\n\n".join(detail) + f"\n\nBackup file:\n{backup}",
                    parent=self,
                )
            else:
                messagebox.showinfo(
                    APP_NAME,
                    f"Upgrade complete.\n\nBackup:\n{backup}\n\n"
                    "Firmware uploaded, parameters restored and verified.",
                    parent=self,
                )
            self._set_step(100, "Auto: done")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(
                APP_NAME,
                f"Auto! failed:\n{exc}\n\n"
                + (f"Backup (if any):\n{backup}" if backup.exists() else ""),
                parent=self,
            )
            self._set_step(0, "Auto: failed")
        finally:
            self.config(cursor="")

    def _auto_full_loop_focuser(self, one: detect.DetectedFocuser) -> None:
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup = config_base_path() / f"auto_focuser_{stamp}.json"
        self.config(cursor="watch")
        try:
            self._set_step(10, "Auto: backing up focuser parameters…")
            with SerialSession(one.port_name, baud=9600, timeout=1.0) as sess:
                cfg = config_focuser.read_focuser(sess)
            config_focuser.save_focuser(backup, cfg)
            self._set_step(35, f"Auto: saved {backup.name}")

            self._set_step(45, "Auto: uploading firmware…")
            flash.upload_focuser(self._version.get(), self._use_latest(), one.pcb)
            self._set_step(65, "Auto: flash started — waiting for board…")

            messagebox.showinfo(
                "Auto!",
                "Firmware upload was started.\n\n"
                "When the Teensy Loader has finished and the focuser has rebooted, "
                "click OK to restore parameters.",
                parent=self,
            )

            found = self._wait_for_device(one.port_name, "focuser")
            if found is None:
                messagebox.showwarning(
                    APP_NAME,
                    "Could not reconnect after flash.\n"
                    f"Parameters were saved to:\n{backup}\n\n"
                    "Run Auto! again after the board is back, or restore "
                    "the JSON with TeenAstroConfig.",
                    parent=self,
                )
                self._set_step(100, "Auto: backup OK, restore skipped")
                return
            _info, baud = found
            if self._refuse_old_firmware(_info.firmware, "restore parameters"):
                self._set_step(100, "Auto: backup OK, restore skipped (FW < 1.5)")
                return
            self._set_step(90, "Auto: restoring focuser parameters…")
            with SerialSession(one.port_name, baud=baud, timeout=1.0) as sess:
                errors = config_focuser.write_focuser(sess, cfg)
                time.sleep(0.2)
                self._set_step(95, "Auto: verifying restored parameters…")
                mismatches = config_focuser.verify_focuser(sess, cfg)
            if errors or mismatches:
                detail = []
                if errors:
                    detail.append("Write issues:\n" + "\n".join(errors[:10]))
                if mismatches:
                    detail.append("Verify mismatches:\n" + "\n".join(mismatches[:12]))
                messagebox.showwarning(
                    APP_NAME,
                    "\n\n".join(detail) + f"\n\nBackup file:\n{backup}",
                    parent=self,
                )
            else:
                messagebox.showinfo(
                    APP_NAME,
                    f"Upgrade complete.\n\nBackup:\n{backup}\n\n"
                    "Firmware uploaded, parameters restored and verified.",
                    parent=self,
                )
            self._set_step(100, "Auto: done")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(
                APP_NAME,
                f"Auto! failed:\n{exc}\n\n"
                + (f"Backup (if any):\n{backup}" if backup.exists() else ""),
                parent=self,
            )
            self._set_step(0, "Auto: failed")
        finally:
            self.config(cursor="")


def run() -> None:
    app = UploaderApp()
    app.mainloop()
