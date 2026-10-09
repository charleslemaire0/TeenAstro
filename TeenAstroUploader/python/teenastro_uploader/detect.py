"""Serial auto-detect for MainUnit and Focuser."""

from __future__ import annotations

import time
from dataclasses import dataclass

import serial
from serial.tools import list_ports


@dataclass
class DetectedMainUnit:
    port_name: str
    firmware: str
    board: int
    driver: int
    pcb: str | None


@dataclass
class DetectedFocuser:
    port_name: str
    firmware: str
    board_text: str
    driver: int
    pcb: str | None


def pcb_from_board(board: int, driver: int) -> str | None:
    if board == 220 and driver == 1:
        return "2.2 TMC260"
    if board == 230 and driver == 1:
        return "2.3 TMC260"
    if board == 240 and driver == 2:
        return "2.4 TMC2130"
    if board == 240 and driver == 3:
        return "2.4 TMC5160"
    if board == 250 and driver == 2:
        return "2.5 TMC2130"
    if board == 250 and driver == 3:
        return "2.5 TMC5160"
    return None


def pcb_from_focuser(board_text: str, driver: int) -> str | None:
    board = board_text[:-2] if board_text.endswith(".0") else board_text
    if board == "2.2":
        return "2.2 TMC2130"
    if board == "2.3":
        return "2.3 TMC2130"
    if board == "2.4":
        if driver == 3:
            return "2.4 TMC5160"
        if driver == 2:
            return "2.4 TMC2130"
    return None


def _lx200_query(port: serial.Serial, cmd: str, timeout_ms: float = 350) -> str | None:
    port.reset_input_buffer()
    port.write(cmd.encode("ascii", errors="ignore"))
    buf = ""
    deadline = time.monotonic() + timeout_ms / 1000.0
    while time.monotonic() < deadline:
        waiting = port.in_waiting
        if waiting:
            buf += port.read(waiting).decode("ascii", errors="ignore")
            if "#" in buf:
                break
        else:
            time.sleep(0.015)
    hash_at = buf.find("#")
    if hash_at < 0:
        return None
    return buf[:hash_at].strip()


def try_read_mainunit(port_name: str, baud: int) -> DetectedMainUnit | None:
    port: serial.Serial | None = None
    try:
        port = serial.Serial(
            port_name,
            baudrate=baud,
            timeout=0.3,
            write_timeout=0.3,
            dsrdtr=False,
            rtscts=False,
        )
        port.dtr = False
        port.rts = False
        time.sleep(0.12)
        product = _lx200_query(port, ":GVP#")
        if product != "TeenAstro":
            return None
        fw = _lx200_query(port, ":GVN#") or "?"
        board_text = _lx200_query(port, ":GVB#")
        driver_text = _lx200_query(port, ":GVb#")
        try:
            board = int(board_text)  # type: ignore[arg-type]
            driver = int(driver_text)  # type: ignore[arg-type]
        except (TypeError, ValueError):
            return None
        return DetectedMainUnit(
            port_name=port_name,
            firmware=fw,
            board=board,
            driver=driver,
            pcb=pcb_from_board(board, driver),
        )
    except Exception:  # noqa: BLE001
        return None
    finally:
        if port is not None:
            try:
                port.close()
            except Exception:  # noqa: BLE001
                pass


def try_read_focuser(port_name: str) -> DetectedFocuser | None:
    port: serial.Serial | None = None
    try:
        port = serial.Serial(
            port_name,
            baudrate=9600,
            timeout=0.3,
            write_timeout=0.3,
            dsrdtr=False,
            rtscts=False,
        )
        port.dtr = False
        port.rts = False
        time.sleep(0.12)
        reply = _lx200_query(port, ":FV#")
        if not reply or "TeenAstro Focuser" not in reply:
            return None
        parts = reply.split()
        board_text = None
        firmware = "?"
        driver = 0
        for i, part in enumerate(parts):
            if part == "Focuser" and i + 1 < len(parts):
                board_text = parts[i + 1]
                if i + 2 < len(parts):
                    firmware = parts[i + 2]
                if i + 3 < len(parts):
                    try:
                        driver = int(parts[i + 3])
                    except ValueError:
                        driver = 0
                break
        if board_text is None:
            return None
        return DetectedFocuser(
            port_name=port_name,
            firmware=firmware,
            board_text=board_text,
            driver=driver,
            pcb=pcb_from_focuser(board_text, driver),
        )
    except Exception:  # noqa: BLE001
        return None
    finally:
        if port is not None:
            try:
                port.close()
            except Exception:  # noqa: BLE001
                pass


def list_serial_ports() -> list[str]:
    return [p.device for p in list_ports.comports()]


def find_main_units() -> list[DetectedMainUnit]:
    found: list[DetectedMainUnit] = []
    for port_name in list_serial_ports():
        unit = try_read_mainunit(port_name, 57600) or try_read_mainunit(port_name, 115200)
        if unit is not None:
            found.append(unit)
    return found


def find_focusers() -> list[DetectedFocuser]:
    found: list[DetectedFocuser] = []
    for port_name in list_serial_ports():
        unit = try_read_focuser(port_name)
        if unit is not None:
            found.append(unit)
    return found
