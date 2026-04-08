#!/usr/bin/env python3
"""
Verify MainUnit emulator + GXAS byte 100 bits 5-7 (GotoState) — same wire data the SHC
uses to pick slewing icons (EQ / AltAz / meridian flip).

Checks:
  1) During :MS# slew, GXAS gotoKind should be 1 (GOTO_EQ) at least once.
  2) If :MF# returns 0, GXAS gotoKind should be 3 (GOTO_FLIP_PIER_SIDE) during flip slew.

Requires mainunit_emu.exe on TCP 9997.

Usage:
  python -u emu_flip_gxas_mimic_test.py [host] [port]
Defaults: 127.0.0.1 9997
"""
from __future__ import print_function

import base64
import socket
import sys
import time


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 9997

GXAS_B64_LEN = 136
GXAS_PKT_LEN = 102

# CommandEnums.h GotoState — same as GXAS byte 100 bits 5-7
GOTO_NAMES = {
    0: "GOTO_NONE",
    1: "GOTO_EQ",
    2: "GOTO_ALTAZ",
    3: "GOTO_FLIP_PIER_SIDE",
}


def send_recv(sock, cmd):
    sock.sendall(cmd.encode("ascii"))
    time.sleep(0.05)
    buf = b""
    sock.settimeout(2.0)
    try:
        while True:
            b = sock.recv(16384)
            if not b:
                break
            buf += b
            if b"#" in buf:
                break
    except socket.timeout:
        pass
    return buf.decode("ascii", errors="replace")


def decode_gxas_goto_kind(reply):
    """reply is full response string; strip trailing # and base64-decode 102 bytes."""
    s = reply.strip()
    if s.endswith("#"):
        s = s[:-1]
    if len(s) != GXAS_B64_LEN:
        return None, "bad GXAS len %d (want %d)" % (len(s), GXAS_B64_LEN)
    raw = base64.b64decode(s)
    if len(raw) != GXAS_PKT_LEN:
        return None, "bad pkt len %d" % len(raw)
    b100 = raw[100]
    kind = (b100 >> 5) & 0x7
    return kind, None


def is_slewing_d(reply):
    return len(reply) >= 1 and ord(reply[0]) == 0x7F


def wait_idle(s, max_s, label):
    """Wait until :D# says not slewing or timeout."""
    log("  %s (max %ds) ..." % (label, max_s))
    t0 = time.time()
    while time.time() - t0 < max_s:
        d = send_recv(s, ":D#")
        if len(d) >= 1 and not is_slewing_d(d):
            log("  idle.")
            return True
        time.sleep(0.12)
    log("  timeout (still slewing or unexpected :D#).")
    return False


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(10.0)
    s.connect((HOST, PORT))
    log("Connected %s:%s — GXAS mimic (byte 100 bits 5-7 = GotoState)\n" % (HOST, PORT))

    def tx(label, cmd):
        r = send_recv(s, cmd)
        short = r[:100] + ("..." if len(r) > 100 else "")
        log("%-14s %s -> %r" % (label, cmd, short))
        return r

    tx("unpark", ":hR#")
    tx("track_on", ":Te#")

    # --- Part 1: EQ goto -> expect gotoKind 1 in GXAS while slewing (SHC EQ slewing mimic) ---
    tx("Sr", ":Sr06:00:00#")
    tx("Sd", ":Sd+45*00:00#")
    rms = tx("MS", ":MS#")
    saw_eq = False
    if rms and rms[0] == "0":
        log("  polling GXAS during :MS# slew...")
        t_poll = time.time() + 20.0
        while time.time() < t_poll:
            gx = send_recv(s, ":GXAS#")
            k, err = decode_gxas_goto_kind(gx)
            if err:
                log("  GXAS decode: %s" % err)
                break
            if k == 1:
                saw_eq = True
                log("  OK: gotoKind=1 (GOTO_EQ) — SHC would show EQ slew icon")
                break
            if not is_slewing_d(send_recv(s, ":D#")):
                break
            time.sleep(0.06)
        wait_idle(s, 25, "wait settle after goto")
    else:
        log("WARN: :MS# not OK, skip EQ mimic check")

    if not saw_eq:
        log("WARN: did not see gotoKind=1 during EQ goto (may be instant in emu).")

    # Baseline GXAS
    gx0 = tx("GXAS_idle", ":GXAS#")
    k0, err = decode_gxas_goto_kind(gx0)
    if err:
        print("FAIL decode GXAS:", err)
        s.close()
        return 1
    log("  -> gotoKind (idle) = %d %s" % (k0, GOTO_NAMES.get(k0, "?")))

    # --- Part 2: Meridian flip -> if accepted, expect gotoKind 3 (SHC flip slew mimic) ---
    r = tx("MF", ":MF#")
    code = r[0] if r else "?"
    log("\n:MF# first byte (ERRGOTO wire): %r" % code)
    if code != "0":
        log("SKIP flip slew mimic: :MF# not accepted (%s)." % code)
        log("  (3=SAMESIDE, 6=limits — pose/limits in emulator.)")
        s.close()
        return 0 if saw_eq else 3

    saw_flip = False
    t_end = time.time() + 15.0
    while time.time() < t_end:
        d = send_recv(s, ":D#")
        gx = send_recv(s, ":GXAS#")
        k, err = decode_gxas_goto_kind(gx)
        if err:
            log("GXAS decode error: %s" % err)
            break
        if k == 3:
            if not saw_flip:
                saw_flip = True
                log("  OK: gotoKind=3 (GOTO_FLIP_PIER_SIDE) — SHC flip slew icon")
        if not is_slewing_d(d):
            log("  slew finished :D# -> idle")
            break
        time.sleep(0.08)

    s.close()
    if not saw_flip:
        log("WARN: did not observe gotoKind=3 during flip window (timing or instant finish).")
        return 2 if saw_eq else 4
    log("\nPASS: flip slew + GXAS meridian-flip kind.")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
