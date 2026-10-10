"""Classic Windows EEPROM editor dialog (Webserver grouping)."""

from __future__ import annotations

import json
import time
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from . import config_focuser, detect, eeprom_mount
from .paths import APP_NAME, config_base_path
from .serial_lx200 import SerialSession, probe_device
from .version_util import firmware_supported_for_config


def _spin(parent, from_, to, width=10, increment=1.0) -> ttk.Spinbox:
    return ttk.Spinbox(parent, from_=from_, to=to, width=width, increment=increment)


def _add_row(parent, row: int, label: str, widget) -> None:
    ttk.Label(parent, text=label).grid(row=row, column=0, sticky="w", padx=(0, 8), pady=2)
    widget.grid(row=row, column=1, sticky="w", pady=2)


class MountEepromDialog(tk.Toplevel):
    """Edit MainUnit EEPROM — tabs match Webserver: Mount, Motors, Speed, Limits, Encoders, Site, Tracking."""

    def __init__(self, master: tk.Tk, port_name: str, baud: int) -> None:
        super().__init__(master)
        self.title("Telescope EEPROM Configuration")
        self.resizable(False, False)
        self.transient(master)
        self.grab_set()
        self._port = port_name
        self._baud = baud
        self._cfg = eeprom_mount.EepromMount()
        self._write_mtype = tk.BooleanVar(value=False)
        self._status = tk.StringVar(value="")
        self._vars: dict[str, tk.Variable] = {}
        self._build()
        self.after(50, self._read_from_device)

    def _build(self) -> None:
        nb = ttk.Notebook(self)
        nb.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        self._tab_mount = ttk.Frame(nb, padding=8)
        self._tab_motors = ttk.Frame(nb, padding=8)
        self._tab_speed = ttk.Frame(nb, padding=8)
        self._tab_limits = ttk.Frame(nb, padding=8)
        self._tab_enc = ttk.Frame(nb, padding=8)
        self._tab_site = ttk.Frame(nb, padding=8)
        self._tab_track = ttk.Frame(nb, padding=8)

        nb.add(self._tab_mount, text="Mount")
        nb.add(self._tab_motors, text="Motors")
        nb.add(self._tab_speed, text="Speed")
        nb.add(self._tab_limits, text="Limits")
        nb.add(self._tab_enc, text="Encoders")
        nb.add(self._tab_site, text="Site")
        nb.add(self._tab_track, text="Tracking")

        self._build_mount()
        self._build_motors()
        self._build_speed()
        self._build_limits()
        self._build_encoders()
        self._build_site()
        self._build_tracking()

        btns = ttk.Frame(self, padding=(8, 0, 8, 8))
        btns.pack(fill=tk.X)
        ttk.Button(btns, text="Read", width=10, command=self._read_from_device).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Write to EEPROM", width=16, command=self._write_to_device).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Save…", width=8, command=self._save_file).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Load…", width=8, command=self._load_file).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Close", width=8, command=self.destroy).pack(side=tk.RIGHT)

        ttk.Label(self, textvariable=self._status).pack(anchor="w", padx=8, pady=(0, 6))

    def _var(self, key: str, value) -> tk.Variable:
        if isinstance(value, bool):
            v: tk.Variable = tk.BooleanVar(value=value)
        elif isinstance(value, float):
            v = tk.DoubleVar(value=value)
        elif isinstance(value, int):
            v = tk.IntVar(value=value)
        else:
            v = tk.StringVar(value=str(value))
        self._vars[key] = v
        return v

    def _build_mount(self) -> None:
        f = self._tab_mount
        self._vars["mount_index"] = tk.StringVar(value="0")
        self._var("mount_name", "")
        self._var("mount_name_0", "")
        self._var("mount_name_1", "")
        self._var("mtype", "Eq-German")
        self._var("slew_settle", 0)

        ttk.Label(f, text="Selected Mount (TeenAstro stores 2)").grid(
            row=0, column=0, columnspan=2, sticky="w", pady=(0, 4)
        )
        row = ttk.Frame(f)
        row.grid(row=1, column=0, columnspan=2, sticky="w", pady=2)
        ttk.Combobox(
            row,
            textvariable=self._vars["mount_index"],
            values=("0", "1"),
            state="readonly",
            width=6,
        ).pack(side=tk.LEFT)
        ttk.Button(row, text="Activate (reboot)", command=self._activate_mount).pack(
            side=tk.LEFT, padx=(8, 0)
        )

        _add_row(
            f, 2, "Active mount name", ttk.Entry(f, textvariable=self._vars["mount_name"], width=18)
        )
        _add_row(
            f, 3, "Mount 0 name", ttk.Entry(f, textvariable=self._vars["mount_name_0"], width=18)
        )
        _add_row(
            f, 4, "Mount 1 name", ttk.Entry(f, textvariable=self._vars["mount_name_1"], width=18)
        )
        _add_row(
            f,
            5,
            "Mount type",
            ttk.Combobox(
                f,
                textvariable=self._vars["mtype"],
                values=list(eeprom_mount.MTYPE_LABELS),
                state="readonly",
                width=16,
            ),
        )
        settle = _spin(f, 0, 20, width=8)
        settle.configure(textvariable=self._vars["slew_settle"])
        _add_row(f, 6, "Slew settle (s)", settle)

        ttk.Checkbutton(
            f,
            text="Also write mount type (reboots board)",
            variable=self._write_mtype,
        ).grid(row=7, column=0, columnspan=2, sticky="w", pady=(12, 0))

        ttk.Label(
            f,
            text="Switching mount index reboots the MainUnit.\n"
            "Motor / speed / limit values are per selected mount.",
            foreground="#555555",
        ).grid(row=8, column=0, columnspan=2, sticky="w", pady=(12, 0))

    def _build_motors(self) -> None:
        f = self._tab_motors
        for col, (title, prefix) in enumerate((("Axis 1", "a1"), ("Axis 2", "a2"))):
            box = ttk.LabelFrame(f, text=title, padding=6)
            box.grid(row=0, column=col, sticky="nw", padx=(0, 10) if col == 0 else 0)
            self._var(f"{prefix}_gear", 1.0)
            self._var(f"{prefix}_steps", 200)
            self._vars[f"{prefix}_micro"] = tk.StringVar(value="16")
            self._var(f"{prefix}_reverse", False)
            self._var(f"{prefix}_low", 1000)
            self._var(f"{prefix}_high", 1000)
            self._var(f"{prefix}_bl", 0)
            self._var(f"{prefix}_blr", 16)
            self._var(f"{prefix}_silent", False)

            gear = _spin(box, 1, 60000, width=10, increment=0.001)
            gear.configure(textvariable=self._vars[f"{prefix}_gear"])
            steps = _spin(box, 1, 400, width=10)
            steps.configure(textvariable=self._vars[f"{prefix}_steps"])
            micro = ttk.Combobox(
                box,
                textvariable=self._vars[f"{prefix}_micro"],
                values=[str(x) for x in eeprom_mount.MICROSTEPS],
                state="readonly",
                width=8,
            )
            low = _spin(box, 200, 2800, width=10, increment=200)
            low.configure(textvariable=self._vars[f"{prefix}_low"])
            high = _spin(box, 200, 2800, width=10, increment=200)
            high.configure(textvariable=self._vars[f"{prefix}_high"])
            bl = _spin(box, 0, 999, width=10)
            bl.configure(textvariable=self._vars[f"{prefix}_bl"])
            blr = _spin(box, 16, 64, width=10)
            blr.configure(textvariable=self._vars[f"{prefix}_blr"])

            rows = (
                ("Gear", gear),
                ("Steps / rev", steps),
                ("Microsteps", micro),
                ("Low current (mA)", low),
                ("High current (mA)", high),
                ("Backlash (\")", bl),
                ("Backlash rate", blr),
            )
            for i, (lab, w) in enumerate(rows):
                _add_row(box, i, lab, w)
            ttk.Checkbutton(box, text="Reverse", variable=self._vars[f"{prefix}_reverse"]).grid(
                row=len(rows), column=0, columnspan=2, sticky="w", pady=2
            )
            ttk.Checkbutton(box, text="Silent", variable=self._vars[f"{prefix}_silent"]).grid(
                row=len(rows) + 1, column=0, columnspan=2, sticky="w", pady=2
            )

    def _build_speed(self) -> None:
        f = self._tab_speed
        for key, default in (
            ("guide_rate", 0.5),
            ("rate1", 4),
            ("rate2", 16),
            ("rate3", 64),
            ("max_rate", 800),
            ("default_rate", 0),
            ("deg_acc", 10),
        ):
            self._var(key, default)

        guide = _spin(f, 0.01, 2.0, width=10, increment=0.01)
        guide.configure(textvariable=self._vars["guide_rate"])
        r1 = _spin(f, 1, 999, width=10)
        r1.configure(textvariable=self._vars["rate1"])
        r2 = _spin(f, 1, 999, width=10)
        r2.configure(textvariable=self._vars["rate2"])
        r3 = _spin(f, 1, 999, width=10)
        r3.configure(textvariable=self._vars["rate3"])
        mx = _spin(f, 1, 9999, width=10)
        mx.configure(textvariable=self._vars["max_rate"])
        dr = _spin(f, 0, 4, width=10)
        dr.configure(textvariable=self._vars["default_rate"])
        acc = _spin(f, 1, 999, width=10)
        acc.configure(textvariable=self._vars["deg_acc"])

        for i, (lab, w) in enumerate(
            (
                ("Guide rate (× sidereal)", guide),
                ("Rate 1", r1),
                ("Rate 2", r2),
                ("Rate 3", r3),
                ("Max rate", mx),
                ("Default rate (0–4)", dr),
                ("Acceleration (GXRA)", acc),
            )
        ):
            _add_row(f, i, lab, w)

    def _build_limits(self) -> None:
        f = self._tab_limits
        for key, default in (
            ("horizon", -10),
            ("overhead", 90),
            ("under_pole", 12.0),
            ("meridian_e", 15),
            ("meridian_w", 15),
            ("axis1_min", -360),
            ("axis1_max", 360),
            ("axis2_min", -360),
            ("axis2_max", 360),
            ("dist_from_pole", 181),
        ):
            self._var(key, default)

        ttk.Label(f, text="Limits Altitude", font=("Segoe UI", 9, "bold")).grid(
            row=0, column=0, columnspan=2, sticky="w"
        )
        h = _spin(f, -30, 30, width=10)
        h.configure(textvariable=self._vars["horizon"])
        o = _spin(f, 60, 91, width=10)
        o.configure(textvariable=self._vars["overhead"])
        _add_row(f, 1, "Horizon (°)", h)
        _add_row(f, 2, "Overhead (°)", o)

        ttk.Label(f, text="Limits German Equatorial", font=("Segoe UI", 9, "bold")).grid(
            row=3, column=0, columnspan=2, sticky="w", pady=(8, 0)
        )
        up = _spin(f, 9, 12, width=10, increment=0.1)
        up.configure(textvariable=self._vars["under_pole"])
        me = _spin(f, -45, 45, width=10)
        me.configure(textvariable=self._vars["meridian_e"])
        mw = _spin(f, -45, 45, width=10)
        mw.configure(textvariable=self._vars["meridian_w"])
        _add_row(f, 4, "Under pole (h)", up)
        _add_row(f, 5, "Past meridian E (°)", me)
        _add_row(f, 6, "Past meridian W (°)", mw)

        ttk.Label(f, text="Limits of Instrument Axes", font=("Segoe UI", 9, "bold")).grid(
            row=7, column=0, columnspan=2, sticky="w", pady=(8, 0)
        )
        for i, (lab, key) in enumerate(
            (
                ("Axis1 min (°)", "axis1_min"),
                ("Axis1 max (°)", "axis1_max"),
                ("Axis2 min (°)", "axis2_min"),
                ("Axis2 max (°)", "axis2_max"),
            ),
            start=8,
        ):
            sp = _spin(f, -360, 360, width=10)
            sp.configure(textvariable=self._vars[key])
            _add_row(f, i, lab, sp)

        ttk.Label(f, text="Tracking Safety", font=("Segoe UI", 9, "bold")).grid(
            row=12, column=0, columnspan=2, sticky="w", pady=(8, 0)
        )
        dfp = _spin(f, 0, 181, width=10)
        dfp.configure(textvariable=self._vars["dist_from_pole"])
        _add_row(f, 13, "Dist. from pole (181=off)", dfp)

    def _build_encoders(self) -> None:
        f = self._tab_enc
        for key, default in (
            ("enc1_pulse", 0),
            ("enc1_reverse", False),
            ("enc2_pulse", 0),
            ("enc2_reverse", False),
            ("enc_sync", 0),
        ):
            self._var(key, default)
        p1 = _spin(f, 0, 999999, width=12)
        p1.configure(textvariable=self._vars["enc1_pulse"])
        p2 = _spin(f, 0, 999999, width=12)
        p2.configure(textvariable=self._vars["enc2_pulse"])
        sync = _spin(f, 0, 10, width=12)
        sync.configure(textvariable=self._vars["enc_sync"])
        _add_row(f, 0, "Axis1 pulse/°", p1)
        ttk.Checkbutton(f, text="Axis1 reverse", variable=self._vars["enc1_reverse"]).grid(
            row=1, column=0, columnspan=2, sticky="w"
        )
        _add_row(f, 2, "Axis2 pulse/°", p2)
        ttk.Checkbutton(f, text="Axis2 reverse", variable=self._vars["enc2_reverse"]).grid(
            row=3, column=0, columnspan=2, sticky="w"
        )
        _add_row(f, 4, "Sync mode", sync)

    def _build_site(self) -> None:
        f = self._tab_site
        self._var("latitude", "+00*00")
        self._var("longitude", "+000*00")
        self._var("elevation", 0)
        self._var("timezone", 0.0)
        _add_row(f, 0, "Latitude", ttk.Entry(f, textvariable=self._vars["latitude"], width=14))
        _add_row(f, 1, "Longitude", ttk.Entry(f, textvariable=self._vars["longitude"], width=14))
        elev = _spin(f, -500, 9000, width=10)
        elev.configure(textvariable=self._vars["elevation"])
        _add_row(f, 2, "Elevation (m)", elev)
        tz = _spin(f, -12, 14, width=10, increment=0.5)
        tz.configure(textvariable=self._vars["timezone"])
        _add_row(f, 3, "Time zone (h)", tz)

    def _build_tracking(self) -> None:
        f = self._tab_track
        self._var("refr_goto", False)
        self._var("refr_pole", False)
        self._var("refr_tracking", False)
        ttk.Label(f, text="Refraction Options").grid(row=0, column=0, sticky="w", pady=(0, 6))
        ttk.Checkbutton(f, text="Refraction on Goto", variable=self._vars["refr_goto"]).grid(
            row=1, column=0, sticky="w"
        )
        ttk.Checkbutton(f, text="Refraction near Pole", variable=self._vars["refr_pole"]).grid(
            row=2, column=0, sticky="w"
        )
        ttk.Checkbutton(
            f, text="Refraction on Tracking", variable=self._vars["refr_tracking"]
        ).grid(row=3, column=0, sticky="w")

    def _session(self) -> SerialSession:
        return SerialSession(self._port, baud=self._baud, timeout=1.0)

    def _populate(self, cfg: eeprom_mount.EepromMount) -> None:
        self._cfg = cfg
        self._vars["mount_index"].set(str(cfg.mount_index))
        self._vars["mount_name"].set(cfg.mount_name)
        self._vars["mount_name_0"].set(cfg.mount_name_0)
        self._vars["mount_name_1"].set(cfg.mount_name_1)
        self._vars["mtype"].set(cfg.mtype)
        self._vars["slew_settle"].set(cfg.slew_settle)

        for prefix, ax in (("a1", cfg.axis1), ("a2", cfg.axis2)):
            self._vars[f"{prefix}_gear"].set(ax.gear)
            self._vars[f"{prefix}_steps"].set(ax.steps)
            self._vars[f"{prefix}_micro"].set(str(ax.micro))
            self._vars[f"{prefix}_reverse"].set(ax.reverse)
            self._vars[f"{prefix}_low"].set(ax.low_curr)
            self._vars[f"{prefix}_high"].set(ax.high_curr)
            self._vars[f"{prefix}_bl"].set(ax.backlash)
            self._vars[f"{prefix}_blr"].set(ax.backlash_rate)
            self._vars[f"{prefix}_silent"].set(ax.silent)

        self._vars["guide_rate"].set(cfg.guide_rate)
        self._vars["rate1"].set(cfg.rate1)
        self._vars["rate2"].set(cfg.rate2)
        self._vars["rate3"].set(cfg.rate3)
        self._vars["max_rate"].set(cfg.max_rate)
        self._vars["default_rate"].set(cfg.default_rate)
        self._vars["deg_acc"].set(cfg.deg_acc)

        self._vars["horizon"].set(cfg.horizon)
        self._vars["overhead"].set(cfg.overhead)
        self._vars["under_pole"].set(cfg.under_pole)
        self._vars["meridian_e"].set(cfg.meridian_e)
        self._vars["meridian_w"].set(cfg.meridian_w)
        self._vars["axis1_min"].set(cfg.axis1_min)
        self._vars["axis1_max"].set(cfg.axis1_max)
        self._vars["axis2_min"].set(cfg.axis2_min)
        self._vars["axis2_max"].set(cfg.axis2_max)
        self._vars["dist_from_pole"].set(cfg.dist_from_pole)

        self._vars["enc1_pulse"].set(cfg.enc1_pulse)
        self._vars["enc1_reverse"].set(cfg.enc1_reverse)
        self._vars["enc2_pulse"].set(cfg.enc2_pulse)
        self._vars["enc2_reverse"].set(cfg.enc2_reverse)
        self._vars["enc_sync"].set(cfg.enc_sync)

        self._vars["latitude"].set(cfg.latitude)
        self._vars["longitude"].set(cfg.longitude)
        self._vars["elevation"].set(cfg.elevation)
        self._vars["timezone"].set(cfg.timezone)

        self._vars["refr_goto"].set(cfg.refr_goto)
        self._vars["refr_pole"].set(cfg.refr_pole)
        self._vars["refr_tracking"].set(cfg.refr_tracking)

        fw = cfg.meta.get("firmware", "?")
        self._status.set(
            f"{self._port}  FW {fw}  mount {cfg.mount_index}  "
            f"({cfg.mount_name_0!r} / {cfg.mount_name_1!r})"
        )

    def _collect(self) -> eeprom_mount.EepromMount:
        cfg = eeprom_mount.clone(self._cfg)
        cfg.mount_index = int(str(self._vars["mount_index"].get()))
        cfg.mount_name = str(self._vars["mount_name"].get())
        cfg.mount_name_0 = str(self._vars["mount_name_0"].get())
        cfg.mount_name_1 = str(self._vars["mount_name_1"].get())
        cfg.mtype = str(self._vars["mtype"].get())
        cfg.slew_settle = int(self._vars["slew_settle"].get())

        for prefix, ax in (("a1", cfg.axis1), ("a2", cfg.axis2)):
            ax.gear = float(self._vars[f"{prefix}_gear"].get())
            ax.steps = int(self._vars[f"{prefix}_steps"].get())
            ax.micro = int(str(self._vars[f"{prefix}_micro"].get()))
            ax.reverse = bool(self._vars[f"{prefix}_reverse"].get())
            ax.low_curr = int(self._vars[f"{prefix}_low"].get())
            ax.high_curr = int(self._vars[f"{prefix}_high"].get())
            ax.backlash = int(self._vars[f"{prefix}_bl"].get())
            ax.backlash_rate = int(self._vars[f"{prefix}_blr"].get())
            ax.silent = bool(self._vars[f"{prefix}_silent"].get())

        cfg.guide_rate = float(self._vars["guide_rate"].get())
        cfg.rate1 = int(self._vars["rate1"].get())
        cfg.rate2 = int(self._vars["rate2"].get())
        cfg.rate3 = int(self._vars["rate3"].get())
        cfg.max_rate = int(self._vars["max_rate"].get())
        cfg.default_rate = int(self._vars["default_rate"].get())
        cfg.deg_acc = int(self._vars["deg_acc"].get())

        cfg.horizon = int(self._vars["horizon"].get())
        cfg.overhead = int(self._vars["overhead"].get())
        cfg.under_pole = float(self._vars["under_pole"].get())
        cfg.meridian_e = int(self._vars["meridian_e"].get())
        cfg.meridian_w = int(self._vars["meridian_w"].get())
        cfg.axis1_min = int(self._vars["axis1_min"].get())
        cfg.axis1_max = int(self._vars["axis1_max"].get())
        cfg.axis2_min = int(self._vars["axis2_min"].get())
        cfg.axis2_max = int(self._vars["axis2_max"].get())
        cfg.dist_from_pole = int(self._vars["dist_from_pole"].get())

        cfg.enc1_pulse = int(self._vars["enc1_pulse"].get())
        cfg.enc1_reverse = bool(self._vars["enc1_reverse"].get())
        cfg.enc2_pulse = int(self._vars["enc2_pulse"].get())
        cfg.enc2_reverse = bool(self._vars["enc2_reverse"].get())
        cfg.enc_sync = int(self._vars["enc_sync"].get())

        cfg.latitude = str(self._vars["latitude"].get())
        cfg.longitude = str(self._vars["longitude"].get())
        cfg.elevation = int(self._vars["elevation"].get())
        cfg.timezone = float(self._vars["timezone"].get())

        cfg.refr_goto = bool(self._vars["refr_goto"].get())
        cfg.refr_pole = bool(self._vars["refr_pole"].get())
        cfg.refr_tracking = bool(self._vars["refr_tracking"].get())
        return cfg

    def _read_from_device(self) -> None:
        self.config(cursor="watch")
        self.update_idletasks()
        try:
            with self._session() as sess:
                cfg = eeprom_mount.read_eeprom(sess)
            self._populate(cfg)
            if cfg.warnings:
                self._status.set(self._status.get() + f"  ({len(cfg.warnings)} warn)")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, f"Read failed:\n{exc}", parent=self)
        finally:
            self.config(cursor="")

    def _write_to_device(self) -> None:
        if not messagebox.askyesno(
            "Write to EEPROM",
            "Write the displayed values to the MainUnit EEPROM?\n"
            "Values will be verified after write.",
            parent=self,
        ):
            return
        cfg = self._collect()
        self.config(cursor="watch")
        self.update_idletasks()
        try:
            with self._session() as sess:
                # Keep both mount names in EEPROM
                sess.send_ack(f":SXOB,{cfg.mount_name_0}#")
                sess.send_ack(f":SXOC,{cfg.mount_name_1}#")
                errors = eeprom_mount.write_eeprom(
                    sess, cfg, write_mount_type=self._write_mtype.get()
                )
                time.sleep(0.3)
                mismatches = eeprom_mount.verify_eeprom(sess, cfg)
                refreshed = eeprom_mount.read_eeprom(sess)
            self._populate(refreshed)
            if errors:
                messagebox.showwarning(
                    APP_NAME,
                    "Write finished with command issues:\n" + "\n".join(errors[:15]),
                    parent=self,
                )
            if mismatches:
                messagebox.showwarning(
                    APP_NAME,
                    "Verification found differences:\n" + "\n".join(mismatches[:20]),
                    parent=self,
                )
            elif not errors:
                messagebox.showinfo(
                    APP_NAME, "Write OK — parameters verified on device.", parent=self
                )
                self._status.set(self._status.get() + "  ·  verified")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, f"Write failed:\n{exc}", parent=self)
        finally:
            self.config(cursor="")

    def _activate_mount(self) -> None:
        idx = int(str(self._vars["mount_index"].get()))
        if not messagebox.askyesno(
            "Activate mount",
            f"Switch active mount to {idx}?\n"
            "The MainUnit will reboot. Click OK after it is back online, "
            "then values for that mount will be re-read.",
            parent=self,
        ):
            return
        try:
            with self._session() as sess:
                eeprom_mount.switch_mount_index(sess, idx)
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)
            return
        messagebox.showinfo(
            APP_NAME,
            "Mount index command sent (board rebooting).\n"
            "Click OK when the MainUnit is ready to re-read EEPROM.",
            parent=self,
        )
        # Re-probe baud — port may briefly disappear
        for _ in range(30):
            info, baud = probe_device(self._port)
            if info is not None and info.kind == "mainunit":
                self._baud = baud
                break
            time.sleep(1)
            self.update()
        self._read_from_device()

    def _save_file(self) -> None:
        cfg = self._collect()
        path = filedialog.asksaveasfilename(
            parent=self,
            initialdir=str(config_base_path()),
            defaultextension=".json",
            filetypes=[("TeenAstro EEPROM", "*.json"), ("All", "*.*")],
            initialfile=f"TeenAstro_eeprom_mount{cfg.mount_index}.json",
        )
        if not path:
            return
        Path(path).write_text(json.dumps(cfg.to_file_dict(), indent=4), encoding="utf-8")
        self._status.set(f"Saved {path}")

    def _load_file(self) -> None:
        path = filedialog.askopenfilename(
            parent=self,
            initialdir=str(config_base_path()),
            filetypes=[("JSON", "*.json"), ("All", "*.*")],
        )
        if not path:
            return
        try:
            data = json.loads(Path(path).read_text(encoding="utf-8"))
            cfg = eeprom_mount.EepromMount.from_file_dict(data)
            self._populate(cfg)
            self._status.set(f"Loaded {path}")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)


class FocuserEepromDialog(tk.Toplevel):
    """Edit Focuser EEPROM (same fields as ASCOM FocuserConfigEepromForm)."""

    def __init__(self, master: tk.Tk, port_name: str, baud: int = 9600) -> None:
        super().__init__(master)
        self.title("Focuser EEPROM Configuration")
        self.resizable(False, False)
        self.transient(master)
        self.grab_set()
        self._port = port_name
        self._baud = baud
        self._cfg = config_focuser.FocuserConfig()
        self._status = tk.StringVar(value="")
        self._vars: dict[str, tk.Variable] = {}
        self._build()
        self.after(50, self._read_from_device)

    def _build(self) -> None:
        f = ttk.Frame(self, padding=10)
        f.pack(fill=tk.BOTH, expand=True)
        fields = (
            ("parkPos", 0, 0, 999999),
            ("maxPos", 50000, 0, 999999),
            ("minSpeed", 1, 1, 100),
            ("maxSpeed", 20, 1, 100),
            ("cmdAcc", 10, 1, 100),
            ("manAcc", 10, 1, 100),
            ("manDec", 10, 1, 100),
            ("resolution", 1, 1, 100),
            ("current", 100, 10, 280),
            ("steprot", 200, 1, 400),
        )
        for i, (key, default, lo, hi) in enumerate(fields):
            self._vars[key] = tk.IntVar(value=default)
            sp = _spin(f, lo, hi, width=12)
            sp.configure(textvariable=self._vars[key])
            _add_row(f, i, key, sp)
        self._vars["reverse"] = tk.BooleanVar(value=False)
        self._vars["micro"] = tk.IntVar(value=16)
        ttk.Checkbutton(f, text="Reverse", variable=self._vars["reverse"]).grid(
            row=len(fields), column=0, columnspan=2, sticky="w", pady=4
        )
        micro = ttk.Combobox(
            f,
            textvariable=self._vars["micro"],
            values=[str(x) for x in (2, 4, 8, 16, 32, 64, 128, 256)],
            state="readonly",
            width=10,
        )
        _add_row(f, len(fields) + 1, "Microsteps", micro)

        btns = ttk.Frame(self, padding=(10, 0, 10, 10))
        btns.pack(fill=tk.X)
        ttk.Button(btns, text="Read", width=10, command=self._read_from_device).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Write to EEPROM", width=16, command=self._write_to_device).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Save…", width=8, command=self._save_file).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Load…", width=8, command=self._load_file).pack(
            side=tk.LEFT, padx=(0, 4)
        )
        ttk.Button(btns, text="Close", width=8, command=self.destroy).pack(side=tk.RIGHT)
        ttk.Label(self, textvariable=self._status).pack(anchor="w", padx=10, pady=(0, 6))

    def _populate(self, cfg: config_focuser.FocuserConfig) -> None:
        self._cfg = cfg
        for key, var in self._vars.items():
            if key == "reverse":
                var.set(bool(int(cfg.settings.get("reverse", 0))))
            elif key == "micro":
                var.set(str(cfg.settings.get("micro", 16)))
            else:
                var.set(int(cfg.settings.get(key, 0)))
        self._status.set(f"{self._port}  {cfg.meta.get('fv', '')}")

    def _collect(self) -> config_focuser.FocuserConfig:
        cfg = config_focuser.FocuserConfig(
            settings=dict(self._cfg.settings), meta=dict(self._cfg.meta)
        )
        for key, var in self._vars.items():
            if key == "reverse":
                cfg.settings[key] = 1 if var.get() else 0
            elif key == "micro":
                cfg.settings[key] = int(str(var.get()))
            else:
                cfg.settings[key] = int(var.get())
        return cfg

    def _read_from_device(self) -> None:
        self.config(cursor="watch")
        try:
            with SerialSession(self._port, baud=self._baud, timeout=1.0) as sess:
                cfg = config_focuser.read_focuser(sess)
            self._populate(cfg)
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, f"Read failed:\n{exc}", parent=self)
        finally:
            self.config(cursor="")

    def _write_to_device(self) -> None:
        if not messagebox.askyesno(
            "Write to EEPROM",
            "Write focuser values to EEPROM?\nValues will be verified after write.",
            parent=self,
        ):
            return
        cfg = self._collect()
        self.config(cursor="watch")
        try:
            with SerialSession(self._port, baud=self._baud, timeout=1.0) as sess:
                errors = config_focuser.write_focuser(sess, cfg)
                time.sleep(0.2)
                mismatches = config_focuser.verify_focuser(sess, cfg)
                refreshed = config_focuser.read_focuser(sess)
            self._populate(refreshed)
            if errors:
                messagebox.showwarning(
                    APP_NAME, "Write issues:\n" + "\n".join(errors[:12]), parent=self
                )
            if mismatches:
                messagebox.showwarning(
                    APP_NAME,
                    "Verification found differences:\n" + "\n".join(mismatches[:15]),
                    parent=self,
                )
            elif not errors:
                messagebox.showinfo(
                    APP_NAME, "Write OK — parameters verified on device.", parent=self
                )
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, f"Write failed:\n{exc}", parent=self)
        finally:
            self.config(cursor="")

    def _save_file(self) -> None:
        cfg = self._collect()
        path = filedialog.asksaveasfilename(
            parent=self,
            initialdir=str(config_base_path()),
            defaultextension=".json",
            filetypes=[("JSON", "*.json")],
            initialfile="TeenAstro_focuser.json",
        )
        if path:
            config_focuser.save_focuser(Path(path), cfg)
            self._status.set(f"Saved {path}")

    def _load_file(self) -> None:
        path = filedialog.askopenfilename(
            parent=self, initialdir=str(config_base_path()), filetypes=[("JSON", "*.json")]
        )
        if not path:
            return
        try:
            self._populate(config_focuser.load_focuser(Path(path)))
            self._status.set(f"Loaded {path}")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_NAME, str(exc), parent=self)


def open_mount_eeprom(master: tk.Tk) -> None:
    master.config(cursor="watch")
    master.update_idletasks()
    try:
        units = detect.find_main_units()
    except Exception as exc:  # noqa: BLE001
        messagebox.showerror(APP_NAME, str(exc), parent=master)
        return
    finally:
        master.config(cursor="")

    if not units:
        messagebox.showinfo(APP_NAME, "No TeenAstro MainUnit found.", parent=master)
        return
    if len(units) > 1:
        messagebox.showinfo(
            APP_NAME,
            "Several MainUnits connected — unplug the others.",
            parent=master,
        )
        return
    one = units[0]
    if not firmware_supported_for_config(one.firmware):
        messagebox.showerror(
            APP_NAME,
            f"EEPROM editor needs firmware 1.5 or newer (found {one.firmware}).",
            parent=master,
        )
        return
    info, baud = probe_device(one.port_name)
    if info is None:
        messagebox.showerror(APP_NAME, "Could not open MainUnit port.", parent=master)
        return
    MountEepromDialog(master, one.port_name, baud)


def open_focuser_eeprom(master: tk.Tk) -> None:
    master.config(cursor="watch")
    master.update_idletasks()
    try:
        units = detect.find_focusers()
    except Exception as exc:  # noqa: BLE001
        messagebox.showerror(APP_NAME, str(exc), parent=master)
        return
    finally:
        master.config(cursor="")

    if not units:
        messagebox.showinfo(APP_NAME, "No TeenAstro Focuser found.", parent=master)
        return
    if len(units) > 1:
        messagebox.showinfo(
            APP_NAME, "Several Focusers connected — unplug the others.", parent=master
        )
        return
    one = units[0]
    if not firmware_supported_for_config(one.firmware):
        messagebox.showerror(
            APP_NAME,
            f"EEPROM editor needs firmware 1.5 or newer (found {one.firmware}).",
            parent=master,
        )
        return
    FocuserEepromDialog(master, one.port_name, 9600)
