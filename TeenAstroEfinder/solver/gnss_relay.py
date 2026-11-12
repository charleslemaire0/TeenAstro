"""
USB GPS → NMEA relay for TeenAstroEfinder.

When the Pi sits on the main-unit GNSS UART, the eFinder can forward NMEA
sentences from a USB GPS dongle so the main unit keeps site/time sync.
"""
from __future__ import annotations

import glob
import logging
import threading
import time
from typing import Callable, List, Optional

LOG = logging.getLogger("gnss_relay")


def find_gps_ports(exclude: Optional[List[str]] = None) -> List[str]:
    exclude = set(exclude or [])
    candidates: List[str] = []
    for pattern in ("/dev/ttyACM*", "/dev/ttyUSB*"):
        for path in sorted(glob.glob(pattern)):
            if path not in exclude:
                candidates.append(path)
    return candidates


class GnssRelay:
    """Background reader that pushes complete NMEA lines to a callback."""

    def __init__(
        self,
        on_nmea: Callable[[bytes], None],
        baud: int = 9600,
        exclude_ports: Optional[List[str]] = None,
    ) -> None:
        self.on_nmea = on_nmea
        self.baud = baud
        self.exclude_ports = list(exclude_ports or [])
        self.enabled = False
        self.gps_present = False
        self.port_name: Optional[str] = None
        self._ser = None
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()

    def set_enabled(self, on: bool) -> None:
        with self._lock:
            self.enabled = on

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="gnss-relay", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._close_ser()
        if self._thread:
            self._thread.join(timeout=2.0)
            self._thread = None

    def _close_ser(self) -> None:
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:  # noqa: BLE001
                pass
            self._ser = None
        self.gps_present = False
        self.port_name = None

    def _open(self) -> bool:
        import serial  # type: ignore

        self._close_ser()
        for path in find_gps_ports(self.exclude_ports):
            try:
                self._ser = serial.Serial(path, baudrate=self.baud, timeout=0.2)
                self.port_name = path
                self.gps_present = True
                LOG.info("GPS opened on %s", path)
                return True
            except Exception as exc:  # noqa: BLE001
                LOG.debug("skip %s: %s", path, exc)
        self.gps_present = False
        return False

    def _run(self) -> None:
        buf = bytearray()
        next_scan = 0.0
        while not self._stop.is_set():
            if self._ser is None:
                now = time.time()
                if now >= next_scan:
                    self._open()
                    next_scan = now + 5.0
                time.sleep(0.2)
                continue

            try:
                waiting = self._ser.in_waiting
                chunk = self._ser.read(waiting or 1)
            except Exception as exc:  # noqa: BLE001
                LOG.warning("GPS read failed: %s", exc)
                self._close_ser()
                time.sleep(1.0)
                continue

            if not chunk:
                continue

            for b in chunk:
                if b in (10, 13):  # LF / CR
                    if buf.startswith(b"$") and len(buf) > 5:
                        line = bytes(buf) + b"\r\n"
                        if self.enabled:
                            try:
                                self.on_nmea(line)
                            except Exception as exc:  # noqa: BLE001
                                LOG.debug("nmea callback failed: %s", exc)
                    buf.clear()
                elif len(buf) < 120:
                    if not buf and b != ord("$"):
                        continue
                    buf.append(b)
                else:
                    buf.clear()
