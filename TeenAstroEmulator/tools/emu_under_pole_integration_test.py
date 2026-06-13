#!/usr/bin/env python3
"""
Integration checks for under-pole limit configuration on MainUnit (emulator TCP or serial).

1) Read :GXLU# and GXCS bytes 68-69 (uint16 LE, under-pole limit x10); they must agree.
2) Optional (--write-test): set :SXLU,110# (11.0 h), verify GXLU/GXCS, restore original.
3) (--verify-twelve-hour-deactivation): run native PlatformIO test_under_pole (asserts >=12 h
    disables MountLimits::checkPole); then optionally on device temporarily :SXLU,120#, verify,
    restore.
4) (--native-only-under-pole): native tests only — no TCP (no emulator required).

MountLimits treats underPoleLimitGOTO >= 12.0 as “inactive” (OnStep / UniversalMainUnit parity).

GOTO failures blocked only by pier geometry otherwise use wire '6' (ERRGOTO_LIMITS); see docs in repo.

Usage:
  python -u emu_under_pole_integration_test.py [host] [port]
  python -u emu_under_pole_integration_test.py --write-test [host] [port]
  python -u emu_under_pole_integration_test.py --verify-twelve-hour-deactivation [host] [port]
  python -u emu_under_pole_integration_test.py --native-only-under-pole
  python -u emu_under_pole_integration_test.py --serial COM3 [baud]

Requires mainunit_emu (default 127.0.0.1:9997) or hardware, except --native-only-under-pole.
"""
from __future__ import print_function

import base64
import os
import socket
import struct
import subprocess
import sys
import time

try:
    import serial as pyserial
except ImportError:
    pyserial = None

GXCS_B64_LEN = 120
GXCS_PKT_LEN = 90


def repo_root_from_script():
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass


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


def strip_hash_b64(reply, want_len, name="b64"):
    s = reply.strip()
    if s.endswith("#"):
        s = s[:-1]
    if len(s) != want_len:
        return None, "bad %s len %d (want %d)" % (name, len(s), want_len)
    raw = base64.b64decode(s)
    return raw, None


def gxcs_under_pole_u10(raw):
    """GXCS bytes 68-69: uint16 LE, round(underPoleLimitGOTO * 10)."""
    if len(raw) < 70:
        return None, "GXCS too short"
    u = raw[68] | (raw[69] << 8)
    return u, None


def parse_gxlu(reply):
    s = reply.strip()
    if s.endswith("#"):
        s = s[:-1]
    try:
        return int(s), None
    except ValueError:
        return None, "bad GXLU %r" % reply


def run_native_under_pole_tests():
    """Same logic as MountLimits::checkPole is covered by tests/test_under_pole (SYNC)."""
    root = repo_root_from_script()
    tests_dir = os.path.join(root, "tests")
    if not os.path.isdir(tests_dir):
        log("FAIL: tests dir missing: %s" % tests_dir)
        return False
    log("Running: pio test -d tests -e native --filter test_under_pole")
    r = subprocess.run(
        ["pio", "test", "-d", tests_dir, "-e", "native", "--filter", "test_under_pole"],
        cwd=root,
    )
    if r.returncode != 0:
        log("FAIL: native test_under_pole exit %d" % r.returncode)
        return False
    log("OK: native test_under_pole passed (includes 12 h deactivation vs 11.99 h still limited).")
    return True


def parse_args():
    argv = list(sys.argv[1:])
    if "--help" in argv or "-h" in argv:
        print(__doc__)
        sys.exit(0)
    write_test = "--write-test" in argv
    native_only = "--native-only-under-pole" in argv
    verify_12 = "--verify-twelve-hour-deactivation" in argv
    strip = {
        "--write-test",
        "--native-only-under-pole",
        "--verify-twelve-hour-deactivation",
    }
    argv = [a for a in argv if a not in strip]
    if native_only and verify_12:
        log("ERROR: use only one of --native-only-under-pole / --verify-twelve-hour-deactivation")
        sys.exit(2)
    if native_only:
        return "native_only", None, write_test, verify_12
    if argv and argv[0] == "--serial":
        if len(argv) < 2:
            log("ERROR: need port, e.g. --serial COM3")
            sys.exit(2)
        if pyserial is None:
            log("ERROR: pip install pyserial")
            sys.exit(2)
        port = argv[1]
        baud = int(argv[2]) if len(argv) > 2 else 115200
        return "serial", (port, baud), write_test, verify_12
    host = argv[0] if len(argv) > 0 else "127.0.0.1"
    port = int(argv[1]) if len(argv) > 1 else 9997
    return "tcp", (host, port), write_test, verify_12


def device_verify_twelve_hour_roundtrip(tx, failed, gxlu_orig):
    """Temporary :SXLU,120#; confirm GXLU and GXCS; restore prior value."""
    tx("SXLU_12h", ":SXLU,120#")
    lu = tx("GXLU_12h", ":GXLU#")
    cs = tx("GXCS_12h", ":GXCS#")
    v, e = parse_gxlu(lu)
    if e:
        failed.append(e)
    raw, e2 = strip_hash_b64(cs, GXCS_B64_LEN, "GXCS")
    if e2:
        failed.append(e2)
    else:
        u10, e3 = gxcs_under_pole_u10(raw)
        if e3:
            failed.append(e3)
        elif v is not None:
            if v != 120 or abs(u10 - v) > 1:
                failed.append("12h probe: want GXLU=120 GXCSu10~=120 got %d / %r" % (v, u10))
            else:
                log(
                    "OK: device stores 12 h (120). Firmware skips under-pole check when limit >= 12 "
                    "(MountLimits::checkPole)."
                )
    tx("SXLU_restore_12h", ":SXLU,%d#" % int(gxlu_orig))
    back = tx("GXLU_restored_12h", ":GXLU#")
    vr, er = parse_gxlu(back)
    if er:
        failed.append(er)
    elif vr is not None and abs(vr - int(gxlu_orig)) > 1:
        failed.append("after 12h probe restore wanted GXLU~%d got %r" % (gxlu_orig, back))


def main():
    mode, conn, write_test, verify_12 = parse_args()

    if mode == "native_only":
        return 0 if run_native_under_pole_tests() else 1

    if verify_12 and not run_native_under_pole_tests():
        return 2

    if mode == "tcp":
        host, port = conn
        tr = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tr.settimeout(10.0)
        try:
            tr.connect((host, port))
        except OSError as ex:
            log("TCP connect failed %s:%s — %s" % (host, port, ex))
            log("Start mainunit_emu or use --native-only-under-pole for logic-only checks.")
            return 3
        send_recv = send_recv_tcp
        log("Connected %s:%s - under-pole integration\n" % (host, port))
    else:
        port_name, baud = conn
        tr = pyserial.Serial(port_name, baud, bytesize=8, parity="N", stopbits=1, timeout=1)
        send_recv = send_recv_serial
        log("Serial %s at %d - under-pole integration\n" % (port_name, baud))

    def tx(label, cmd):
        r = send_recv(tr, cmd)
        short = r[:120] + ("..." if len(r) > 120 else "")
        log("%-12s %s -> %r" % (label, cmd, short))
        return r

    failed = []

    gxlu_r = tx("GXLU", ":GXLU#")
    gxcs_r = tx("GXCS", ":GXCS#")

    gxlu_val, err = parse_gxlu(gxlu_r)
    if err:
        log("FAIL: %s" % err)
        failed.append(err)

    raw_c, err = strip_hash_b64(gxcs_r, GXCS_B64_LEN, "GXCS")
    if err:
        log("FAIL: %s" % err)
        failed.append(err)
    else:
        if len(raw_c) != GXCS_PKT_LEN:
            failed.append("GXCS pkt len %d" % len(raw_c))
        else:
            u10, err2 = gxcs_under_pole_u10(raw_c)
            if err2:
                failed.append(err2)
            else:
                if gxlu_val is not None:
                    delta = abs(gxlu_val - u10)
                    if delta <= 1:
                        log("OK: GXLU (%d) matches GXCS u10 (%d)" % (gxlu_val, u10))
                    else:
                        msg = "GXLU %d vs GXCS[%d] mismatch (>1)" % (gxlu_val, u10)
                        log("FAIL: %s" % msg)
                        failed.append(msg)

    if verify_12 and gxlu_val is not None and not failed:
        device_verify_twelve_hour_roundtrip(tx, failed, gxlu_val)
    elif verify_12 and gxlu_val is None:
        failed.append("12h device probe skipped (no baseline GXLU)")

    if write_test:
        if gxlu_val is None:
            log("skip --write-test (no baseline GXLU)")
        else:
            orig = gxlu_val
            tx("SXLU_test", ":SXLU,110#")
            gxlu2_r = tx("GXLU_after", ":GXLU#")
            gxcs2_r = tx("GXCS_after", ":GXCS#")
            v2, e2 = parse_gxlu(gxlu2_r)
            rc2, e3 = strip_hash_b64(gxcs2_r, GXCS_B64_LEN, "GXCS")
            if e2:
                failed.append(e2)
            elif e3:
                failed.append(e3)
            else:
                u10b, errb = gxcs_under_pole_u10(rc2)
                if errb:
                    failed.append(errb)
                elif v2 != 110 or abs(u10b - v2) > 1:
                    failed.append(
                        "after SXLU 110 expected GXLU 110 GXCS u10~=110; got GXLU=%r GXCS_u10=%r"
                        % (v2, u10b)
                    )
                else:
                    log("OK: EEPROM round-trip to 110 (11.0 h)")
                tx("SXLU_restore", ":SXLU,%d#" % orig)
                gxlu3_r = tx("GXLU_restored", ":GXLU#")
                v3, e4 = parse_gxlu(gxlu3_r)
                if e4:
                    failed.append(e4)
                elif v3 is not None and abs(v3 - orig) > 1:
                    failed.append("restore GXLU wanted %d got %r" % (orig, gxlu3_r))

    try:
        tr.close()
    except Exception:
        pass

    if failed:
        log("FAILED: %s" % "; ".join(failed))
        return 1
    log("All checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
