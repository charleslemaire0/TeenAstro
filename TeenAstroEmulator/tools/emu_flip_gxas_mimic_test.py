#!/usr/bin/env python3
"""
Verify MainUnit (emulator TCP or Teensy serial) + GXAS byte 100 bits 5-7 (GotoState).

Checks:
  0) Optional: with --set-meridian-goto-limits, write meridian GOTO E/W to EEPROM via :SXLE,60# /
     :SXLW,60# (MERIDIAN_GOTO_MINUTES_EW). Default is off — uses whatever limits are already in
     GXCS (safe on real hardware).
  1) GEM: after unpark/track, read :GXAS# + :GXCS# and choose Sr/Sd so the slew targets the
     middle of the meridian overlap band (HA vs LST). Non-GEM or parse failure: Sr 06:00 / Sd +45 deg.
  2) After :MS#, poll :GXAS# until gotoKind is GOTO_NONE (0); during the slew, gotoKind should
     be 1 (GOTO_EQ) at least once. Same idea after :MF# until idle (expect gotoKind 3 during flip).
  3) If :MS# succeeds, GXAS confirms target HA in the configured overlap band.
  4) If :MF# returns 0, GXAS gotoKind should be 3 (GOTO_FLIP_PIER_SIDE) during flip slew.

Usage:
  python -u emu_flip_gxas_mimic_test.py [--set-meridian-goto-limits] [host] [port]
  python -u emu_flip_gxas_mimic_test.py [--set-meridian-goto-limits] --serial COM3 [baud]

  --set-meridian-goto-limits  Write :SXLE/:SXLW to EEPROM (omit on hardware unless you intend to).
"""
from __future__ import print_function

import base64
import math
import socket
import struct
import sys
import time

try:
    import serial as pyserial
except ImportError:
    pyserial = None


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass


GXAS_B64_LEN = 136
GXAS_PKT_LEN = 102
GXCS_B64_LEN = 120
GXCS_PKT_LEN = 90

# GEM meridian GOTO limits set via :SXLE, / :SXLW, (minutes). HA span in deg ~ minutes/4.
# 60 min E/W => ~15 deg per side => ~30 deg overlap width in HA (-15..+15).
MERIDIAN_GOTO_MINUTES_EW = 60

# Safety cap while polling :GXAS# until byte 100 gotoKind returns to GOTO_NONE (0).
GOTO_WAIT_MAX_S = 180

GOTO_NAMES = {
    0: "GOTO_NONE",
    1: "GOTO_EQ",
    2: "GOTO_ALTAZ",
    3: "GOTO_FLIP_PIER_SIDE",
}

# :MS# / :MF# reply[0] = chr(ord('0') + ErrorsGoTo). Examples: '6'=LIMITS, 'A'=ERRGOTO_MERIDIAN (17).
# Meridian in sky overlap does not prevent 'A' if goTo() was blocked by stale ERRT_MERIDIAN
# from safetyCheck while tracking (fixed in firmware: clear meridian/pole before goTo).


def send_recv_tcp(sock, cmd):
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


def send_recv_serial(ser, cmd):
    ser.reset_input_buffer()
    ser.write(cmd.encode("ascii"))
    ser.flush()
    time.sleep(0.08)
    end = time.time() + 5.0
    buf = b""
    # Longer per-read timeout so Windows/USB serial returns full GXAS/GXCS lines reliably.
    ser.timeout = 0.25
    while time.time() < end:
        chunk = ser.read(16384)
        if chunk:
            buf += chunk
            if b"#" in buf:
                break
        else:
            time.sleep(0.02)
    return buf.decode("ascii", errors="replace")


def _strip_hash_b64(reply, want_len, name="b64"):
    s = reply.strip()
    if s.endswith("#"):
        s = s[:-1]
    if len(s) != want_len:
        return None, "bad %s len %d (want %d)" % (name, len(s), want_len)
    raw = base64.b64decode(s)
    return raw, None


def ha_range_deg(d):
    """Match TeenAstroMath::haRange (C++ remainder(d, 360))."""
    return math.remainder(d, 360.0)


def gxas_mount_type(raw):
    """Byte 1 bits 4-6: 0=undef, 1=GEM, 2=FORK, 3=ALTAZM, 4=FORK_ALT (Command_GX.cpp)."""
    return (raw[1] >> 4) & 0x7


def gxas_target_ha_deg(raw):
    """HA deg = haRange(LST*15 - targetRA_deg); GXAS stores tgtRA hours = newTargetRA/15."""
    lst_h = struct.unpack_from("<d", raw, 44)[0]
    tgt_ra_h = struct.unpack_from("<d", raw, 52)[0]
    return ha_range_deg(lst_h * 15.0 - tgt_ra_h * 15.0)


def gxcs_meridian_minutes_le(raw):
    """GXCS bytes 56-59: meridian E/W (int16 LE), arcminutes for GOTO (MountLimits)."""
    e = raw[56] | (raw[57] << 8)
    w = raw[58] | (raw[59] << 8)
    if e >= 0x8000:
        e -= 0x10000
    if w >= 0x8000:
        w -= 0x10000
    return e, w


def meridian_minutes_to_ha_deg(m):
    """(minutes/60)*15 = minutes/4 - same scaling as MountLimits + checkMeridian."""
    return m / 4.0


def gxas_lst_hours(raw):
    return struct.unpack_from("<d", raw, 44)[0]


def format_sr_cmd(ra_h):
    """LX200 :SrHH:MM:SS# with ra_h in [0,24)."""
    ra_h = ra_h % 24.0
    total_sec = int(round(ra_h * 3600.0)) % (24 * 3600)
    hh = total_sec // 3600
    mm = (total_sec % 3600) // 60
    ss = total_sec % 60
    return ":Sr%02d:%02d:%02d#" % (hh, mm, ss)


def build_sr_sd_meridian_overlap(gxas_reply, gxcs_reply, dec_sd=":Sd+45*00:00#"):
    """
    For GEM, set target RA so HA = midpoint of [-E,+W] (degrees) for current LST.
    Returns (sr_cmd, sd_cmd, info_str) or (None, None, reason) to fall back to fixed coords.
    """
    raw_a, err = _strip_hash_b64(gxas_reply, GXAS_B64_LEN, "GXAS")
    if err:
        return None, None, err
    raw_c, err = _strip_hash_b64(gxcs_reply, GXCS_B64_LEN, "GXCS")
    if err:
        return None, None, err
    if gxas_mount_type(raw_a) != 1:
        return None, None, "not GEM"
    e_min, w_min = gxcs_meridian_minutes_le(raw_c)
    e_deg = meridian_minutes_to_ha_deg(abs(e_min))
    w_deg = meridian_minutes_to_ha_deg(abs(w_min))
    lo = -e_deg
    hi = w_deg
    if lo > hi:
        return None, None, "empty overlap (E/W limits)"
    desired_ha_deg = 0.5 * (lo + hi)
    lst_h = gxas_lst_hours(raw_a)
    lst_deg = lst_h * 15.0
    ra_deg = ha_range_deg(lst_deg - desired_ha_deg)
    ra_h = (ra_deg / 15.0) % 24.0
    sr = format_sr_cmd(ra_h)
    info = (
        "GEM overlap aim: HA target=%.2f deg (band [%.2f, %.2f] deg), LST=%.4fh -> %s"
        % (desired_ha_deg, lo, hi, lst_h, sr)
    )
    return sr, dec_sd, info


def decode_gxas_goto_kind(reply):
    raw, err = _strip_hash_b64(reply, GXAS_B64_LEN, "GXAS")
    if err:
        return None, err
    if len(raw) != GXAS_PKT_LEN:
        return None, "bad pkt len %d" % len(raw)
    b100 = raw[100]
    kind = (b100 >> 5) & 0x7
    return kind, None


def verify_meridian_overlap(gxas_reply, gxcs_reply):
    """
    True if target HA lies in [-E_deg, +W_deg] (overlapping pier / meridian window).
    Returns (ok, message).
    """
    raw_a, err = _strip_hash_b64(gxas_reply, GXAS_B64_LEN, "GXAS")
    if err:
        return False, err
    if len(raw_a) != GXAS_PKT_LEN:
        return False, "GXAS bad pkt len %d" % len(raw_a)
    raw_c, err = _strip_hash_b64(gxcs_reply, GXCS_B64_LEN, "GXCS")
    if err:
        return False, err
    if len(raw_c) != GXCS_PKT_LEN:
        return False, "GXCS bad pkt len %d" % len(raw_c)
    mt = gxas_mount_type(raw_a)
    if mt != 1:
        return True, "skip overlap (mountType=%d, not GEM)" % mt
    ha_deg = gxas_target_ha_deg(raw_a)
    e_min, w_min = gxcs_meridian_minutes_le(raw_c)
    e_deg = meridian_minutes_to_ha_deg(abs(e_min))
    w_deg = meridian_minutes_to_ha_deg(abs(w_min))
    lo = -e_deg
    hi = w_deg
    if lo <= ha_deg <= hi:
        return True, (
            "OK: HA=%.2f deg in overlap [%.2f, %.2f] (E=%d W=%d min)"
            % (ha_deg, lo, hi, e_min, w_min)
        )
    return False, (
        "FAIL: HA=%.2f deg outside meridian overlap [%.2f, %.2f] (E=%d W=%d min)"
        % (ha_deg, lo, hi, e_min, w_min)
    )


def parse_args():
    argv = list(sys.argv[1:])
    if "--help" in argv or "-h" in argv:
        print(__doc__)
        sys.exit(0)
    set_meridian_limits = "--set-meridian-goto-limits" in argv
    argv = [a for a in argv if a != "--set-meridian-goto-limits"]
    if argv and argv[0] == "--serial":
        if len(argv) < 2:
            log("ERROR: need port, e.g. --serial COM3")
            sys.exit(2)
        if pyserial is None:
            log("ERROR: pip install pyserial")
            sys.exit(2)
        port = argv[1]
        baud = int(argv[2]) if len(argv) > 2 else 115200
        return "serial", (port, baud), set_meridian_limits
    host = argv[0] if len(argv) > 0 else "127.0.0.1"
    port = int(argv[1]) if len(argv) > 1 else 9997
    return "tcp", (host, port), set_meridian_limits


def main():
    mode, conn, set_meridian_limits = parse_args()

    if mode == "tcp":
        host, port = conn
        tr = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tr.settimeout(10.0)
        tr.connect((host, port))
        send_recv = send_recv_tcp
        log("Connected %s:%s - GXAS mimic (byte 100 bits 5-7 = GotoState)\n" % (host, port))
    else:
        port_name, baud = conn
        tr = pyserial.Serial(port_name, baud, bytesize=8, parity="N", stopbits=1, timeout=1)
        send_recv = send_recv_serial
        log("Serial %s at %d - GXAS mimic\n" % (port_name, baud))

    def tx(label, cmd):
        r = send_recv(tr, cmd)
        short = r[:100] + ("..." if len(r) > 100 else "")
        log("%-14s %s -> %r" % (label, cmd, short))
        return r

    tx("unpark", ":hR#")
    if set_meridian_limits:
        tx("SXLE", ":SXLE,%d#" % MERIDIAN_GOTO_MINUTES_EW)
        tx("SXLW", ":SXLW,%d#" % MERIDIAN_GOTO_MINUTES_EW)
    else:
        log(
            "skip :SXLE/:SXLW (existing EEPROM meridian GOTO limits; "
            "--set-meridian-goto-limits to write %d min E/W)"
            % MERIDIAN_GOTO_MINUTES_EW
        )

    tx("track_on", ":Te#")

    gx_pre = send_recv(tr, ":GXAS#")
    gxcs_pre = send_recv(tr, ":GXCS#")
    sr_cmd, sd_cmd, aim = build_sr_sd_meridian_overlap(gx_pre, gxcs_pre)
    if sr_cmd is None:
        log("  target: %s (using fixed Sr/Sd)" % aim)
        sr_cmd = ":Sr06:00:00#"
        sd_cmd = ":Sd+45*00:00#"
    else:
        log("  target: %s" % aim)

    tx("Sr", sr_cmd)
    tx("Sd", sd_cmd)
    rms = tx("MS", ":MS#")
    ms_ok = bool(rms and rms[0] == "0")
    if rms and not ms_ok:
        c = rms[0]
        extra = ""
        if c == "A":
            extra = " (ERRGOTO_MERIDIAN: tracking had set meridian error before slew; rebuild firmware if this persists)"
        log("  NOTE: :MS# not 0 first byte %r%s" % (c, extra))
    saw_eq = False
    if ms_ok:
        log("  wait for EQ goto to finish (GXAS byte100 gotoKind -> GOTO_NONE) ...")
        t0 = time.time()
        while time.time() - t0 < GOTO_WAIT_MAX_S:
            gx = send_recv(tr, ":GXAS#")
            k, err = decode_gxas_goto_kind(gx)
            if err:
                time.sleep(0.1)
                continue
            if k == 1 and not saw_eq:
                saw_eq = True
                log("  OK: gotoKind=1 (GOTO_EQ) - SHC would show EQ slew icon")
            if k == 0:
                log("  GXAS: goto idle (GOTO_NONE) - slew/settle done.")
                break
            time.sleep(0.08)
        else:
            log("  WARN: timeout (%ds) waiting for GXAS gotoKind=0 after :MS#." % GOTO_WAIT_MAX_S)
    else:
        log("WARN: :MS# not OK, skip EQ mimic check")

    if not saw_eq:
        log("WARN: did not see gotoKind=1 during EQ goto (may be instant).")

    gx0 = tx("GXAS_idle", ":GXAS#")
    k0, err = decode_gxas_goto_kind(gx0)
    if err:
        print("FAIL decode GXAS:", err)
        tr.close()
        return 1
    log("  -> gotoKind (idle) = %d %s" % (k0, GOTO_NAMES.get(k0, "?")))

    if ms_ok:
        gxcs = tx("GXCS", ":GXCS#")
        ok_overlap, overlap_msg = verify_meridian_overlap(gx0, gxcs)
        log("  meridian overlap: %s" % overlap_msg)
        if not ok_overlap:
            print(overlap_msg)
            tr.close()
            return 1
    else:
        log("  meridian overlap: skip (:MS# did not succeed - GXAS target/LST not reliable)")

    r = tx("MF", ":MF#")
    code = r[0] if r else "?"
    log("\n:MF# first byte (ERRGOTO wire): %r" % code)
    if code != "0":
        log("SKIP flip slew mimic: :MF# not accepted (%s)." % code)
        log("  (3=SAMESIDE, 6=limits.)")
        tr.close()
        return 0 if saw_eq else 3

    saw_flip = False
    log("  wait for meridian flip to finish (GXAS gotoKind 3 -> 0) ...")
    t0 = time.time()
    while time.time() - t0 < GOTO_WAIT_MAX_S:
        gx = send_recv(tr, ":GXAS#")
        k, err = decode_gxas_goto_kind(gx)
        if err:
            time.sleep(0.1)
            continue
        if k == 3 and not saw_flip:
            saw_flip = True
            log("  OK: gotoKind=3 (GOTO_FLIP_PIER_SIDE) - SHC flip slew icon")
        if k == 0:
            log("  GXAS: goto idle after flip (GOTO_NONE).")
            break
        time.sleep(0.08)
    else:
        log("  WARN: timeout (%ds) waiting for GXAS gotoKind=0 after :MF#." % GOTO_WAIT_MAX_S)

    tr.close()
    if not saw_flip:
        log("WARN: did not observe gotoKind=3 during flip window (timing or instant finish).")
        return 2 if saw_eq else 4
    log("\nPASS: flip slew + GXAS meridian-flip kind.")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
