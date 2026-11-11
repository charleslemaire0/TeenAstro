"""
UART slave link for TeenAstroEfinder.

Speaks the :e…# protocol expected by TeenAstroMainUnit::EfinderManager.
Baud is auto-scanned between 56000 (focuser port) and 9600 (GNSS port).
"""
from __future__ import annotations

import logging
import threading
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Callable, Optional

LOG = logging.getLogger("teenastro_link")

VERSION = "0.1.0"
BAUD_CANDIDATES = (56000, 9600)
BAUD_SCAN_PERIOD_S = 8.0


@dataclass
class SiteTime:
    lat: float = 0.0
    lon_east: float = 0.0
    elev: float = 0.0
    utc: Optional[datetime] = None


@dataclass
class SolveOutcome:
    ok: bool = False
    ra_hours: float = 0.0
    dec_deg: float = 0.0
    stars: int = 0
    ms: int = 0
    fail_code: str = ""
    offset_dx: float = 0.0
    offset_dy: float = 0.0
    offset_name: str = ""
    is_offset: bool = False


@dataclass
class LinkState:
    site: SiteTime = field(default_factory=SiteTime)
    busy: bool = False
    last: SolveOutcome = field(default_factory=SolveOutcome)
    nmea_relay: bool = False
    gps_present: bool = False


class TeenAstroLink:
    """Parse LX200-style frames and dispatch to callbacks."""

    def __init__(
        self,
        on_solve: Callable[[], None],
        on_offset: Callable[[], None],
        on_reset_offset: Callable[[], None],
        on_param: Optional[Callable[[str], str]] = None,
        on_nmea_enable: Optional[Callable[[bool], None]] = None,
        port: str = "/dev/serial0",
    ) -> None:
        self.on_solve = on_solve
        self.on_offset = on_offset
        self.on_reset_offset = on_reset_offset
        self.on_param = on_param
        self.on_nmea_enable = on_nmea_enable
        self.port_name = port
        self.state = LinkState()
        self._ser = None
        self._rx = bytearray()
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._baud_idx = 0
        self._baud_locked = False
        self._last_good_frame = 0.0

    def open(self) -> None:
        import serial  # type: ignore

        baud = BAUD_CANDIDATES[self._baud_idx]
        self._ser = serial.Serial(self.port_name, baudrate=baud, timeout=0.05)
        LOG.info("opened %s at %s baud", self.port_name, baud)

    def _reopen_next_baud(self) -> None:
        import serial  # type: ignore

        if self._ser:
            try:
                self._ser.close()
            except Exception:  # noqa: BLE001
                pass
            self._ser = None
        self._baud_idx = (self._baud_idx + 1) % len(BAUD_CANDIDATES)
        baud = BAUD_CANDIDATES[self._baud_idx]
        self._ser = serial.Serial(self.port_name, baudrate=baud, timeout=0.05)
        self._rx = bytearray()
        LOG.info("switched %s to %s baud", self.port_name, baud)

    def close(self) -> None:
        self._stop.set()
        if self._ser:
            self._ser.close()
            self._ser = None

    def write_reply(self, text: str) -> None:
        if not self._ser:
            return
        data = text.encode("ascii", errors="ignore")
        with self._lock:
            self._ser.write(data)
            self._ser.flush()
        LOG.debug("tx %s", text)

    def write_nmea(self, line: bytes) -> None:
        """Forward a raw NMEA sentence (already terminated) onto the TeenAstro UART."""
        if not self._ser or not self.state.nmea_relay:
            return
        with self._lock:
            self._ser.write(line)
            self._ser.flush()

    def set_solved(self, outcome: SolveOutcome) -> None:
        with self._lock:
            self.state.last = outcome
            self.state.busy = False

    def set_busy(self) -> None:
        with self._lock:
            self.state.busy = True
            self.state.last = SolveOutcome()

    def handle_frame(self, body: str) -> None:
        """body is the command without leading ':' and trailing '#'."""
        if not body.startswith("e"):
            return
        self._baud_locked = True
        self._last_good_frame = time.time()
        cmd = body[1:]
        if cmd == "?":
            self.write_reply(f"TeenAstroEfinder,{VERSION}#")
            return
        if cmd.startswith("T") and len(cmd) >= 15:
            try:
                dt = datetime.strptime(cmd[1:15], "%Y%m%d%H%M%S").replace(tzinfo=timezone.utc)
                self.state.site.utc = dt
                self.write_reply("1#")
            except ValueError:
                self.write_reply("0#")
            return
        if cmd.startswith("L"):
            try:
                parts = cmd[1:].split(",")
                self.state.site.lat = float(parts[0])
                self.state.site.lon_east = float(parts[1])
                self.state.site.elev = float(parts[2]) if len(parts) > 2 else 0.0
                self.write_reply("1#")
            except (ValueError, IndexError):
                self.write_reply("0#")
            return
        if cmd == "S":
            if self.state.busy:
                self.write_reply("0#")
                return
            self.set_busy()
            self.write_reply("1#")
            threading.Thread(target=self.on_solve, daemon=True).start()
            return
        if cmd == "O":
            if self.state.busy:
                self.write_reply("0#")
                return
            self.set_busy()
            self.write_reply("1#")
            threading.Thread(target=self.on_offset, daemon=True).start()
            return
        if cmd == "o":
            self.on_reset_offset()
            self.write_reply("1#")
            return
        if cmd == "R":
            with self._lock:
                if self.state.busy:
                    self.write_reply("B#")
                    return
                last = self.state.last
            if last.is_offset and last.ok:
                self.write_reply(
                    f"O{last.offset_dx:.2f},{last.offset_dy:.2f},{last.offset_name}#"
                )
            elif last.ok:
                self.write_reply(
                    f"S{last.ra_hours:.6f},{last.dec_deg:.5f},{last.stars},{last.ms}#"
                )
            elif last.fail_code:
                self.write_reply(f"F{last.fail_code}#")
            else:
                self.write_reply("N#")
            return
        if cmd == "N1":
            self.state.nmea_relay = True
            if self.on_nmea_enable:
                self.on_nmea_enable(True)
            self.write_reply("1#")
            return
        if cmd == "N0":
            self.state.nmea_relay = False
            if self.on_nmea_enable:
                self.on_nmea_enable(False)
            self.write_reply("1#")
            return
        if cmd == "N?":
            self.write_reply("1#" if self.state.gps_present else "0#")
            return
        if cmd.startswith("X") and self.on_param:
            self.write_reply(self.on_param(cmd[1:]))
            return
        self.write_reply("0#")

    def feed(self, data: bytes) -> None:
        for b in data:
            if b == ord(":"):
                self._rx = bytearray()
                continue
            if b == ord("#"):
                try:
                    body = self._rx.decode("ascii", errors="ignore")
                except Exception:  # noqa: BLE001
                    body = ""
                self._rx = bytearray()
                if body:
                    LOG.debug("rx :%s#", body)
                    self.handle_frame(body)
                continue
            if 32 <= b < 127 and len(self._rx) < 80:
                self._rx.append(b)

    def run_forever(self) -> None:
        if not self._ser:
            self.open()
        assert self._ser is not None
        self._last_good_frame = time.time()
        while not self._stop.is_set():
            waiting = self._ser.in_waiting
            if waiting:
                self.feed(self._ser.read(waiting))
            else:
                time.sleep(0.01)
            if (
                not self._baud_locked
                and time.time() - self._last_good_frame > BAUD_SCAN_PERIOD_S
            ):
                try:
                    self._reopen_next_baud()
                    self._last_good_frame = time.time()
                except Exception as exc:  # noqa: BLE001
                    LOG.warning("baud rescan failed: %s", exc)
                    time.sleep(1.0)
