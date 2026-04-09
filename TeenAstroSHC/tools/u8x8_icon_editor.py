#!/usr/bin/env python3
"""
Visual editor for TeenAstro SHC 16×16 U8g2 / U8X8 bitmaps (LSB-first XBM bytes in C).

Opens a .cpp source (default: SmartController_Display.cpp), lists 32-byte icon
arrays, lets you paint pixels and saves by replacing only the selected array
block. Large bitmaps (e.g. teenastro_bits) are skipped.

Run:  python TeenAstroSHC/tools/u8x8_icon_editor.py
       python TeenAstroSHC/tools/u8x8_icon_editor.py path/to/SmartController_Display.cpp
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
import tkinter as tk
from dataclasses import dataclass
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

# --- 16×16 U8g2 row packing: 2 bytes per row, LSB = left pixel within each byte ---
ICON_BYTES = 32
ICON_W = ICON_H = 16


def unpack_u8x8_16x16(data: list[int]) -> list[list[int]]:
    if len(data) != ICON_BYTES:
        raise ValueError("expected 32 bytes for 16×16 icon")
    rows: list[list[int]] = []
    for r in range(ICON_H):
        b0, b1 = data[r * 2], data[r * 2 + 1]
        row: list[int] = []
        for c in range(8):
            row.append(1 if (b0 >> c) & 1 else 0)
        for c in range(8, 16):
            row.append(1 if (b1 >> (c - 8)) & 1 else 0)
        rows.append(row)
    return rows


def pack_u8x8_16x16(rows: list[list[int]]) -> list[int]:
    out: list[int] = []
    for r in range(ICON_H):
        b0 = b1 = 0
        for c in range(16):
            bit = rows[r][c] & 1
            if c < 8:
                if bit:
                    b0 |= 1 << c
            else:
                if bit:
                    b1 |= 1 << (c - 8)
        out.extend([b0, b1])
    if len(out) != ICON_BYTES:
        raise ValueError("internal pack error")
    return out


BLOCK_RE = re.compile(
    r"static (const )?unsigned char (\w+)\s*\[\]\s*U8X8_PROGMEM\s*=\s*\{([^}]*)\}\s*;",
    re.MULTILINE,
)


@dataclass
class IconBlock:
    name: str
    start: int
    end: int
    is_const: bool
    bytes_: list[int]


def parse_icons(text: str, only_16x16: bool = True) -> list[IconBlock]:
    icons: list[IconBlock] = []
    for m in BLOCK_RE.finditer(text):
        is_const = m.group(1) is not None
        name = m.group(2)
        body = m.group(3)
        vals = [int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]+", body)]
        if only_16x16 and len(vals) != ICON_BYTES:
            continue
        icons.append(
            IconBlock(
                name=name,
                start=m.start(),
                end=m.end(),
                is_const=is_const,
                bytes_=vals,
            )
        )
    return icons


def format_icon_block(name: str, data: list[int], is_const: bool) -> str:
    const = "const " if is_const else ""
    lines = [f"static {const}unsigned char {name}[] U8X8_PROGMEM = {{"]
    parts = [f"0x{b:02x}" for b in data]
    for i in range(0, len(parts), 12):
        chunk = ", ".join(parts[i : i + 12])
        if i + 12 >= len(parts):
            lines.append(f"    {chunk} }};")
        else:
            lines.append(f"    {chunk},")
    return "\n".join(lines)


class U8x8EditorApp:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.text = path.read_text(encoding="utf-8", errors="replace")
        self.icons = parse_icons(self.text)
        self.icon_by_name = {b.name: b for b in self.icons}

        self.current_name: str | None = None
        self.rows: list[list[int]] = [
            [0] * ICON_W for _ in range(ICON_H)
        ]
        self.dirty = False
        self.backup = True
        self._paint_value: int | None = None
        self._last_paint_cell: tuple[int, int] | None = None

        self.root = tk.Tk()
        self.root.title(f"U8x8 icon editor — {path.name}")
        self.root.minsize(520, 420)

        self.cell = 22
        self.pad = 8

        menubar = tk.Menu(self.root)
        fm = tk.Menu(menubar, tearoff=0)
        fm.add_command(label="Open…", command=self.on_open)
        fm.add_command(label="Save", command=self.on_save, accelerator="Ctrl+S")
        fm.add_separator()
        fm.add_command(label="Quit", command=self.on_quit)
        menubar.add_cascade(label="File", menu=fm)
        self.root.config(menu=menubar)
        self.root.bind("<Control-s>", lambda e: self.on_save())
        self.root.protocol("WM_DELETE_WINDOW", self.on_quit)

        main = ttk.Frame(self.root, padding=8)
        main.pack(fill=tk.BOTH, expand=True)

        left = ttk.Frame(main)
        left.pack(side=tk.LEFT, fill=tk.Y)
        ttk.Label(left, text="Icons (16×16)").pack(anchor=tk.W)
        self.listbox = tk.Listbox(left, width=28, height=24, exportselection=False)
        self.listbox.pack(fill=tk.BOTH, expand=True)
        for b in self.icons:
            self.listbox.insert(tk.END, b.name)
        self.listbox.bind("<<ListboxSelect>>", self.on_select)

        right = ttk.Frame(main)
        right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(12, 0))

        tools = ttk.Frame(right)
        tools.pack(fill=tk.X)
        ttk.Button(tools, text="Clear", command=self.on_clear).pack(side=tk.LEFT, padx=(0, 6))
        ttk.Button(tools, text="Invert", command=self.on_invert).pack(side=tk.LEFT, padx=(0, 6))
        self.var_backup = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            tools, text="Backup .bak on save", variable=self.var_backup
        ).pack(side=tk.LEFT, padx=(12, 0))

        self.canvas = tk.Canvas(
            right,
            width=self.pad * 2 + ICON_W * self.cell,
            height=self.pad * 2 + ICON_H * self.cell,
            bg="#2d2d2d",
            highlightthickness=0,
        )
        self.canvas.pack(pady=(8, 0))
        self.canvas.bind("<ButtonPress-1>", self.on_paint_press)
        self.canvas.bind("<B1-Motion>", self.on_paint_motion)
        self.canvas.bind("<ButtonRelease-1>", self.on_paint_release)

        self.status = tk.StringVar(value="Open a file or select an icon.")
        ttk.Label(right, textvariable=self.status, wraplength=400).pack(
            anchor=tk.W, pady=(8, 0)
        )

        btns = ttk.Frame(right)
        btns.pack(fill=tk.X, pady=(8, 0))
        ttk.Button(btns, text="Save to source", command=self.on_save).pack(
            side=tk.LEFT
        )

        self.rect_ids: list[list[int]] = [[0] * ICON_W for _ in range(ICON_H)]
        self._build_grid()

    def _build_grid(self) -> None:
        for r in range(ICON_H):
            for c in range(ICON_W):
                x0 = self.pad + c * self.cell
                y0 = self.pad + r * self.cell
                x1 = x0 + self.cell - 1
                y1 = y0 + self.cell - 1
                self.rect_ids[r][c] = self.canvas.create_rectangle(
                    x0, y0, x1, y1, outline="#555", width=1
                )
        self._refresh_canvas()

    def _refresh_canvas(self) -> None:
        for r in range(ICON_H):
            for c in range(ICON_W):
                fill = "#f0f0f0" if self.rows[r][c] else "#101010"
                self.canvas.itemconfig(self.rect_ids[r][c], fill=fill)

    def on_select(self, _evt=None) -> None:
        sel = self.listbox.curselection()
        if not sel:
            return
        name = self.listbox.get(sel[0])
        if self.dirty and self.current_name and self.current_name != name:
            if not messagebox.askyesno(
                "Unsaved changes",
                f"Discard edits to “{self.current_name}”?",
            ):
                if self.current_name:
                    try:
                        idx = next(
                            i
                            for i, b in enumerate(self.icons)
                            if b.name == self.current_name
                        )
                        self.listbox.selection_clear(0, tk.END)
                        self.listbox.selection_set(idx)
                        self.listbox.see(idx)
                    except (StopIteration, KeyError):
                        pass
                return

        self.current_name = name
        block = self.icon_by_name[name]
        self.rows = unpack_u8x8_16x16(block.bytes_)
        self.dirty = False
        self._refresh_canvas()
        self.status.set(f"Editing {name} — click pixels to toggle (LSB = left).")

    def _cell_from_event(self, e) -> tuple[int, int] | None:
        x, y = e.x - self.pad, e.y - self.pad
        if x < 0 or y < 0:
            return None
        c, r = x // self.cell, y // self.cell
        if 0 <= r < ICON_H and 0 <= c < ICON_W:
            return r, c
        return None

    def on_paint_press(self, e) -> None:
        pos = self._cell_from_event(e)
        if pos is None or not self.current_name:
            self._paint_value = None
            self._last_paint_cell = None
            return
        r, c = pos
        self._paint_value = 1 - self.rows[r][c]
        self.rows[r][c] = self._paint_value
        self._last_paint_cell = (r, c)
        self.dirty = True
        self._refresh_canvas()

    def on_paint_motion(self, e) -> None:
        if self._paint_value is None:
            return
        pos = self._cell_from_event(e)
        if pos is None or not self.current_name:
            return
        r, c = pos
        if self._last_paint_cell == (r, c):
            return
        self.rows[r][c] = self._paint_value
        self._last_paint_cell = (r, c)
        self.dirty = True
        self._refresh_canvas()

    def on_paint_release(self, _e) -> None:
        self._paint_value = None
        self._last_paint_cell = None

    def on_clear(self) -> None:
        if not self.current_name:
            return
        self.rows = [[0] * ICON_W for _ in range(ICON_H)]
        self.dirty = True
        self._refresh_canvas()

    def on_invert(self) -> None:
        if not self.current_name:
            return
        for r in range(ICON_H):
            for c in range(ICON_W):
                self.rows[r][c] ^= 1
        self.dirty = True
        self._refresh_canvas()

    def on_open(self) -> None:
        p = filedialog.askopenfilename(
            title="Open C++ source",
            filetypes=[("C++", "*.cpp"), ("All", "*.*")],
        )
        if not p:
            return
        self._load_file(Path(p))

    def _load_file(self, path: Path) -> None:
        self.path = path
        self.text = path.read_text(encoding="utf-8", errors="replace")
        self.icons = parse_icons(self.text)
        self.icon_by_name = {b.name: b for b in self.icons}
        self.listbox.delete(0, tk.END)
        for b in self.icons:
            self.listbox.insert(tk.END, b.name)
        self.current_name = None
        self.dirty = False
        self.rows = [[0] * ICON_W for _ in range(ICON_H)]
        self._refresh_canvas()
        self.root.title(f"U8x8 icon editor — {path.name}")
        self.status.set(f"Loaded {len(self.icons)} icons from {path.name}")

    def on_save(self, _evt=None) -> None:
        if not self.current_name:
            messagebox.showinfo("Save", "Select an icon first.")
            return
        block = self.icon_by_name[self.current_name]
        new_bytes = pack_u8x8_16x16(self.rows)
        new_block = format_icon_block(
            block.name, new_bytes, block.is_const
        )
        before = self.text[: block.start]
        after = self.text[block.end :]
        new_text = before + new_block + after

        backup = self.var_backup.get()
        if backup:
            bak = self.path.with_suffix(self.path.suffix + ".bak")
            try:
                shutil.copy2(self.path, bak)
            except OSError as ex:
                messagebox.showerror("Backup failed", str(ex))
                return

        try:
            self.path.write_text(new_text, encoding="utf-8", newline="\n")
        except OSError as ex:
            messagebox.showerror("Save failed", str(ex))
            return

        self.text = new_text
        self.icons = parse_icons(self.text)
        self.icon_by_name = {b.name: b for b in self.icons}
        self.dirty = False
        self.status.set(f"Saved {block.name} to {self.path.name}")

    def on_quit(self) -> None:
        if self.dirty:
            if not messagebox.askyesno("Quit", "Discard unsaved pixel edits?"):
                return
        self.root.destroy()

    def run(self) -> None:
        self.root.mainloop()


def default_cpp_path() -> Path:
    return Path(__file__).resolve().parent.parent / "SmartController_Display.cpp"


def main() -> int:
    ap = argparse.ArgumentParser(description="Edit TeenAstro U8X8 16×16 icons in C++ source.")
    ap.add_argument(
        "cpp",
        nargs="?",
        type=Path,
        default=None,
        help="Path to SmartController_Display.cpp (default: next to this tool)",
    )
    args = ap.parse_args()
    path = args.cpp or default_cpp_path()
    if not path.is_file():
        print(f"File not found: {path}", file=sys.stderr)
        return 1
    U8x8EditorApp(path).run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
