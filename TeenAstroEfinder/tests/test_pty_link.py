"""Optional pty integration test (POSIX only) for TeenAstroLink over a serial-like port."""
from __future__ import annotations

import os
import sys
import threading
import time
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

pytest.importorskip("pty")
import pty  # noqa: E402

from solver.teenastro_link import SolveOutcome, TeenAstroLink  # noqa: E402


@pytest.mark.skipif(not hasattr(os, "openpty"), reason="no pty on this platform")
def test_pty_probe_and_solve():
    master, slave = pty.openpty()
    slave_name = os.ttyname(slave)

    def on_solve():
        link.set_solved(SolveOutcome(ok=True, ra_hours=3.0, dec_deg=-10.0, stars=55, ms=12))

    link = TeenAstroLink(
        on_solve=on_solve,
        on_offset=lambda: None,
        on_reset_offset=lambda: None,
        port=slave_name,
    )
    # Open using the slave side as the "device"
    import serial

    link._ser = serial.Serial(slave_name, baudrate=9600, timeout=0.05)
    stop = threading.Event()

    def loop():
        while not stop.is_set():
            n = link._ser.in_waiting
            if n:
                link.feed(link._ser.read(n))
            else:
                time.sleep(0.01)

    t = threading.Thread(target=loop, daemon=True)
    t.start()
    try:
        os.write(master, b":e?#")
        time.sleep(0.2)
        reply = os.read(master, 64).decode("ascii", errors="ignore")
        assert "TeenAstroEfinder" in reply

        os.write(master, b":eS#")
        time.sleep(0.2)
        _ = os.read(master, 16)
        for _ in range(50):
            if not link.state.busy:
                break
            time.sleep(0.02)
        os.write(master, b":eR#")
        time.sleep(0.2)
        reply = os.read(master, 64).decode("ascii", errors="ignore")
        assert reply.startswith("S3.000000,-10.00000")
    finally:
        stop.set()
        link.close()
        os.close(master)
        os.close(slave)
