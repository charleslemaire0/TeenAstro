"""Flash MainUnit/Focuser (Teensy) and SHC (ESP8266)."""

from __future__ import annotations

import shutil
import subprocess
import sys
import time
import webbrowser
from pathlib import Path

from . import firmware as fw
from .paths import app_dir, which_tool


def _run(cmd: list[str], *, cwd: Path | None = None, hide_console: bool = True) -> None:
    kwargs: dict = {"cwd": str(cwd) if cwd else None, "check": False}
    if hide_console and sys.platform == "win32":
        kwargs["creationflags"] = subprocess.CREATE_NO_WINDOW
        si = subprocess.STARTUPINFO()
        si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        si.wShowWindow = 0
        kwargs["startupinfo"] = si
    proc = subprocess.run(cmd, **kwargs)
    if proc.returncode != 0:
        raise RuntimeError(f"Command failed ({proc.returncode}): {' '.join(cmd)}")


def _flash_teensy_cli(hex_path: Path, mcu: str) -> None:
    cli = which_tool("teensy_loader_cli")
    if cli is None:
        raise FileNotFoundError(
            "teensy_loader_cli not found.\n"
            "Install it or place it in the uploader tools folder."
        )
    _run([str(cli), f"--mcu={mcu}", "-w", "-v", str(hex_path)])


def _flash_teensy_post_compile(hex_stem: str, hex_dir: Path, mcu: str) -> None:
    """Windows PJRC tools (teensy_post_compile + teensy_reboot), same as VB uploader."""
    post = which_tool("teensy_post_compile")
    if post is None:
        raise FileNotFoundError(
            "teensy_post_compile.exe not found.\n"
            "Place PJRC Teensy tools next to the uploader, or install teensy_loader_cli."
        )
    tools = str(post.parent)
    base = [
        str(post),
        f"-file={hex_stem}",
        f"-path={hex_dir}",
        f"-tools={tools}",
        f"-board={mcu}",
    ]
    # Allow teensy_post_compile to show the Teensy Loader UI.
    _run(base, cwd=post.parent, hide_console=False)
    time.sleep(3)
    _run(base + ["-reboot"], cwd=post.parent, hide_console=False)


def flash_teensy(hex_stem: str, hex_path: Path, mcu: str) -> str:
    """Flash a .hex. Returns the backend used."""
    if which_tool("teensy_loader_cli") is not None:
        _flash_teensy_cli(hex_path, mcu)
        return "teensy_loader_cli"
    if sys.platform == "win32" and which_tool("teensy_post_compile") is not None:
        _flash_teensy_post_compile(hex_stem, hex_path.parent, mcu)
        return "teensy_post_compile"
    raise FileNotFoundError(
        "No Teensy flasher found.\n"
        "Install teensy_loader_cli (all platforms) or bundle teensy_post_compile.exe (Windows)."
    )


def upload_mainunit(version: str, latest: bool, pcb: str) -> str:
    stem = fw.mainunit_hex_stem(version, pcb)
    if not stem:
        raise ValueError("No firmware file for this board.")
    hex_path = fw.resolve_hex(version, latest, stem)
    return flash_teensy(stem, hex_path, fw.mainunit_mcu(pcb))


def upload_focuser(version: str, latest: bool, pcb: str) -> str:
    stem = fw.focuser_hex_stem(version, pcb)
    if not stem:
        raise ValueError("No firmware file for this board.")
    hex_path = fw.resolve_hex(version, latest, stem)
    return flash_teensy(stem, hex_path, fw.focuser_mcu(pcb))


def _esptool_cmd() -> list[str]:
    bundled = which_tool("esptool")
    if bundled is not None:
        return [str(bundled)]
    # Prefer module form so a frozen/venv Python works without a standalone exe.
    return [sys.executable, "-m", "esptool"]


def upload_shc_com(version: str, latest: bool, language: str, comport: str) -> None:
    if not comport:
        raise ValueError("Select a COM port.")
    bin_path = fw.resolve_shc_bin(version, latest, language)
    cmd = _esptool_cmd() + [
        "-vv",
        "-cd",
        "nodemcu",
        "-cb",
        "921600",
        "-cp",
        comport,
        "-ca",
        "0x00000",
        "-cf",
        str(bin_path),
    ]
    # Legacy esptool.exe used Arduino-style args; python esptool uses write_flash.
    if cmd[0].endswith("esptool.exe") or Path(cmd[0]).name.lower() == "esptool.exe":
        _run(cmd, cwd=app_dir())
        return
    # Modern esptool (PyPI): esptool.py --port COM --baud 921600 write_flash 0x0 file.bin
    modern = [
        *(_esptool_cmd()),
        "--chip",
        "esp8266",
        "--port",
        comport,
        "--baud",
        "921600",
        "write_flash",
        "0x0",
        str(bin_path),
    ]
    _run(modern)


def open_shc_wifi(ip: str) -> None:
    ip = (ip or "").strip()
    if not ip:
        raise ValueError("Enter an IP address.")
    webbrowser.open(f"http://{ip}/update")


def open_firmware_folder() -> None:
    from .paths import firmware_base_path

    folder = firmware_base_path()
    if sys.platform == "win32":
        os_start = ["explorer", str(folder)]
        subprocess.Popen(os_start)  # noqa: S603
    elif sys.platform == "darwin":
        subprocess.Popen(["open", str(folder)])  # noqa: S603
    else:
        opener = shutil.which("xdg-open")
        if opener:
            subprocess.Popen([opener, str(folder)])  # noqa: S603
        else:
            raise RuntimeError(f"Open this folder manually:\n{folder}")
