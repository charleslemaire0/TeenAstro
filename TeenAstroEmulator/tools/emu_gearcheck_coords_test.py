#!/usr/bin/env python3
"""
Gear Check against mainunit_emu.

The hand controller no longer slews to stars. It sends :MG1,90# / :MG1,180#
/ :MG1,360# or the same with axis 2. This test checks that command on a GEM
and on Alt/Az: only the chosen axis moves, by the commanded angle, and a
360° turn is not cut off at the soft limit. :GXP replies are parsed the way
the hand controller parses them.

Usage:
  python -u emu_gearcheck_coords_test.py
"""
from __future__ import print_function

import math
import sys

import emu_moveaxis_tracking_test as emu


def log(msg):
    emu.log(msg)


def shc_declination_dms(text):
    """dmsToDouble(..., sign_present=True, highPrecision=True). ±90, length 9."""
    s = text.strip().rstrip("#")
    if len(s) != 9 or s[0] not in "+-":
        return None
    if s[3] not in ":*" or s[6] not in ":\'":
        return None
    try:
        deg = int(s[1:3])
        minutes = int(s[4:6])
        seconds = int(s[7:9])
    except ValueError:
        return None
    if deg > 90 or minutes > 59 or seconds > 59:
        return None
    sign = -1.0 if s[0] == "-" else 1.0
    return sign * (deg + minutes / 60.0 + seconds / 3600.0)


def shc_axis_dms(text):
    """dmsFullRangeToDouble — the parser Gear Check uses for :GXP."""
    s = text.strip()
    i = 0
    while i < len(s) and s[i] == " ":
        i += 1
    if i >= len(s) or s[i] not in "+-":
        return None
    sign = -1.0 if s[i] == "-" else 1.0
    i += 1
    if i >= len(s) or not s[i].isdigit():
        return None
    deg = 0
    n = 0
    while i < len(s) and s[i].isdigit():
        deg = deg * 10 + int(s[i])
        i += 1
        n += 1
        if n > 9:
            return None
    if i >= len(s) or s[i] not in ":*\xb0":
        return None
    i += 1
    if i + 1 >= len(s) or not s[i].isdigit() or not s[i + 1].isdigit():
        return None
    minutes = int(s[i:i + 2])
    i += 2
    seconds = 0
    if i < len(s) and s[i] in ":\'":
        i += 1
        if i + 1 >= len(s) or not s[i].isdigit() or not s[i + 1].isdigit():
            return None
        seconds = int(s[i:i + 2])
        i += 2
    if i < len(s) and s[i] == "#":
        i += 1
    while i < len(s) and s[i] == " ":
        i += 1
    if i != len(s):
        return None
    if minutes > 59 or seconds > 59:
        return None
    return sign * (deg + minutes / 60.0 + seconds / 3600.0)


def wrap180(deg):
    while deg > 180.0:
        deg -= 360.0
    while deg < -180.0:
        deg += 360.0
    return deg


def measured_gear(gear_cfg, cmd_deg, miss_deg):
    """Same rule as computeMeasuredGear in Actions_GearCheck.cpp."""
    true_deg = cmd_deg + miss_deg
    if abs(cmd_deg) < 5.0 or abs(true_deg) < 1e-3:
        return None
    gear = gear_cfg * true_deg / cmd_deg
    if not math.isfinite(gear) or gear <= 0.0 or gear > 100000.0:
        return None
    return gear


def self_check_parser():
    samples = (
        ("+180*30:00", 180.5),
        ("-045*12:30", -(45.0 + 12.0 / 60.0 + 30.0 / 3600.0)),
        ("+005*00:00", 5.0),
        ("+1000*15:00", 1000.25),
    )
    for text, want in samples:
        got = shc_axis_dms(text)
        if got is None or abs(got - want) > 1e-6:
            log("PARSER SELF-CHECK FAIL %s -> %s" % (text, got))
            return False
        if shc_declination_dms(text) is not None:
            log("PARSER SELF-CHECK FAIL declination accepted %s" % text)
            return False
    log("parser self-check ok")
    return True


def read_gxp(sock, label):
    raw1 = emu.send_recv(sock, ":GXP1#", wait=0.08).strip()
    raw2 = emu.send_recv(sock, ":GXP2#", wait=0.08).strip()
    a1 = shc_axis_dms(raw1)
    a2 = shc_axis_dms(raw2)
    log("  %s GXP1 %r -> %s   GXP2 %r -> %s" % (
        label, raw1, a1, raw2, a2))
    old1 = shc_declination_dms(raw1)
    old2 = shc_declination_dms(raw2)
    return raw1, raw2, a1, a2, old1, old2


def read_gear(sock, axis_cmd):
    raw = emu.send_recv(sock, axis_cmd, wait=0.08).strip().rstrip("#")
    try:
        return int(raw) / 1000.0
    except ValueError:
        return None


def wait_slew_done(sock, timeout_s):
    t0 = emu.time.time()
    while emu.time.time() - t0 < timeout_s:
        gs = emu.read_gxas(sock)
        if gs and not gs["slewing"]:
            emu.time.sleep(0.4)
            return True
        emu.time.sleep(0.2)
    return False


def cmd_code(reply):
    reply = reply.strip()
    return reply[0] if reply else "?"


def prepare(sock):
    emu.setup_site(sock)
    rate = emu.send_recv(sock, ":SXRX,30000#", wait=0.1).strip()
    live = emu.send_recv(sock, ":GXR4#", wait=0.08).strip()
    log("  max rate set %r live %r" % (rate, live))
    emu.send_recv(sock, ":Td#", wait=0.05)


def slew_timeout(sock, degrees):
    live = emu.send_recv(sock, ":GXR4#", wait=0.08).strip().rstrip("#")
    try:
        rate = float(live)
    except ValueError:
        rate = 600.0
    if rate < 1.0:
        rate = 600.0
    return degrees / (rate / 3600.0) * 1.8 + 20.0


def rotate(sock, axis, degrees):
    """Start :MGn,ddd# and wait until the axis has moved by about that angle."""
    _, _, a1, a2, old1, old2 = read_gxp(sock, "before MG%d,%d" % (axis, degrees))
    if a1 is None or a2 is None:
        log("FAIL cannot read axes before the turn")
        return False
    if old1 is not None or old2 is not None:
        log("FAIL declination parser accepted a :GXP reply")
        return False
    reply = cmd_code(emu.send_recv(sock, ":MG%d,%d#" % (axis, degrees), wait=0.15))
    log("  :MG%d,%d# -> %s" % (axis, degrees, reply))
    if reply != "0":
        log("FAIL gear-check turn was refused")
        return False
    gs = emu.read_gxas(sock)
    if not gs or not gs["slewing"]:
        log("FAIL turn did not start slewing")
        return False
    if not wait_slew_done(sock, slew_timeout(sock, degrees)):
        log("FAIL turn did not finish")
        emu.send_recv(sock, ":Q#", wait=0.05)
        return False
    _, _, b1, b2, _, _ = read_gxp(sock, "after MG%d,%d" % (axis, degrees))
    if b1 is None or b2 is None:
        log("FAIL cannot read axes after the turn")
        return False
    d1 = b1 - a1
    d2 = b2 - a2
    moved = d1 if axis == 1 else d2
    other = d2 if axis == 1 else d1
    log("  dA1 %+.3f  dA2 %+.3f" % (d1, d2))
    if abs(moved - degrees) > 1.5 or abs(other) > 1.5:
        log("FAIL axis %d did not turn by %d degrees alone" % (axis, degrees))
        return False
    gear = read_gear(sock, ":GXMGR#" if axis == 1 else ":GXMGD#")
    meas = measured_gear(gear, moved, 0.0) if gear else None
    if meas is None or abs(meas - gear) > 0.05:
        log("FAIL measured gear")
        return False
    gs = emu.read_gxas(sock)
    if gs and gs["tracking_on"]:
        log("FAIL sidereal tracking restarted after the turn")
        return False
    log("PASS axis %d  %d deg  gear %.2f" % (axis, degrees, gear))
    return True


def axis_quiet(sock, n):
    raw = emu.send_recv(sock, ":GXP%d#" % n, wait=0.05).strip()
    return shc_axis_dms(raw)


def run_stop(sock):
    """:Q# during a gear-check turn must halt that axis."""
    a1 = axis_quiet(sock, 1)
    a2 = axis_quiet(sock, 2)
    reply = cmd_code(emu.send_recv(sock, ":MG1,360#", wait=0.1))
    log("  stop check :MG1,360# -> %s" % reply)
    if reply != "0" or a1 is None or a2 is None:
        log("FAIL stop check could not start")
        return False
    t0 = emu.time.time()
    started = False
    while emu.time.time() - t0 < 15.0:
        gs = emu.read_gxas(sock)
        b1 = axis_quiet(sock, 1)
        if gs and gs["slewing"] and b1 is not None and (b1 - a1) > 3.0:
            started = True
            break
        emu.time.sleep(0.15)
    if not started:
        log("FAIL stop check: turn did not start")
        emu.send_recv(sock, ":Q#", wait=0.05)
        return False
    emu.send_recv(sock, ":Q#", wait=0.05)
    if not wait_slew_done(sock, 20.0):
        log("FAIL stop check: still slewing after :Q#")
        return False
    c1 = axis_quiet(sock, 1)
    c2 = axis_quiet(sock, 2)
    if c1 is None or c2 is None:
        log("FAIL stop check: cannot read axes")
        return False
    moved = c1 - a1
    log("  stopped after %+.2f deg" % moved)
    if moved < 2.0 or moved > 90.0:
        log("FAIL stop check: axis did not halt (moved %+.1f)" % moved)
        return False
    if abs(c2 - a2) > 1.5:
        log("FAIL stop check: the other axis moved")
        return False
    emu.time.sleep(1.2)
    d1 = axis_quiet(sock, 1)
    if d1 is None or abs(d1 - c1) > 0.5:
        log("FAIL stop check: axis kept turning after :Q#")
        return False
    log("PASS stop  axis halted at %+.1f deg" % moved)
    return True


def run_mount(sock, title):
    log("")
    log("======== %s ========" % title)
    prepare(sock)
    if not run_stop(sock):
        return False
    bad = cmd_code(emu.send_recv(sock, ":MG1,45#", wait=0.1))
    log("  :MG1,45# -> %s (want not 0)" % bad)
    if bad == "0":
        log("FAIL a 45 degree turn was accepted")
        emu.send_recv(sock, ":Q#", wait=0.05)
        return False
    other = cmd_code(emu.send_recv(sock, ":MG3,90#", wait=0.1))
    log("  :MG3,90# -> %s (want not 0)" % other)
    if other == "0":
        log("FAIL axis 3 was accepted")
        emu.send_recv(sock, ":Q#", wait=0.05)
        return False
    if not rotate(sock, 1, 90):
        return False
    if not rotate(sock, 2, 90):
        return False

    # Axis 1 is about +90 and its soft limit is +180, so 360° has to cross
    # that limit and the horizon. The turn is finished only at +360°.
    _, _, a1, a2, _, _ = read_gxp(sock, "before 360")
    reply = cmd_code(emu.send_recv(sock, ":MG1,360#", wait=0.15))
    log("  :MG1,360# -> %s" % reply)
    if reply != "0" or a1 is None:
        log("FAIL 360 degree turn was refused")
        return False
    if not wait_slew_done(sock, min(slew_timeout(sock, 360), 240.0)):
        log("FAIL 360 degree turn did not finish")
        emu.send_recv(sock, ":Q#", wait=0.05)
        return False
    _, _, late1, late2, _, _ = read_gxp(sock, "after 360")
    if late1 is None or late2 is None or a2 is None:
        log("FAIL cannot read axes after the 360 degree turn")
        return False
    moved = late1 - a1
    log("  360 moved %+.3f  other %+.3f" % (moved, late2 - a2))
    if abs(moved - 360.0) > 1.5:
        log("FAIL 360 degree turn was cut short at %+.1f" % moved)
        return False
    if abs(late2 - a2) > 1.5:
        log("FAIL axis 2 moved during the axis 1 turn")
        return False
    log("PASS 360 deg crossed the soft limit")
    return True


def main():
    if not self_check_parser():
        return 2
    exe, emu_root = emu.find_emu_exe()
    if not exe:
        log("ERROR: mainunit_emu.exe not found. Build: pio run -d TeenAstroEmulator -e emu_mainunit")
        return 2
    log("Using emu: %s" % exe)
    proc = emu.EmuProcess(exe, emu_root)
    proc.start()
    ok_gem = False
    ok_aa = False
    try:
        sock = emu.connect("127.0.0.1", 9997)
        log("set mount type GEM")
        sock = emu.set_mount_type(sock, 1, proc, "127.0.0.1", 9997)
        ok_gem = run_mount(sock, "GEM")
        log("set mount type AltAz")
        sock = emu.set_mount_type(sock, 3, proc, "127.0.0.1", 9997)
        ok_aa = run_mount(sock, "AltAz")
        try:
            sock.close()
        except Exception:
            pass
    finally:
        proc.stop()
    log("")
    log("======== SUMMARY  GEM %s   AltAz %s ========" % (
        "PASS" if ok_gem else "FAIL", "PASS" if ok_aa else "FAIL"))
    return 0 if ok_gem and ok_aa else 1


if __name__ == "__main__":
    sys.exit(main())
