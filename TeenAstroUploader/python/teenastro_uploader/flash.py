"""Flash MainUnit/Focuser (Teensy) and SHC (ESP8266 or ESP32-S3)."""

from __future__ import annotations

import io
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import webbrowser
from contextlib import redirect_stderr, redirect_stdout
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
    proc = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    if proc.returncode != 0:
        detail = f"{proc.stdout or ''}{proc.stderr or ''}".strip()
        message = f"Command failed ({proc.returncode}): {' '.join(cmd)}"
        if detail:
            message = f"{message}\n{detail}"
        raise RuntimeError(message)


def _flash_teensy_cli(hex_path: Path, mcu: str) -> None:
    cli = which_tool("teensy_loader_cli")
    if cli is None:
        raise FileNotFoundError(
            "teensy_loader_cli not found.\n"
            "Install it or place it in the uploader tools folder."
        )
    _run([str(cli), f"--mcu={mcu}", "-w", "-v", str(hex_path)])


def _stage_post_compile_hex(hex_stem: str, hex_dir: Path) -> tuple[str, Path]:
    """Give teensy_post_compile a file name with no extra dots.

    Teensyduino 1.56 cuts -file at the first '.'. TeenAstro_1.6_250_TMC5160
    is then sent as TeenAstro_1.hex, and the loader reports the file missing.
    """
    if "." not in hex_stem:
        return hex_stem, hex_dir
    safe = hex_stem.replace(".", "_")
    stage = Path(tempfile.mkdtemp(prefix="teenastro_hex_"))
    shutil.copy2(hex_dir / f"{hex_stem}.hex", stage / f"{safe}.hex")
    return safe, stage


_LOADER_PORT = 3149


def _read_loader_line(sock: socket.socket) -> str:
    sock.settimeout(5)
    buf = b""
    while b"\n" not in buf:
        chunk = sock.recv(256)
        if not chunk:
            break
        buf += chunk
    return buf.decode("ascii", errors="replace").strip()


def _open_loader(tools: Path, wait_s: float = 20.0) -> socket.socket:
    """Connect to Teensy Loader, starting teensy.exe if it is not up yet.

    teensy_post_compile reports the loader missing even while it is already
    answering on port 3149. The uploader speaks that port itself.
    """
    teensy = tools / "teensy.exe"
    started = False
    deadline = time.time() + wait_s
    while time.time() < deadline:
        try:
            sock = socket.create_connection(("127.0.0.1", _LOADER_PORT), timeout=1)
        except OSError:
            if not started:
                if not teensy.is_file():
                    raise FileNotFoundError(f"teensy.exe not found in {tools}")
                subprocess.Popen([str(teensy)], cwd=str(tools), close_fds=True)
                started = True
            time.sleep(0.25)
            continue
        try:
            banner = _read_loader_line(sock)
        except OSError:
            sock.close()
            time.sleep(0.25)
            continue
        if banner.startswith("Teensy Loader"):
            return sock
        sock.close()
        time.sleep(0.25)
    raise RuntimeError(
        "Teensy Loader did not answer on localhost port 3149.\n"
        "Close Teensy Loader and try the upload again."
    )


def _loader_ok(sock: socket.socket, command: str) -> None:
    sock.sendall((command + "\n").encode("ascii"))
    reply = _read_loader_line(sock)
    if not reply.startswith("Ok"):
        raise RuntimeError(f"Teensy Loader rejected {command!r}\n{reply}")


def _teensy_serial_ports() -> list[str]:
    """COM ports of a running Teensy (not the HalfKay bootloader)."""
    from serial.tools import list_ports

    ports: list[str] = []
    for port in list_ports.comports():
        if port.vid == 0x16C0 and port.pid != 0x0478:
            ports.append(port.device)
    return ports


def _request_bootloader() -> None:
    """Ask the Teensy USB stack to enter program mode.

    Opening the serial port at 134 baud is the Teensyduino reboot request.
    teensy_reboot.exe cannot see the loader after we have used its port.
    """
    import serial

    ports = _teensy_serial_ports()
    if len(ports) != 1:
        raise RuntimeError(
            "Teensy Loader has the firmware and Auto is on.\n"
            "Press the program button on the Teensy to finish."
        )
    try:
        with serial.Serial(ports[0], 134, timeout=0.2):
            time.sleep(0.3)
    except serial.SerialException as exc:
        raise RuntimeError(
            "Teensy Loader has the firmware and Auto is on.\n"
            f"Could not reboot {ports[0]}. Press the program button on the Teensy."
        ) from exc


def _flash_teensy_post_compile(hex_stem: str, hex_dir: Path, mcu: str) -> None:
    """Program a Teensy through the Loader GUI, then ask the board to reboot."""
    del mcu  # board choice is already in the hex; reboot uses the USB serial port
    post = which_tool("teensy_post_compile")
    if post is None:
        raise FileNotFoundError(
            "teensy_post_compile.exe not found.\n"
            "Place PJRC Teensy tools next to the uploader, or install teensy_loader_cli."
        )
    hex_stem, hex_dir = _stage_post_compile_hex(hex_stem, hex_dir)
    tools = post.parent
    directory = str(hex_dir)
    if not directory.endswith(("\\", "/")):
        directory += "\\"
    sock = _open_loader(tools)
    try:
        _loader_ok(sock, "comment: TeenAstro Uploader")
        _loader_ok(sock, f"dir:{directory}")
        _loader_ok(sock, f"file:{hex_stem}.hex")
        _loader_ok(sock, "auto:on")
        _request_bootloader()
    finally:
        sock.close()


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


def shc_write_flash_args(chip: str, comport: str, bin_path: Path) -> list[str]:
    """Modern esptool args. S3 uses the merged image at 0x0 (same address as ESP8266)."""
    return [
        "--chip",
        chip,
        "--port",
        comport,
        "--baud",
        "921600",
        "write_flash",
        "0x0",
        str(bin_path),
    ]


def _invoke_esptool(args: list[str]) -> None:
    """Run PyPI esptool in-process so a frozen uploader can flash ESP32-S3.

    The bundled esptool.exe speaks the old ESP8266 CLI and cannot target esp32s3.
    esptool.main() may raise SystemExit; that must not close the uploader.
    """
    try:
        import esptool
    except ImportError as exc:
        raise FileNotFoundError(
            "esptool is not installed.\nInstall it with: pip install 'esptool>=4.7'"
        ) from exc
    main = getattr(esptool, "main", None)
    if main is None:
        import esptool.__main__ as esptool_main

        main = esptool_main.main
    log = io.StringIO()
    try:
        with redirect_stdout(log), redirect_stderr(log):
            try:
                main(args)
            except SystemExit as exc:
                code = exc.code if isinstance(exc.code, int) else 0
                if code not in (0, None):
                    raise RuntimeError(_esptool_failure(code, log.getvalue())) from exc
    except RuntimeError:
        raise
    except Exception as exc:  # noqa: BLE001 - FatalError and connect errors
        text = log.getvalue().strip()
        message = str(exc).strip() or exc.__class__.__name__
        if text and text not in message:
            message = f"{message}\n\n{text[-2000:]}"
        raise RuntimeError(message) from exc


def _esptool_failure(code: int, log: str) -> str:
    detail = f"esptool failed (exit {code})"
    text = log.strip()
    if text:
        detail += f"\n\n{text[-2000:]}"
    return detail


def upload_shc_com(
    version: str, latest: bool, language: str, comport: str, pcb: str = "0.x"
) -> None:
    if not comport:
        raise ValueError("Select a COM port.")
    bin_path = fw.resolve_shc_bin(version, latest, language, pcb)
    if fw.shc_is_s3(pcb):
        # Legacy esptool.exe has no ESP32-S3 support; always use the Python package.
        _invoke_esptool(shc_write_flash_args(fw.shc_chip(pcb), comport, bin_path))
        return
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
    _run([*(_esptool_cmd()), *shc_write_flash_args("esp8266", comport, bin_path)])


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
