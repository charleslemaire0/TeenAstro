#!/usr/bin/env python3
"""End-to-end check of the rigid alignment session against the MainUnit emulator.

Drives the real firmware over the LX200 socket (TCP 9997) the way the SHC or an
app would, covering the parts unit tests cannot reach: the :A0,r<n># session
selector, the :A1#..:A9# accumulate-then-fit path in Command_A, and the
:GXA{c,p,i,r,n}# / :SXA{c,p,i}# / :SXAC# command surface.

Stars are fed by nudging the axes with :Mn#/:Me# and then declaring the mount's
own reported RA/Dec as the star position. That makes the star set exactly self
consistent, so a correct fit must converge on near zero head terms and a near
zero residual: it proves the firmware path runs and does not invent geometry.
How accurately the fit recovers *injected* errors is covered by the unit tests
in tests/test/test_rigid_model.

Gotos are deliberately not used: a freshly initialised emulator EEPROM has no
motor or limit provisioning, so :MS# answers "below horizon" for every target.

Usage:
    pio run -d TeenAstroEmulator -e emu_mainunit
    python tests/rigid_align_emu.py
"""

import socket
import subprocess
import sys
import time
from pathlib import Path

HOST = "127.0.0.1"
PORT = 9997
EMU = Path(__file__).resolve().parents[1] / "TeenAstroEmulator" / ".pio" / "build" / "emu" / "mainunit_emu.exe"

# Per star axis nudges as (axis2 command, seconds, axis1 command, seconds).
# Spread over both axes so the six parameters are not degenerate.
NUDGES = [
    ("Mn", 9.0, "Me", 4.0),
    ("Mn", 9.0, "Mw", 7.0),
    ("Mn", 9.0, "Me", 6.0),
    ("Ms", 7.0, "Mw", 3.0),
    ("Ms", 9.0, "Me", 7.0),
]

failures = []


def check(cond, what):
    print(("  ok   " if cond else "  FAIL ") + what)
    if not cond:
        failures.append(what)


class Link:
    """LX200 link. Replies are a single char, '#' terminated, or absent, and
    only the command tells you which, so each call states what it expects."""

    def __init__(self, sock):
        self.sock = sock

    def ch(self, cmd):
        self.sock.sendall(cmd.encode())
        self.sock.settimeout(3.0)
        try:
            return self.sock.recv(1).decode(errors="replace")
        except socket.timeout:
            return ""

    def txt(self, cmd):
        self.sock.sendall(cmd.encode())
        self.sock.settimeout(3.0)
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
        """Command expected to be rejected, i.e. to produce no reply at all."""
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


def nudge(link, axis2cmd, t2, axis1cmd, t1):
    link.none(":R4#")
    for cmd, dur in ((axis2cmd, t2), (axis1cmd, t1)):
        link.none(":%s#" % cmd)
        time.sleep(dur)
        link.none(":Q%s#" % cmd[1])
    time.sleep(0.4)


def point_at_self(link):
    """Declare the mount's current RA/Dec as the target, so the star is exactly
    where the mount already believes it is pointing."""
    ra = link.txt(":GR#")
    dec = link.txt(":GD#")
    if not ra or not dec:
        return False
    # Az/Alt is what the fit actually works in, so log it: the altitude span is
    # what decides whether any head term is separable.
    print("       RA %s Dec %s  /  Az %s Alt %s"
          % (ra, dec, link.txt(":GZ#"), link.txt(":GA#")))
    return link.ch(":Sr%s#" % ra) == "1" and link.ch(":Sd%s#" % dec) == "1"


def connect():
    try:
        return socket.create_connection((HOST, PORT), timeout=2.0), None
    except OSError:
        pass
    if not EMU.exists():
        print("emulator not built: %s" % EMU)
        print("run: pio run -d TeenAstroEmulator -e emu_mainunit")
        return None, None
    print("starting emulator ...")
    proc = subprocess.Popen([str(EMU)], cwd=str(EMU.parent))
    for _ in range(40):
        time.sleep(0.5)
        try:
            return socket.create_connection((HOST, PORT), timeout=2.0), proc
        except OSError:
            continue
    proc.kill()
    print("could not connect to emulator")
    return None, None


def main():
    sock, proc = connect()
    if sock is None:
        return 2
    link = Link(sock)
    try:
        print("firmware: %s" % link.txt(":GVN#"))
        link.ch(":hR#")
        link.ch(":Te#")

        print("\n-- command surface --")
        link.ch(":AB#")
        check(link.ch(":SXAc,12.5#") == "1", ":SXAc,12.5# accepted")
        check(link.ch(":SXAp,-7.25#") == "1", ":SXAp,-7.25# accepted")
        check(link.ch(":SXAi,3.0#") == "1", ":SXAi,3.0# accepted")
        check(abs(link.num(":GXAc#") - 12.5) < 0.01, ':GXAc# reads back 12.5"')
        check(abs(link.num(":GXAp#") + 7.25) < 0.01, ':GXAp# reads back -7.25"')
        check(abs(link.num(":GXAi#") - 3.0) < 0.01, ':GXAi# reads back 3.0"')
        # Beyond a few degrees this cannot be a head error and must be refused.
        check(link.ch(":SXAc,36000#") == "0", ":SXAc,36000# rejected (10 deg)")
        check(link.ch(":SXAC#") == "1", ":SXAC# accepted")
        check(abs(link.num(":GXAc#")) < 1e-6, "head cleared by :SXAC#")

        print("\n-- session selector --")
        check(link.none(":A0,r2#") == "", ":A0,r2# rejected (too few stars)")
        check(link.none(":A0,r3#") == "", ":A0,r3# rejected (no redundancy)")
        check(link.none(":A0,rx#") == "", ":A0,rx# rejected (malformed)")
        check(link.none(":A0,r10#") == "", ":A0,r10# rejected (too many stars)")
        check(link.none(":A0,3#") == "", ":A0,3# still rejected")
        check(link.ch(":A0,2#") == "1", ":A0,2# still accepted")
        check(link.ch(":A0,m#") == "1", ":A0,m# still accepted")
        # Outside a rigid session :A3# must stay rejected.
        check(link.ch(":A0,2#") == "1", ":A0,2# restart")
        check(link.none(":A3#") == "", ":A3# rejected outside a rigid session")

        n = len(NUDGES)
        print("\n-- rigid %d star session --" % n)
        check(link.ch(":A0,r%d#" % n) == "1", ":A0,r%d# accepted" % n)
        check(int(link.num(":GXAn#")) == 0, "session starts with 0 stars")

        for i, (a2, t2, a1, t1) in enumerate(NUDGES, start=1):
            nudge(link, a2, t2, a1, t1)
            if not point_at_self(link):
                check(False, "set target for star %d" % i)
                break
            check(link.ch(":A%d#" % i) == "1", ":A%d# accepted" % i)
            check(int(link.num(":GXAn#")) == i, "star count is %d after :A%d#" % (i, i))
            if i < n:
                # The head must stay untouched until the closing star.
                check(abs(link.num(":GXAc#")) < 1e-6, "head still zero after star %d" % i)

        print("\n-- fitted model --")
        cone = link.num(":GXAc#")
        perp = link.num(":GXAp#")
        idx2 = link.num(":GXAi#")
        rms = link.num(":GXAr#")
        mask = link.txt(":GXAf#")
        print('  terms %s  cone %+.3f"  perp %+.3f"  idx2 %+.3f"  rms %.3f"'
              % (mask, cone, perp, idx2, rms))
        # The stars are self consistent by construction, so a correct fit must
        # reproduce the pointing to well under an arcsecond.
        check(rms < 10.0, 'fit RMS under 10"')
        # Nudging the axes only reaches a narrow band of altitude (roughly 29 to
        # 45 degrees here), and the head terms are what a wide altitude spread
        # separates, so in practice this star set separates none of them and the
        # mask comes back "---". That is the point of the check: the firmware
        # must decline terms it cannot measure rather than fit noise into them.
        # Recovery from genuinely well spread stars is covered by the unit tests
        # in tests/test/test_rigid_model.
        #
        # Cone and axis2 non-perpendicularity displace axis1 with almost the same
        # dependence on axis2, so no realistic star set separates both. The
        # fitter must report at most one of the two, never both.
        check(len(mask) == 3, ":GXAf# returns three characters")
        check(not ("C" in mask and "P" in mask),
              "cone and perp never both claimed as separable")
        # This emulated mount is geometrically perfect, so whatever the fit does
        # keep must stay near zero; the star positions come from :GR#/:GD#,
        # quantised to 1 second of time and 1 arcsec, which is the only error
        # the fit has to absorb.
        for term, value, name in (("C", cone, "cone"), ("P", perp, "perp"), ("I", idx2, "axis2 index")):
            if term in mask:
                check(abs(value) < 60.0, '%s under 1 arcmin' % name)
            else:
                check(value == 0.0, '%s held at zero when not separable' % name)

        print("\n-- persistence --")
        check(link.ch(":AW#") == "1", ":AW# saves model")
        saved = (link.num(":GXAc#"), link.num(":GXAp#"), link.num(":GXAi#"))
        check(all(abs(a - b) < 1e-6 for a, b in zip(saved, (cone, perp, idx2))),
              "head unchanged by :AW#")
        check(link.ch(":AB#") == "1", ":AB# aborts alignment")
        check(abs(link.num(":GXAc#")) < 1e-6, ":AB# clears head in RAM")
        check(int(link.num(":GXAn#")) == 0, ":AB# clears retained stars")
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
