"""Minimal LX200 serial session for config backup/restore."""

from __future__ import annotations

import time
from dataclasses import dataclass

import serial
from serial.tools import list_ports as serial_list_ports


@dataclass
class DeviceInfo:
    product: str
    firmware: str
    board: str
    driver: str
    kind: str  # "mainunit" | "focuser" | "unknown"


class SerialSession:
    def __init__(self, port_name: str, baud: int = 57600, timeout: float = 1.0) -> None:
        self.port_name = port_name
        self.baud = baud
        # Emulator / TCP: "socket://127.0.0.1:9997" or "tcp:127.0.0.1:9997"
        url = port_name
        if port_name.lower().startswith("tcp:"):
            url = "socket://" + port_name.split(":", 1)[1]
        if "://" in url:
            self._ser = serial.serial_for_url(
                url,
                baudrate=baud,
                timeout=timeout,
                write_timeout=timeout,
            )
        else:
            self._ser = serial.Serial(
                port_name,
                baudrate=baud,
                timeout=timeout,
                write_timeout=timeout,
                dsrdtr=False,
                rtscts=False,
            )
            self._ser.dtr = False
            self._ser.rts = False
        time.sleep(0.15)

    def close(self) -> None:
        try:
            if self._ser.is_open:
                self._ser.close()
        except Exception:  # noqa: BLE001
            pass

    def __enter__(self) -> SerialSession:
        return self

    def __exit__(self, *args) -> None:
        self.close()

    def query(self, cmd: str, max_len: int = 256) -> str:
        """Send :cmd# (or full :...#) and return reply without trailing #."""
        wire = cmd if cmd.startswith(":") else f":{cmd}#"
        if not wire.endswith("#"):
            wire += "#"
        self._ser.reset_input_buffer()
        self._ser.write(wire.encode("ascii", errors="ignore"))
        buf = bytearray()
        deadline = time.monotonic() + max(self._ser.timeout or 1.0, 0.5)
        while time.monotonic() < deadline and len(buf) < max_len:
            chunk = self._ser.read(1)
            if not chunk:
                if buf:
                    break
                continue
            buf.extend(chunk)
            if buf[-1:] == b"#":
                break
        text = buf.decode("ascii", errors="ignore")
        if text.endswith("#"):
            text = text[:-1]
        return text

    def send_ack(self, cmd: str) -> str:
        """Send a setter; return first response character(s)."""
        wire = cmd if cmd.startswith(":") else f":{cmd}"
        if not wire.endswith("#"):
            wire += "#"
        self._ser.reset_input_buffer()
        self._ser.write(wire.encode("ascii", errors="ignore"))
        time.sleep(0.05)
        waiting = self._ser.in_waiting
        if waiting:
            return self._ser.read(waiting).decode("ascii", errors="ignore")
        ch = self._ser.read(1)
        return ch.decode("ascii", errors="ignore") if ch else ""


def list_ports() -> list[str]:
    return [p.device for p in serial_list_ports.comports()]


def probe_device(port_name: str) -> tuple[DeviceInfo | None, int]:
    """Try common bauds; return (info, baud) or (None, 0)."""
    for baud in (57600, 115200, 9600):
        try:
            with SerialSession(port_name, baud=baud, timeout=0.4) as sess:
                product = sess.query("GVP")
                if product == "TeenAstro":
                    return (
                        DeviceInfo(
                            product=product,
                            firmware=sess.query("GVN") or "?",
                            board=sess.query("GVB") or "?",
                            driver=sess.query("GVb") or "?",
                            kind="mainunit",
                        ),
                        baud,
                    )
                fv = sess.query("FV")
                if fv and "TeenAstro Focuser" in fv:
                    parts = fv.split()
                    fw = "?"
                    for i, p in enumerate(parts):
                        if p == "Focuser" and i + 2 < len(parts):
                            fw = parts[i + 2]
                            break
                    return (
                        DeviceInfo(
                            product="TeenAstro Focuser",
                            firmware=fw,
                            board=parts[parts.index("Focuser") + 1] if "Focuser" in parts else "?",
                            driver="",
                            kind="focuser",
                        ),
                        baud,
                    )
        except Exception:  # noqa: BLE001
            continue
    return None, 0
