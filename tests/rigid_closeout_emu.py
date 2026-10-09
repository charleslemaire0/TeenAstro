#!/usr/bin/env python3
"""Emulator regressions for alignment close-out sync policy.

Rigid (:A0,rN#) must NOT sync after a successful fit: the encoder frame is the
one set by the first-star sync, and rewriting it by the last-star residual is
what threw side A off after a 3+3 session.

Classic two-star (:A0,2#) MUST still sync on the closing star: that is how a
two-star model absorbs the last-star residual into the motor origin.

Both paths are exercised with a deliberate ~30' catalog error on the closing
star (RA for rigid, Dec for classic so the offset stays large near the pole).
Checks:

  * motor step counts (:GXDP0# / :GXDP1#) around the closing :An#
  * reported :GR# before vs after (semantic: did the sky origin move?)

Also checks that a narrow rigid session with no published cone still keeps a
tight :GXAr# (multi-star T retained, not a two-star wipe).

Usage:
    pio run -d TeenAstroEmulator -e emu_mainunit
    python tests/rigid_closeout_emu.py
"""

import socket
import subprocess
import sys
import time
from pathlib import Path

HOST = "127.0.0.1"
PORT = 9997
EMU = Path(__file__).resolve().parents[1] / "TeenAstroEmulator" / ".pio" / "build" / "emu" / "mainunit_emu.exe"

# Six nudges for the rigid session; classic reuses the first two.
NUDGES = [
    ("Mn", 9.0, "Me", 4.0),
    ("Mn", 9.0, "Mw", 7.0),
    ("Mn", 9.0, "Me", 6.0),
    ("Ms", 7.0, "Mw", 3.0),
    ("Ms", 9.0, "Me", 7.0),
    ("Mn", 5.0, "Mw", 4.0),
]

# A 2-minute RA sync on typical emu gearing is thousands of steps; tracking
# during the close-out reply is at most a handful.
SYNC_STEP_FLOOR = 500

failures = []


def check(cond, what):
    print(("  ok   " if cond else "  FAIL ") + what)
    if not cond:
        failures.append(what)


class Link:
    def __init__(self, sock):
        self.sock = sock

    def ch(self, cmd):
        self.sock.sendall(cmd.encode())
        self.sock.settimeout(8.0)
        try:
            return self.sock.recv(1).decode(errors="replace")
        except socket.timeout:
            return ""

    def txt(self, cmd):
        self.sock.sendall(cmd.encode())
        self.sock.settimeout(8.0)
        buf = b""
        try:
            while not buf.endswith(b"#") and len(buf) < 512:
                chunk = self.sock.recv(1)
                if not chunk:
                    break
                buf += chunk
        except socket.timeout:
            pass
        return buf.decode(errors="replace").rstrip("#")

    def none(self, cmd, wait=0.4):
        self.sock.sendall(cmd.encode())
        self.sock.settimeout(wait)
        try:
            return self.sock.recv(64).decode(errors="replace")
        except socket.timeout:
            return ""

    def num(self, cmd):
        try:
            return float(self.txt(cmd).strip())
        except ValueError:
            return float("nan")

    def steps(self):
        """Raw axis step counts — integer, so a sync jump is unambiguous."""
        a1 = self.txt(":GXDP0#").strip()
        a2 = self.txt(":GXDP1#").strip()
        try:
            return int(a1), int(a2)
        except ValueError:
            return None, None


def nudge(link, axis2cmd, t2, axis1cmd, t1):
    link.none(":R4#")
    for cmd, dur in ((axis2cmd, t2), (axis1cmd, t1)):
        link.none(":%s#" % cmd)
        time.sleep(dur)
        link.none(":Q%s#" % cmd[1])
    time.sleep(0.4)


def point_at_self(link):
    ra = link.txt(":GR#")
    dec = link.txt(":GD#")
    if not ra or not dec:
        return False
    return link.ch(":Sr%s#" % ra) == "1" and link.ch(":Sd%s#" % dec) == "1"


def ra_to_seconds(ra):
    parts = ra.strip().replace("+", "").split(":")
    if len(parts) != 3:
        return None
    h, m, s = (int(parts[0]), int(parts[1]), float(parts[2]))
    return h * 3600.0 + m * 60.0 + s


def ra_plus_minutes(ra, minutes):
    """Offset an LX200 RA string HH:MM:SS by whole minutes (wrap 24h)."""
    parts = ra.strip().replace("+", "").split(":")
    if len(parts) != 3:
        return ra
    h, m, s = (int(parts[0]), int(parts[1]), int(float(parts[2])))
    total = h * 60 + m + minutes
    total %= 24 * 60
    return "%02d:%02d:%02d" % (total // 60, total % 60, s)


def ra_delta_seconds(a, b):
    sa, sb = ra_to_seconds(a), ra_to_seconds(b)
    if sa is None or sb is None:
        return float("inf")
    d = abs(sa - sb)
    return min(d, 24 * 3600 - d)


def point_with_ra_offset(link, minutes):
    ra = link.txt(":GR#")
    dec = link.txt(":GD#")
    if not ra or not dec:
        return None
    ra2 = ra_plus_minutes(ra, minutes)
    print("       declare RA %s (was %s) Dec %s" % (ra2, ra, dec))
    if link.ch(":Sr%s#" % ra2) != "1" or link.ch(":Sd%s#" % dec) != "1":
        return None
    return ra, ra2


def dec_plus_arcmin(dec, arcmin):
    """Offset an LX200 Dec string sDD*MM:SS by whole arcminutes (clamp ±90)."""
    s = dec.strip()
    sign = -1 if s.startswith("-") else 1
    body = s[1:] if s[0] in "+-" else s
    parts = body.replace("*", ":").split(":")
    if len(parts) != 3:
        return dec
    d, m, sec = int(parts[0]), int(parts[1]), int(float(parts[2]))
    total = sign * (d * 60 + m) + arcmin
    if total > 90 * 60 - 1:
        total = 90 * 60 - 1
    if total < -(90 * 60 - 1):
        total = -(90 * 60 - 1)
    sign_ch = "+" if total >= 0 else "-"
    total = abs(total)
    return "%s%02d*%02d:%02d" % (sign_ch, total // 60, total % 60, sec)


def point_with_dec_offset(link, arcmin):
    """Catalog offset in Dec — sky angle is uniform, unlike RA near the pole."""
    ra = link.txt(":GR#")
    dec = link.txt(":GD#")
    if not ra or not dec:
        return None
    dec2 = dec_plus_arcmin(dec, arcmin)
    print("       declare RA %s Dec %s (was %s)" % (ra, dec2, dec))
    if link.ch(":Sr%s#" % ra) != "1" or link.ch(":Sd%s#" % dec2) != "1":
        return None
    return dec, dec2


def dec_delta_arcmin(a, b):
    def to_arcmin(d):
        s = d.strip()
        sign = -1 if s.startswith("-") else 1
        body = s[1:] if s[0] in "+-" else s
        parts = body.replace("*", ":").split(":")
        if len(parts) != 3:
            return None
        return sign * (int(parts[0]) * 60 + int(parts[1]) + float(parts[2]) / 60.0)

    aa, bb = to_arcmin(a), to_arcmin(b)
    if aa is None or bb is None:
        return float("inf")
    return abs(aa - bb)


def step_jump(before, after):
    if before[0] is None or after[0] is None:
        return None
    return abs(after[0] - before[0]) + abs(after[1] - before[1])


def _kill_listeners_on_port(port):
    """Drop any leftover mainunit_emu so we always exercise the binary on disk."""
    if sys.platform == "win32":
        try:
            out = subprocess.check_output(
                ["cmd", "/c", "netstat", "-ano"],
                text=True, errors="replace")
        except (OSError, subprocess.CalledProcessError):
            return
        marker = ":%d" % port
        pids = set()
        for line in out.splitlines():
            if marker not in line or "LISTENING" not in line:
                continue
            parts = line.split()
            if parts:
                pids.add(parts[-1])
        for pid in pids:
            subprocess.call(["taskkill", "/F", "/PID", pid],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.5)


def connect():
    # Always start our own process from EMU: a leftover listener may be an
    # older build with the wrong close-out sync policy.
    _kill_listeners_on_port(PORT)
    if not EMU.exists():
        print("emulator not built: %s" % EMU)
        print("run: pio run -d TeenAstroEmulator -e emu_mainunit")
        return None, None
    print("starting emulator ...")
    proc = subprocess.Popen([str(EMU)], cwd=str(EMU.parent),
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(40):
        time.sleep(0.5)
        try:
            return socket.create_connection((HOST, PORT), timeout=2.0), proc
        except OSError:
            continue
    proc.kill()
    print("could not connect to emulator")
    return None, None


def run_rigid_closeout(link):
    n = len(NUDGES)
    print("\n-- rigid %d-star close-out (last star +30' catalog error) --" % n)
    check(link.ch(":A0,r%d#" % n) == "1", ":A0,r%d# accepted" % n)

    declared = None
    for i, (a2, t2, a1, t1) in enumerate(NUDGES, start=1):
        nudge(link, a2, t2, a1, t1)
        if i < n:
            ok = point_at_self(link)
        else:
            declared = point_with_ra_offset(link, minutes=2)
            ok = declared is not None
        if not ok:
            check(False, "set target for star %d" % i)
            return
        if i == n:
            before = link.steps()
            ra_before = link.txt(":GR#")
            print("       steps before :A%d# = %s" % (i, before))
            print("       GR before    = %s" % ra_before)
        check(link.ch(":A%d#" % i) == "1", ":A%d# accepted" % i)
        if i == n:
            after = link.steps()
            ra_after = link.txt(":GR#")
            print("       steps after  :A%d# = %s" % (i, after))
            print("       GR after     = %s" % ra_after)
            jump = step_jump(before, after)
            check(jump is not None, "read axis step counts around rigid close-out")
            check(jump == 0, "motor steps unchanged after rigid close-out (no sync)")
            # Sidereal tracking keeps :GR# moving during the fit reply, so only
            # require that we did not snap to the false catalog RA (~120s off).
            if declared:
                check(ra_delta_seconds(ra_after, declared[1]) > 60.0,
                      "GR did not jump to the offset catalog RA")

    rms = link.num(":GXAr#")
    mask = link.txt(":GXAf#")
    print('  terms %s  rms %.3f"' % (mask, rms))
    check(rms == rms and rms < 3600.0, 'fit RMS finite and under 1 deg')
    check(int(link.num(":GXAn#")) == n, "all %d stars retained after close-out" % n)


def run_narrow_rigid(link):
    print("\n-- narrow rigid session keeps T when no cone --")
    link.ch(":AB#")
    check(link.ch(":A0,r4#") == "1", ":A0,r4# accepted")
    for i in range(1, 5):
        nudge(link, "Mn", 1.5, "Me" if i % 2 else "Mw", 1.0)
        if not point_at_self(link):
            check(False, "set target for narrow star %d" % i)
            return
        check(link.ch(":A%d#" % i) == "1", ":A%d# accepted (narrow)" % i)
    mask4 = link.txt(":GXAf#")
    rms4 = link.num(":GXAr#")
    print('  terms %s  rms %.3f"' % (mask4, rms4))
    check("C" not in mask4, "narrow set does not claim cone")
    check(rms4 == rms4 and rms4 < 60.0,
          'narrow rigid close-out keeps a tight multi-star RMS (not two-star wipe)')
    check(int(link.num(":GXAn#")) == 4, "four stars retained after narrow close-out")


def run_classic_twostar_closeout(link):
    """Classic :A0,2# must still sync the closing star."""
    print("\n-- classic 2-star close-out (last star +30' Dec catalog error) --")
    link.ch(":AB#")
    check(link.ch(":A0,2#") == "1", ":A0,2# accepted")

    # Star 1: self-consistent seed.
    nudge(link, *NUDGES[0])
    if not point_at_self(link):
        check(False, "set target for classic star 1")
        return
    check(link.ch(":A1#") == "1", ":A1# accepted (classic)")

    # Star 2: Dec catalog offset (uniform sky angle — RA near the pole is tiny).
    nudge(link, *NUDGES[1])
    declared = point_with_dec_offset(link, arcmin=30)
    if declared is None:
        check(False, "set target for classic star 2")
        return
    dec_phys, dec_decl = declared
    before = link.steps()
    dec_before = link.txt(":GD#")
    print("       steps before :A2# = %s" % (before,))
    print("       GD before    = %s" % dec_before)
    check(link.ch(":A2#") == "1", ":A2# accepted (classic close-out)")
    after = link.steps()
    dec_after = link.txt(":GD#")
    print("       steps after  :A2# = %s" % (after,))
    print("       GD after     = %s" % dec_after)

    jump = step_jump(before, after)
    check(jump is not None, "read axis step counts around classic close-out")
    check(jump is not None and jump >= SYNC_STEP_FLOOR,
          "motor steps jump after classic close-out (sync happened, jump=%s)"
          % jump)
    # After sync, reported Dec follows the declared catalog star.
    check(dec_delta_arcmin(dec_after, dec_decl) < 2.0,
          "GD matches declared catalog Dec after classic sync")
    check(dec_delta_arcmin(dec_after, dec_before) > 15.0,
          "GD moved away from pre-close physical Dec")
    check(dec_delta_arcmin(dec_phys, dec_before) < 1.0,
          "pre-close GD matched physical pointing")


def main():
    sock, proc = connect()
    if sock is None:
        return 2
    link = Link(sock)
    try:
        print("firmware: %s" % link.txt(":GVN#"))
        link.ch(":hR#")
        link.ch(":Te#")
        link.ch(":AB#")

        run_rigid_closeout(link)
        run_narrow_rigid(link)
        run_classic_twostar_closeout(link)
    finally:
        sock.close()
        if proc:
            proc.kill()

    print(("\n%d check(s) failed" % len(failures)) if failures else "\nall checks passed")
    for f in failures:
        print("  - %s" % f)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
