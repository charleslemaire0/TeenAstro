"""Unit tests for the TeenAstro link parser (no serial hardware)."""
from __future__ import annotations

import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from solver.teenastro_link import SolveOutcome, TeenAstroLink


class FakeLink(TeenAstroLink):
    def __init__(self):
        self.replies = []
        self.nmea_flag = None
        super().__init__(
            on_solve=self._solve,
            on_offset=self._offset,
            on_reset_offset=lambda: None,
            on_nmea_enable=self._nmea,
            port="unused",
        )

    def write_reply(self, text: str) -> None:
        self.replies.append(text)

    def _solve(self) -> None:
        self.set_solved(SolveOutcome(ok=True, ra_hours=1.0, dec_deg=2.0, stars=10, ms=5))

    def _offset(self) -> None:
        self.set_solved(
            SolveOutcome(ok=True, is_offset=True, offset_dx=1.5, offset_dy=-0.5, offset_name="x")
        )

    def _nmea(self, on: bool) -> None:
        self.nmea_flag = on


def test_probe():
    link = FakeLink()
    link.feed(b":e?#")
    assert link.replies[-1].startswith("TeenAstroEfinder,")
    assert link.replies[-1].endswith("#")


def test_site_and_time():
    link = FakeLink()
    link.feed(b":eT20260101120000#")
    assert link.replies[-1] == "1#"
    link.feed(b":eL48.1,2.3,100#")
    assert link.replies[-1] == "1#"
    assert abs(link.state.site.lat - 48.1) < 1e-6


def test_pointing():
    link = FakeLink()
    link.feed(b":eC12.345678,-5.43210#")
    assert link.replies[-1] == "1#"
    assert abs(link.state.site.ra_hours - 12.345678) < 1e-6
    assert abs(link.state.site.dec_deg - (-5.43210)) < 1e-6
    assert link.state.site.pointing_valid is True


def test_solve_poll():
    link = FakeLink()
    link.feed(b":eS#")
    assert link.replies[-1] == "1#"
    for _ in range(50):
        if not link.state.busy:
            break
        time.sleep(0.02)
    link.feed(b":eR#")
    assert link.replies[-1].startswith("S1.000000,2.00000,")


def test_nmea_commands():
    link = FakeLink()
    link.state.gps_present = True
    link.feed(b":eN?#")
    assert link.replies[-1] == "1#"
    link.feed(b":eN1#")
    assert link.replies[-1] == "1#"
    assert link.nmea_flag is True
    assert link.state.nmea_relay is True
    link.feed(b":eN0#")
    assert link.nmea_flag is False


def test_ignore_focuser_probe():
    link = FakeLink()
    link.feed(b":F?#")
    assert link.replies == []
