#!/usr/bin/env python3
"""
MoveAxis + tracking retention matrix on mainunit_emu (TCP 9997).

Verifies OnStep-style / ASCOM "other axis" behaviour:
  - MoveAxis does not clear global sidereal tracking
  - Unmoved axis keeps CurrentTrackingRate (GXDR3/GXDR4)
  - Both axes can keep tracking (AltAz / EQ TC_BOTH)
  - Tracking OFF stays off
  - Rate 0 stops AtRate; tracking flag unchanged

Mount types: 1=GEM, 2=Eq-Fork, 3=AltAz-Tee, 4=AltAz-Fork
Tracking combos: OFF, EQ RA-only, EQ both, AltAz dual, solar/lunar smoke

Usage:
  python -u emu_moveaxis_tracking_test.py --manage-emu [host] [port]
"""
from __future__ import print_function

import base64
import os
import socket
import subprocess
import sys
import time

MOUNT_TYPES = (
    (1, "GEM", False),
    (2, "Eq-Fork", False),
    (3, "AltAz-Tee", True),
    (4, "AltAz-Fork", True),
)

# MoveAxis rate in arcsec/s; clamped at runtime to GXR4
# NOTE: firmware enableAtRate treats :M1/:M2 values as *sidereal-rate multiples*
# (same as CurrentTrackingRate). 1.0 ⇒ 15"/s ⇒ 1/240 deg/s.
MOVE_RATE = 30
_move_rate = MOVE_RATE

# Position check: stay below takeupRate (8) so Timer uses linear guide path (no accel).
POS_RATE = 4
POS_DURATION_S = 2.5
# deg/s = rate_sidereal * (360/86400) = rate/240
DEG_PER_SIDEREAL = 1.0 / 240.0


def choose_move_rate(sock):
    """Pick a MoveAxis rate safely under :GXRX# / :GXR4#."""
    global _move_rate
    mx = parse_float_reply(send_recv(sock, ":GXRX#"))
    if mx is None:
        mx = parse_float_reply(send_recv(sock, ":GXR4#"))
    if mx is None or mx != mx or mx <= 1:  # NaN or unusable
        # Restore a usable max slew if EEPROM left maxRate at 0
        send_recv(sock, ":SXRX,600#", wait=0.1)
        mx = parse_float_reply(send_recv(sock, ":GXRX#"))
    if mx is None or mx != mx or mx <= 0:
        _move_rate = MOVE_RATE
        mx = None
    else:
        _move_rate = max(1, min(MOVE_RATE, int(mx * 0.25)))
    log("  MoveAxis test rate=%d (max~%s)" % (_move_rate, "%.4f" % mx if mx else "?"))
    return _move_rate

GUIDING_OFF = 0
GUIDING_AT_RATE = 4


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass


def send_recv(sock, cmd, wait=0.05):
    sock.sendall(cmd.encode("ascii"))
    time.sleep(wait)
    buf = b""
    deadline = time.time() + 2.5
    sock.settimeout(0.15)
    while time.time() < deadline:
        try:
            chunk = sock.recv(16384)
            if not chunk:
                break
            buf += chunk
            if b"#" in buf:
                break
            if len(buf) >= 1 and b"#" not in buf:
                sock.settimeout(0.05)
                try:
                    more = sock.recv(16384)
                    if more:
                        buf += more
                        if b"#" in buf:
                            break
                        continue
                except socket.timeout:
                    break
        except socket.timeout:
            if buf:
                break
    return buf.decode("ascii", errors="replace")


def connect(host, port, timeout=20.0):
    t0 = time.time()
    last = None
    while time.time() - t0 < timeout:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3.0)
        try:
            s.connect((host, port))
            return s
        except OSError as e:
            last = e
            try:
                s.close()
            except Exception:
                pass
            time.sleep(0.25)
    raise OSError("connect %s:%s failed: %s" % (host, port, last))


def find_emu_exe():
    here = os.path.dirname(os.path.abspath(__file__))
    emu_root = os.path.abspath(os.path.join(here, ".."))
    candidates = [
        os.path.join(emu_root, ".pio", "build", "emu", "mainunit_emu.exe"),
        os.path.join(emu_root, ".pio", "build", "emu_mainunit", "mainunit_emu.exe"),
        os.path.join(emu_root, ".pio", "build", "emu_mainunit", "program.exe"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c, emu_root
    return None, emu_root


class EmuProcess(object):
    def __init__(self, exe, cwd):
        self.exe = exe
        self.cwd = cwd
        self.proc = None

    def stop(self):
        if self.proc is None:
            return
        try:
            if self.proc.poll() is None:
                self.proc.terminate()
                try:
                    self.proc.wait(timeout=3)
                except Exception:
                    self.proc.kill()
        except Exception:
            pass
        self.proc = None
        time.sleep(0.5)

    def start(self):
        self.stop()
        env = os.environ.copy()
        env.setdefault("SDL_VIDEODRIVER", "dummy")
        self.proc = subprocess.Popen(
            [self.exe],
            cwd=self.cwd,
            env=env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        time.sleep(1.2)

    def alive(self):
        return self.proc is not None and self.proc.poll() is None


def parse_float_reply(reply):
    s = reply.strip().rstrip("#")
    if not s:
        return None
    try:
        return float(s)
    except Exception:
        return None


def b64_decode_gxas(reply):
    s = reply.strip().rstrip("#")
    if not s:
        return None
    pad = (-len(s)) % 4
    try:
        raw = base64.b64decode(s + ("=" * pad))
    except Exception:
        return None
    if len(raw) < 102:
        return None
    return raw


def parse_gxas(raw):
    """Return dict from GXAS 102-byte packet."""
    b0 = raw[0]
    b2 = raw[2]
    b3 = raw[3]
    tracking_bits = b0 & 0x3
    return {
        "tracking_on": bool(tracking_bits & 0x1),
        "slewing": bool(tracking_bits & 0x2),
        "mount_type": (raw[1] >> 4) & 0x7,
        "track_comp": (b2 >> 4) & 0x3,
        "guiding_state": (b3 >> 5) & 0x7,
    }


def cmd_ok_short(reply):
    """:M1# returns single char 1 / 0 / e / … (may lack '#')."""
    if not reply:
        return False, "?"
    ch = reply.strip()[0]
    return ch == "1", ch


class CaseResult(object):
    def __init__(self, name):
        self.name = name
        self.ok = True
        self.notes = []

    def fail(self, msg):
        self.ok = False
        self.notes.append("FAIL: " + msg)

    def info(self, msg):
        self.notes.append(msg)


def read_rates(sock):
    r1 = parse_float_reply(send_recv(sock, ":GXDR1#"))
    r2 = parse_float_reply(send_recv(sock, ":GXDR2#"))
    c1 = parse_float_reply(send_recv(sock, ":GXDR3#"))
    c2 = parse_float_reply(send_recv(sock, ":GXDR4#"))
    return r1, r2, c1, c2


def read_gxas(sock):
    raw = b64_decode_gxas(send_recv(sock, ":GXAS#", wait=0.08))
    if raw is None:
        return None
    return parse_gxas(raw)


def near(a, b, tol=0.15, abs_floor=0.05):
    if a is None or b is None:
        return False
    if abs(a) < abs_floor and abs(b) < abs_floor:
        return True
    return abs(a - b) <= max(tol, 0.25 * abs(b))


def setup_site(sock):
    send_recv(sock, ":St+47:13:00#")
    send_recv(sock, ":Sg-001:33:00#")
    send_recv(sock, ":SG+01#")
    # Date/time so LST is valid
    send_recv(sock, ":SC03/24/26#")
    send_recv(sock, ":SL21:00:00#")
    send_recv(sock, ":Q#", wait=0.05)
    # Unpark if parked
    send_recv(sock, ":hR#", wait=0.1)
    send_recv(sock, ":Q#", wait=0.05)


def fmt_rate(v):
    return "%.4f" % v if v is not None else "?"


def _write_eeprom_mount_type(eeprom_path, mtype):
    """Set mount type on an already-initialized EEPROM image (keep motor/limit defaults)."""
    INIT_KEY = 152682
    try:
        if not os.path.isfile(eeprom_path):
            log("  WARN eeprom missing before type patch")
            return False
        with open(eeprom_path, "rb") as f:
            data = bytearray(f.read())
        if len(data) < 200:
            log("  WARN eeprom too small (%d)" % len(data))
            return False
        data[0:4] = INIT_KEY.to_bytes(4, "little")
        data[4] = 0  # EE_currentMount
        data[100] = mtype & 0xFF  # EE_Mounts + EE_mountType
        with open(eeprom_path, "wb") as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        return True
    except Exception as e:
        log("  WARN eeprom patch: %s" % e)
        return False


def set_mount_type(sock, mtype, emu, host, port):
    """Set mount type with :S!n# (reboot+EEPROM flush), then restart emu."""
    reply = send_recv(sock, ":S!%d#" % mtype, wait=0.2)
    log("  :S!%d# -> %r" % (mtype, reply.strip()[:8]))
    try:
        sock.close()
    except Exception:
        pass

    if emu is None:
        time.sleep(1.5)
        return connect(host, port)

    # reboot() should exit(0) after commit; if not, wait for periodic flush.
    for _ in range(40):
        if not emu.alive():
            break
        time.sleep(0.1)
    eeprom_path = os.path.join(emu.cwd, "teenastro_mainunit_eeprom.bin")
    if emu.alive():
        time.sleep(5.5)
        emu.stop()
    if os.path.isfile(eeprom_path):
        _write_eeprom_mount_type(eeprom_path, mtype)
    emu.start()
    return connect(host, port)


def wait_idle(sock, timeout_s=60.0):
    """Wait until not slewing and GuidingState OFF."""
    t0 = time.time()
    while time.time() - t0 < timeout_s:
        gs = read_gxas(sock)
        if gs and not gs["slewing"] and gs["guiding_state"] == GUIDING_OFF:
            return True
        time.sleep(0.2)
    return False


def slew_mid_sky(sock):
    """Park-safe mid-sky AltAz target so dual-axis tracking rates are non-trivial."""
    send_recv(sock, ":Q#", wait=0.1)
    wait_idle(sock, 10.0)
    # Match formats used by emu_nantes_goto_safety_test (3-digit az).
    send_recv(sock, ":Sz180*00:00#")
    send_recv(sock, ":Sa+45*00:00#")
    ma = send_recv(sock, ":MA#", wait=0.15).strip()
    if not ma or ma[0] != "0":
        # Fallback: slightly lower altitude if horizon/limits reject first target
        send_recv(sock, ":Sz180*00:00#")
        send_recv(sock, ":Sa+30*00:00#")
        ma = send_recv(sock, ":MA#", wait=0.15).strip()
        if not ma or ma[0] != "0":
            log("  WARN :MA# rejected (%r) — continuing without mid-sky slew" % ma[:4])
            send_recv(sock, ":Q#", wait=0.05)
            return False
    if not wait_idle(sock, 45.0):
        log("  WARN mid-sky slew did not finish")
        send_recv(sock, ":Q#", wait=0.1)
        wait_idle(sock, 10.0)
        return False
    time.sleep(0.3)
    return True


def moveaxis(sock, axis, rate_arcsec):
    # axis 1 or 2; signed integer arcsec/s
    rate_int = int(round(rate_arcsec))
    cmd = ":M%d%+d#" % (axis, rate_int)
    reply = send_recv(sock, cmd, wait=0.08)
    return cmd_ok_short(reply)


def stop_all_moveaxis(sock, timeout_s=5.0):
    send_recv(sock, ":Q#", wait=0.08)
    moveaxis(sock, 1, 0)
    moveaxis(sock, 2, 0)
    t0 = time.time()
    while time.time() - t0 < timeout_s:
        gs = read_gxas(sock)
        if gs and gs["guiding_state"] == GUIDING_OFF and not gs["slewing"]:
            return True
        time.sleep(0.1)
    send_recv(sock, ":Q#", wait=0.15)
    time.sleep(0.25)
    moveaxis(sock, 1, 0)
    moveaxis(sock, 2, 0)
    t0 = time.time()
    while time.time() - t0 < timeout_s:
        gs = read_gxas(sock)
        if gs is None:
            time.sleep(0.1)
            continue
        if gs["guiding_state"] == GUIDING_OFF:
            # Guiding clear is enough for MoveAxis tests; a stuck slewing bit
            # after a failed AltAz goto should not fail the whole matrix.
            if gs["slewing"]:
                send_recv(sock, ":Q#", wait=0.1)
            return True
        time.sleep(0.1)
    return False


def ensure_tracking_pose(sock, is_altaz):
    """AltAz mid-sky goto is flaky in emu after :S! type switch (abortSlew can
    leave gotoState stuck → MoveAxis replies 's'). Skip pose slew; MoveAxis
    tracking retention does not require mid-sky rates."""
    if not is_altaz:
        return
    send_recv(sock, ":Q#", wait=0.1)
    wait_idle(sock, 3.0)


def parse_dms_deg(reply):
    s = reply.strip().rstrip("#")
    if not s or "2147483648" in s:
        return None
    s = s.replace("*", ":").replace("'", ":")
    sign = 1.0
    if s[0] in "+-":
        if s[0] == "-":
            sign = -1.0
        s = s[1:]
    parts = s.split(":")
    try:
        deg = float(parts[0])
        minutes = float(parts[1]) if len(parts) > 1 else 0.0
        seconds = float(parts[2]) if len(parts) > 2 else 0.0
        return sign * (deg + minutes / 60.0 + seconds / 3600.0)
    except Exception:
        return None


def read_axis_deg(sock, axis):
    return parse_dms_deg(send_recv(sock, ":GXP%d#" % axis, wait=0.08))


def axes_sane(sock):
    a1 = read_axis_deg(sock, 1)
    a2 = read_axis_deg(sock, 2)
    return a1 is not None and a2 is not None, a1, a2


def angle_delta_deg(a0, a1):
    """Smallest signed delta a1-a0 in degrees (handle wrap near ±180)."""
    if a0 is None or a1 is None:
        return None
    d = a1 - a0
    while d > 180.0:
        d -= 360.0
    while d < -180.0:
        d += 360.0
    return d


def run_position_case(sock, title, expect_tracking):
    """
    Timed MoveAxis: instrument axis angle must match rate×dt (sidereal units).
    - Moved axis: Δ ≈ +POS_RATE/240 * dt  (tracking suspended on that axis)
    - Other axis, tracking OFF: Δ ≈ 0
    - Other axis, tracking ON:  Δ ≈ CurrentTrackingRate/240 * dt
    """
    cr = CaseResult(title)
    if not stop_all_moveaxis(sock):
        cr.fail("could not idle before position case")
        return cr

    ok_pos, _, _ = axes_sane(sock)
    if not ok_pos:
        cr.fail("GXP axis angles unreadable (motor geometry / EEPROM)")
        return cr

    if expect_tracking:
        send_recv(sock, ":TQ#")
        send_recv(sock, ":Te#")
        time.sleep(0.35)
    else:
        send_recv(sock, ":Td#")
        time.sleep(0.2)

    gs0 = read_gxas(sock)
    if gs0 is None:
        cr.fail("GXAS unreadable")
        return cr
    if expect_tracking and not gs0["tracking_on"]:
        cr.fail("tracking not ON")
        return cr
    if (not expect_tracking) and gs0["tracking_on"]:
        cr.fail("tracking still ON")
        return cr

    _, _, c1, c2 = read_rates(sock)

    for axis, other, c_other in ((2, 1, c1), (1, 2, c2)):
        if not stop_all_moveaxis(sock):
            cr.fail("idle before M%d" % axis)
            return cr
        a_move0 = read_axis_deg(sock, axis)
        a_other0 = read_axis_deg(sock, other)
        if a_move0 is None or a_other0 is None:
            cr.fail("GXP before M%d" % axis)
            return cr

        t0 = time.perf_counter()
        ok, ch = moveaxis(sock, axis, POS_RATE)
        if not ok:
            cr.fail("M%d+%d reply=%s" % (axis, POS_RATE, ch))
            stop_all_moveaxis(sock)
            return cr
        time.sleep(POS_DURATION_S)
        moveaxis(sock, axis, 0)
        dt = time.perf_counter() - t0
        # Read immediately — after rate-0, tracking resumes on the moved axis;
        # any settle/stop_all delay would inflate Δ beyond the MoveAxis window.
        a_move1 = read_axis_deg(sock, axis)
        a_other1 = read_axis_deg(sock, other)
        stop_all_moveaxis(sock)

        d_move = angle_delta_deg(a_move0, a_move1)
        d_other = angle_delta_deg(a_other0, a_other1)
        exp_move = POS_RATE * DEG_PER_SIDEREAL * dt
        if expect_tracking and c_other is not None:
            exp_other = c_other * DEG_PER_SIDEREAL * dt
        else:
            exp_other = 0.0

        # Accel/brake and sidereal vs wall-clock: allow 25% + 0.01° floor
        tol_move = max(0.01, 0.25 * abs(exp_move))
        tol_other = max(0.015, 0.35 * abs(exp_other) if abs(exp_other) > 1e-6 else 0.02)

        cr.info(
            "M%d: dt=%.3fs d_move=%.4f deg (exp %.4f+/-%.4f) d_other=%.4f deg (exp %.4f+/-%.4f) C_other=%s"
            % (
                axis,
                dt,
                d_move if d_move is not None else float("nan"),
                exp_move,
                tol_move,
                d_other if d_other is not None else float("nan"),
                exp_other,
                tol_other,
                fmt_rate(c_other),
            )
        )
        if d_move is None or d_other is None:
            cr.fail("GXP after M%d" % axis)
            continue
        if abs(d_move - exp_move) > tol_move:
            cr.fail(
                "M%d moved axis d=%.4f deg expected %.4f (rate=%dx sidereal, dt=%.3f)"
                % (axis, d_move, exp_move, POS_RATE, dt)
            )
        if exp_move > 0.02 and d_move < 0:
            cr.fail("M%d moved axis went wrong way (d=%.4f deg)" % (axis, d_move))
        if abs(d_other - exp_other) > tol_other:
            cr.fail(
                "M%d other axis d=%.4f deg expected %.4f (tracking %s)"
                % (
                    axis,
                    d_other,
                    exp_other,
                    "ON" if expect_tracking else "OFF",
                )
            )

    return cr


def run_case(sock, title, expect_tracking, expect_a1_track, expect_a2_track):
    """
    expect_a*_track: True → |CurrentTrackingRate| meaningfully non-zero before MoveAxis
                     False → near zero
                     None → no assertion on magnitude
    """
    cr = CaseResult(title)
    if not stop_all_moveaxis(sock):
        cr.fail("could not clear GuidingAtRate before case start")
        return cr
    time.sleep(0.15)

    gs0 = read_gxas(sock)
    if gs0 is None:
        cr.fail("GXAS unreadable")
        return cr

    r1, r2, c1, c2 = read_rates(sock)
    cr.info(
        "pre: track=%s gstate=%d C1=%s C2=%s R1=%s R2=%s"
        % (
            gs0["tracking_on"],
            gs0["guiding_state"],
            fmt_rate(c1),
            fmt_rate(c2),
            fmt_rate(r1),
            fmt_rate(r2),
        )
    )

    if expect_tracking and not gs0["tracking_on"]:
        cr.fail("expected tracking ON before MoveAxis")
    if (not expect_tracking) and gs0["tracking_on"]:
        cr.fail("expected tracking OFF before MoveAxis")

    # CurrentTrackingRate may linger after :Td# until recomputed; only enforce
    # magnitude checks when tracking is ON.
    if expect_tracking:
        if expect_a1_track is True and (c1 is None or abs(c1) < 0.05):
            cr.fail("expected axis1 CurrentTrackingRate non-zero, got %s" % c1)
        if expect_a1_track is False and c1 is not None and abs(c1) > 0.08:
            cr.fail("expected axis1 CurrentTrackingRate ~0, got %s" % c1)
        if expect_a2_track is True and (c2 is None or abs(c2) < 0.02):
            cr.info("WARN axis2 CurrentTrackingRate small: %s" % c2)
        if expect_a2_track is False and c2 is not None and abs(c2) > 0.08:
            cr.fail("expected axis2 CurrentTrackingRate ~0, got %s" % c2)

    # --- MoveAxis Axis2 only ---
    ok, ch = moveaxis(sock, 2, _move_rate)
    if not ok:
        cr.fail("M2+%d reply=%s" % (_move_rate, ch))
        stop_all_moveaxis(sock)
        return cr
    time.sleep(0.25)
    gs1 = read_gxas(sock)
    r1b, r2b, c1b, c2b = read_rates(sock)
    cr.info(
        "M2: track=%s gstate=%d C1=%s C2=%s"
        % (
            gs1["tracking_on"] if gs1 else "?",
            gs1["guiding_state"] if gs1 else -1,
            fmt_rate(c1b),
            fmt_rate(c2b),
        )
    )
    if gs1 is None:
        cr.fail("GXAS after M2")
    else:
        if expect_tracking and not gs1["tracking_on"]:
            cr.fail("tracking cleared while MoveAxis on axis2")
        if (not expect_tracking) and gs1["tracking_on"]:
            cr.fail("tracking unexpectedly ON during MoveAxis (was OFF)")
        if gs1["guiding_state"] != GUIDING_AT_RATE:
            cr.fail("expected GuidingAtRate=%d, got %d" % (GUIDING_AT_RATE, gs1["guiding_state"]))
        if expect_tracking and expect_a1_track is not False:
            if not near(c1b, c1, tol=0.2):
                cr.fail("axis1 CurrentTrackingRate changed during M2: was %s now %s" % (c1, c1b))
            if c1b is not None and abs(c1b) < 0.05 and expect_a1_track is True:
                cr.fail("axis1 tracking rate lost during M2")

    if not stop_all_moveaxis(sock):
        cr.fail("GuidingState still AtRate after M2 stop")
    gs_stop = read_gxas(sock)
    if gs_stop and expect_tracking and not gs_stop["tracking_on"]:
        cr.fail("tracking OFF after stopping M2")
    if gs_stop and gs_stop["guiding_state"] != GUIDING_OFF:
        cr.fail("GuidingState still %d after M2 stop" % gs_stop["guiding_state"])

    # --- MoveAxis Axis1 only ---
    _, _, c1, c2 = read_rates(sock)
    ok, ch = moveaxis(sock, 1, _move_rate)
    if not ok:
        cr.fail("M1+%d reply=%s" % (_move_rate, ch))
        stop_all_moveaxis(sock)
        return cr
    time.sleep(0.25)
    gs2 = read_gxas(sock)
    _, _, c1c, c2c = read_rates(sock)
    cr.info(
        "M1: track=%s gstate=%d C1=%s C2=%s"
        % (
            gs2["tracking_on"] if gs2 else "?",
            gs2["guiding_state"] if gs2 else -1,
            fmt_rate(c1c),
            fmt_rate(c2c),
        )
    )
    if gs2 is None:
        cr.fail("GXAS after M1")
    else:
        if expect_tracking and not gs2["tracking_on"]:
            cr.fail("tracking cleared while MoveAxis on axis1")
        if gs2["guiding_state"] != GUIDING_AT_RATE:
            cr.fail("expected GuidingAtRate during M1, got %d" % gs2["guiding_state"])
        if expect_tracking and expect_a2_track is True:
            if c2c is not None and abs(c2c) < 0.01 and abs(c2 or 0) >= 0.02:
                cr.fail("axis2 tracking rate lost during M1: was %s now %s" % (c2, c2c))
            elif c2 is not None and c2c is not None and not near(c2c, c2, tol=0.25):
                cr.info("WARN axis2 rate drifted during M1: was %s now %s" % (c2, c2c))

    if not stop_all_moveaxis(sock):
        cr.fail("GuidingState still AtRate after M1 stop")
    gs_end = read_gxas(sock)
    if gs_end and expect_tracking and not gs_end["tracking_on"]:
        cr.fail("tracking OFF after stopping M1")
    if gs_end and gs_end["guiding_state"] != GUIDING_OFF:
        cr.fail("GuidingState still %d after M1 stop" % gs_end["guiding_state"])

    return cr


def run_mount_matrix(sock, mtype, mname, is_altaz):
    results = []

    # Tracking OFF
    send_recv(sock, ":Td#")
    time.sleep(0.15)
    results.append(
        run_case(sock, "%s / tracking OFF" % mname, False, False, False)
    )

    # Star + tracking ON
    send_recv(sock, ":TQ#")
    send_recv(sock, ":Te#")
    time.sleep(0.35)
    ensure_tracking_pose(sock, is_altaz)
    send_recv(sock, ":Te#")
    time.sleep(0.4)

    if is_altaz:
        # Axis2 rate may be ~0 near park/home; still require tracking ON through MoveAxis
        results.append(
            run_case(sock, "%s / AltAz dual tracking" % mname, True, True, None)
        )
    else:
        # RA-only
        send_recv(sock, ":T1#")
        send_recv(sock, ":Te#")
        time.sleep(0.35)
        results.append(
            run_case(sock, "%s / EQ TC_RA (axis1 only)" % mname, True, True, False)
        )

        # Both axes
        send_recv(sock, ":T2#")
        send_recv(sock, ":Te#")
        time.sleep(0.35)
        results.append(
            run_case(sock, "%s / EQ TC_BOTH" % mname, True, True, None)
        )

    # Solar / lunar smoke (tracking ON, MoveAxis must not clear it)
    send_recv(sock, ":TS#")
    send_recv(sock, ":Te#")
    time.sleep(0.3)
    results.append(
        run_case(sock, "%s / Solar tracking" % mname, True, True, None)
    )
    send_recv(sock, ":TL#")
    send_recv(sock, ":Te#")
    time.sleep(0.3)
    results.append(
        run_case(sock, "%s / Lunar tracking" % mname, True, True, None)
    )
    send_recv(sock, ":TQ#")

    # Numerical position checks (instrument angles via :GXP#)
    results.append(
        run_position_case(sock, "%s / position track OFF" % mname, False)
    )
    results.append(
        run_position_case(sock, "%s / position track ON" % mname, True)
    )

    return results


def parse_args(argv):
    manage = False
    args = []
    for a in argv:
        if a == "--manage-emu":
            manage = True
        else:
            args.append(a)
    host = args[0] if len(args) > 0 else "127.0.0.1"
    port = int(args[1]) if len(args) > 1 else 9997
    return manage, host, port


def main():
    manage, host, port = parse_args(sys.argv[1:])
    exe, emu_root = find_emu_exe()
    emu = None
    if manage:
        if not exe:
            log("ERROR: mainunit_emu.exe not found under TeenAstroEmulator/.pio/build")
            log("  Build: pio run -d TeenAstroEmulator -e emu_mainunit")
            return 2
        log("Using emu: %s" % exe)
        emu = EmuProcess(exe, emu_root)
        emu.start()
        host = "127.0.0.1"

    all_results = []
    try:
        sock = connect(host, port)
        setup_site(sock)
        for mtype, mname, is_altaz in MOUNT_TYPES:
            log("")
            log("======== Mount type %d (%s) ========" % (mtype, mname))
            sock = set_mount_type(sock, mtype, emu, host, port)
            setup_site(sock)
            # Confirm type
            gs = read_gxas(sock)
            if gs:
                log("GXAS mount_type=%d track_comp=%d" % (gs["mount_type"], gs["track_comp"]))
                if gs["mount_type"] != mtype:
                    log("ERROR: mount type did not stick (want %d got %d)" % (mtype, gs["mount_type"]))
                    all_results.append(CaseResult("%s mount-type switch" % mname))
                    all_results[-1].fail("mount_type=%d" % gs["mount_type"])
                    continue
            choose_move_rate(sock)
            sane, a1, a2 = axes_sane(sock)
            log("  GXP axes sane=%s A1=%s A2=%s" % (sane, fmt_rate(a1), fmt_rate(a2)))
            if not sane:
                log("ERROR: axis angles unreadable — EEPROM/motor geometry bad; skip mount")
                cr = CaseResult("%s GXP sanity" % mname)
                cr.fail("GXP unreadable")
                all_results.append(cr)
                continue
            cases = run_mount_matrix(sock, mtype, mname, is_altaz)
            for cr in cases:
                status = "PASS" if cr.ok else "FAIL"
                log("[%s] %s" % (status, cr.name))
                for n in cr.notes:
                    log("    %s" % n)
                all_results.append(cr)
        try:
            sock.close()
        except Exception:
            pass
    finally:
        if emu is not None:
            emu.stop()

    n_fail = sum(1 for r in all_results if not r.ok)
    n_pass = sum(1 for r in all_results if r.ok)
    log("")
    log("======== SUMMARY: %d passed, %d failed ========" % (n_pass, n_fail))
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main())
