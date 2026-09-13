#!/usr/bin/env python3
"""
Nantes GOTO safety sweep across all TeenAstro mount types.

Verifies on mainunit_emu (TCP 9997):
  1) Targets below minAlt / above maxAlt are rejected (horizon / overhead / "under tripod").
  2) In-band Alt-Az targets never return BELOWHORIZON or ABOVEOVERHEAD at the gate.
  3) Representative mid-sky gotos are accepted and the FULL slew path is monitored:
     altitude is sampled continuously until idle; any sample outside [minAlt, maxAlt] fails.
  4) Chained path slews (E/W/N/S, low→high) are monitored the same way.
  5) :MS# equatorial smoke (when accepted) is path-monitored too.

Site: Nantes ~47°13′N, 1°33′W.
Mount types: 1=GEM, 2=Eq-Fork, 3=AltAz-Tee, 4=AltAz-Fork.

Usage:
  python -u emu_nantes_goto_safety_test.py [--manage-emu] [host] [port]

  --manage-emu  Start/stop/restart mainunit_emu.exe for each :S!n# mount-type change.
"""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import time

# ---- ErrorsGoTo (CommandEnums.h) ----
ERRGOTO_NONE = 0
ERRGOTO_BELOWHORIZON = 1
ERRGOTO_SLEWING = 5
ERRGOTO_LIMITS = 6
ERRGOTO_ABOVEOVERHEAD = 8

MOUNT_TYPES = (
    (1, "GEM"),
    (2, "Eq-Fork"),
    (3, "AltAz-Tee"),
    (4, "AltAz-Fork"),
)

# Nantes
NANTES_LAT = ":St+47:13:00#"
NANTES_LON = ":Sg-001:33:00#"

# Explicit limits for this test (overhead = tripod clearance)
TEST_MIN_ALT = 0
TEST_MAX_ALT = 85

# Tolerance while sampling (arcmin-scale); firmware uses integer degree limits.
ALT_SAMPLE_TOL_DEG = 0.6

# Mid-sky targets: each accepted :MA# is flown to completion with path monitoring.
SAFE_TARGETS = (
    (90, 45),
    (180, 45),
    (270, 45),
    (180, 30),
    (180, 60),
    (45, 40),
    (315, 40),
)

# Chained slews stressing long arcs / near-overhead (full path each leg).
PATH_SEQUENCE = (
    (180, 25),
    (90, 50),
    (0, 35),
    (270, 55),
    (180, 80),
    (135, 20),
    (225, 70),
)

# Az/Alt grid for horizon & overhead gating only (rejects do not move; accepts aborted)
AZ_STEP = 45
ALT_CHECK_POINTS = (-5, -1, 0, 1, 20, 45, 70, 84, 85, 86, 90)

SLEW_TIMEOUT_S = 180.0
SAMPLE_PERIOD_S = 0.08


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass


def errgoto_name(code):
    names = {
        0: "NONE",
        1: "BELOWHORIZON",
        2: "NOOBJECT",
        3: "SAMESIDE",
        4: "PARKED",
        5: "SLEWING",
        6: "LIMITS",
        7: "GUIDINGBUSY",
        8: "ABOVEOVERHEAD",
        9: "MOTOR",
        11: "MOTOR_FAULT",
        12: "ALT",
        13: "LIMIT_SENSE",
        14: "AXIS1",
        15: "AXIS2",
        16: "UNDER_POLE",
        17: "MERIDIAN",
    }
    return names.get(code, "ERR_%d" % code)


def parse_errgoto(reply):
    if not reply:
        return None
    return ord(reply[0]) - ord("0")


def fmt_sa(alt_deg):
    sign = "+" if alt_deg >= 0 else "-"
    a = abs(int(round(alt_deg)))
    return ":Sa%s%02d*00:00#" % (sign, a)


def fmt_sz(az_deg):
    az = int(round(az_deg)) % 360
    return ":Sz%03d*00:00#" % az


def send_recv(sock, cmd, wait=0.04):
    """Read LX200 reply: '#'-terminated, or bare digit (:MA#/:MS#/:S*)."""
    sock.sendall(cmd.encode("ascii"))
    time.sleep(wait)
    buf = b""
    deadline = time.time() + 2.0
    sock.settimeout(0.12)
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


def connect(host, port, timeout=15.0):
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
        time.sleep(0.4)

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
        time.sleep(1.0)

    def alive(self):
        return self.proc is not None and self.proc.poll() is None


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


def parse_dms_deg(reply):
    s = reply.strip().rstrip("#")
    if not s:
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


def read_alt_deg(sock):
    return parse_dms_deg(send_recv(sock, ":GA#", wait=0.02))


def read_az_deg(sock):
    return parse_dms_deg(send_recv(sock, ":GZ#", wait=0.02))


def is_slewing(sock):
    r = send_recv(sock, ":D#", wait=0.02)
    return bool(r) and ord(r[0]) == 0x7F


def send_cmd_noreply(sock, cmd, wait=0.12):
    try:
        sock.sendall(cmd.encode("ascii"))
    except Exception:
        pass
    time.sleep(wait)
    sock.settimeout(0.05)
    try:
        while sock.recv(4096):
            pass
    except Exception:
        pass


def abort_slew(sock):
    send_cmd_noreply(sock, ":Q#", wait=0.15)


def alt_outside_limits(alt):
    return alt < (TEST_MIN_ALT - ALT_SAMPLE_TOL_DEG) or alt > (TEST_MAX_ALT + ALT_SAMPLE_TOL_DEG)


def monitor_slew_path(sock, label, tag, failed, timeout_s=SLEW_TIMEOUT_S):
    """
    Sample :GA# for the entire slew until :D# reports idle (including settle).
    Fails if altitude leaves [minAlt, maxAlt] at any sample.
    Returns (ok, samples, alt_min, alt_max, elapsed_s).
    """
    t0 = time.time()
    samples = 0
    alt_min = 1e9
    alt_max = -1e9
    saw_motion = False
    idle_streak = 0

    # Brief window to observe motion start
    while time.time() - t0 < 1.5:
        if is_slewing(sock):
            saw_motion = True
            break
        alt = read_alt_deg(sock)
        if alt is not None:
            samples += 1
            alt_min = min(alt_min, alt)
            alt_max = max(alt_max, alt)
            if alt_outside_limits(alt):
                failed.append(
                    "%s: %s path alt=%.2f outside [%d,%d] before/at start"
                    % (label, tag, alt, TEST_MIN_ALT, TEST_MAX_ALT)
                )
                abort_slew(sock)
                return False, samples, alt_min, alt_max, time.time() - t0
        time.sleep(SAMPLE_PERIOD_S)

    while time.time() - t0 < timeout_s:
        alt = read_alt_deg(sock)
        if alt is not None:
            samples += 1
            alt_min = min(alt_min, alt)
            alt_max = max(alt_max, alt)
            if alt_outside_limits(alt):
                az = read_az_deg(sock)
                failed.append(
                    "%s: %s DURING SLEW alt=%.2f az=%s outside [%d,%d] (sample %d, t=%.1fs)"
                    % (
                        label,
                        tag,
                        alt,
                        ("%.1f" % az) if az is not None else "?",
                        TEST_MIN_ALT,
                        TEST_MAX_ALT,
                        samples,
                        time.time() - t0,
                    )
                )
                abort_slew(sock)
                return False, samples, alt_min, alt_max, time.time() - t0

        moving = is_slewing(sock)
        if moving:
            saw_motion = True
            idle_streak = 0
        else:
            idle_streak += 1
            # Require a few consecutive idle polls so post-arrival settle is included
            if saw_motion and idle_streak >= 3:
                # Final sample after settle
                alt = read_alt_deg(sock)
                if alt is not None:
                    samples += 1
                    alt_min = min(alt_min, alt)
                    alt_max = max(alt_max, alt)
                    if alt_outside_limits(alt):
                        failed.append(
                            "%s: %s AFTER SETTLE alt=%.2f outside [%d,%d]"
                            % (label, tag, alt, TEST_MIN_ALT, TEST_MAX_ALT)
                        )
                        return False, samples, alt_min, alt_max, time.time() - t0
                return True, samples, alt_min, alt_max, time.time() - t0
            # Very short slew (already idle): still require at least one valid alt sample
            if (not saw_motion) and samples > 0 and (time.time() - t0) > 0.4:
                return True, samples, alt_min, alt_max, time.time() - t0

        time.sleep(SAMPLE_PERIOD_S)

    failed.append(
        "%s: %s slew timeout after %.0fs (%d samples, alt %.1f..%.1f)"
        % (label, tag, timeout_s, samples, alt_min if samples else 0, alt_max if samples else 0)
    )
    abort_slew(sock)
    return False, samples, alt_min, alt_max, time.time() - t0


def try_ma(sock, az, alt):
    rsz = send_recv(sock, fmt_sz(az))
    rsa = send_recv(sock, fmt_sa(alt))
    if not rsz.startswith("1") or not rsa.startswith("1"):
        return -1
    return parse_errgoto(send_recv(sock, ":MA#"))


def goto_ma_and_monitor(sock, az, alt, label, tag, failed, stats):
    """Start :MA# and monitor the entire path. Updates stats['path_ok'] on success."""
    code = try_ma(sock, az, alt)
    if code == ERRGOTO_SLEWING:
        abort_slew(sock)
        code = try_ma(sock, az, alt)
    if code is None or code == -1:
        failed.append("%s: %s set/goto failed (code=%r)" % (label, tag, code))
        return False
    if code != ERRGOTO_NONE:
        failed.append(
            "%s: %s :MA# az=%d alt=%d rejected as %s (expected path slew)"
            % (label, tag, az, alt, errgoto_name(code))
        )
        return False
    ok, n, amin, amax, dt = monitor_slew_path(sock, label, tag, failed)
    if ok:
        stats["path_ok"] += 1
        stats["path_samples"] += n
        log(
            "  path OK  %s az=%d alt=%d  samples=%d  alt=[%.2f,%.2f]  %.1fs"
            % (tag, az, alt, n, amin, amax, dt)
        )
    return ok


def setup_site_and_limits(tx, failed, label):
    tx(":hF#")
    time.sleep(0.1)
    rlat = tx(NANTES_LAT)
    rlon = tx(NANTES_LON)
    if not rlat.startswith("1") or not rlon.startswith("1"):
        failed.append("%s: set Nantes failed lat=%r lon=%r" % (label, rlat, rlon))
    rsh = tx(":Sh%d#" % TEST_MIN_ALT)
    rso = tx(":So%d#" % TEST_MAX_ALT)
    if not rsh.startswith("1") or not rso.startswith("1"):
        failed.append("%s: set limits failed Sh=%r So=%r" % (label, rsh, rso))
    gh = tx(":Gh#")
    go = tx(":Go#")
    gt = tx(":Gt#")
    gg = tx(":Gg#")
    log("  site/limits  Gt=%r Gg=%r Gh=%r Go=%r" % (gt.strip(), gg.strip(), gh.strip(), go.strip()))
    tx(":hR#")
    time.sleep(0.15)


def run_mount_type(sock, mount_name, failed):
    label = mount_name
    stats = {
        "horizon_ok": 0,
        "overhead_ok": 0,
        "inband_gate_ok": 0,
        "limits_reject": 0,
        "path_ok": 0,
        "path_samples": 0,
    }

    def tx(cmd):
        return send_recv(sock, cmd)

    setup_site_and_limits(tx, failed, label)

    # --- Gate only: reject out-of-band; abort any accidental accept ---
    log("  gate sweep (no path follow on rejects / abort on in-band accepts)...")
    for az in range(0, 360, AZ_STEP):
        for alt in ALT_CHECK_POINTS:
            code = try_ma(sock, az, alt)
            if code == -1:
                failed.append("%s: Sa/Sz rejected az=%d alt=%d" % (label, az, alt))
                continue
            if code is None:
                failed.append("%s: no reply :MA# az=%d alt=%d" % (label, az, alt))
                continue
            if code == ERRGOTO_NONE:
                abort_slew(sock)
            elif code == ERRGOTO_SLEWING:
                abort_slew(sock)
                code = try_ma(sock, az, alt)
                if code == ERRGOTO_NONE:
                    abort_slew(sock)

            if alt < TEST_MIN_ALT:
                if code != ERRGOTO_BELOWHORIZON:
                    failed.append(
                        "%s: az=%d alt=%d expected BELOWHORIZON got %s"
                        % (label, az, alt, errgoto_name(code))
                    )
                else:
                    stats["horizon_ok"] += 1
            elif alt > TEST_MAX_ALT:
                if code != ERRGOTO_ABOVEOVERHEAD:
                    failed.append(
                        "%s: az=%d alt=%d expected ABOVEOVERHEAD (under tripod) got %s"
                        % (label, az, alt, errgoto_name(code))
                    )
                else:
                    stats["overhead_ok"] += 1
            else:
                if code in (ERRGOTO_BELOWHORIZON, ERRGOTO_ABOVEOVERHEAD):
                    failed.append(
                        "%s: az=%d alt=%d in-band but got %s"
                        % (label, az, alt, errgoto_name(code))
                    )
                else:
                    stats["inband_gate_ok"] += 1
                    if code == ERRGOTO_LIMITS:
                        stats["limits_reject"] += 1

    # --- Full path monitoring: safe mid-sky targets ---
    log("  full-path slews (SAFE_TARGETS)...")
    for az, alt in SAFE_TARGETS:
        goto_ma_and_monitor(sock, az, alt, label, "safe", failed, stats)

    # --- Full path monitoring: chained stress sequence ---
    log("  full-path slews (PATH_SEQUENCE)...")
    for i, (az, alt) in enumerate(PATH_SEQUENCE):
        goto_ma_and_monitor(sock, az, alt, label, "leg%d" % (i + 1), failed, stats)

    # --- EQ :MS# with full path when accepted ---
    log("  full-path :MS# smoke...")
    gl = send_recv(sock, ":GL#")
    send_recv(sock, ":Sr12:00:00#")
    send_recv(sock, ":Sd+40*00:00#")
    code = parse_errgoto(send_recv(sock, ":MS#"))
    if code == ERRGOTO_SLEWING:
        abort_slew(sock)
        code = parse_errgoto(send_recv(sock, ":MS#"))
    if code == ERRGOTO_NONE:
        ok, n, amin, amax, dt = monitor_slew_path(sock, label, "MS_Sr12_Dec40", failed)
        if ok:
            stats["path_ok"] += 1
            stats["path_samples"] += n
            log("  path OK  :MS# Sr12 Dec+40  samples=%d  alt=[%.2f,%.2f]  %.1fs" % (n, amin, amax, dt))
    elif code in (ERRGOTO_BELOWHORIZON, ERRGOTO_ABOVEOVERHEAD):
        send_recv(sock, ":Sd+60*00:00#")
        code2 = parse_errgoto(send_recv(sock, ":MS#"))
        if code2 == ERRGOTO_NONE:
            ok, n, amin, amax, dt = monitor_slew_path(sock, label, "MS_Sr12_Dec60", failed)
            if ok:
                stats["path_ok"] += 1
                stats["path_samples"] += n
                log("  path OK  :MS# Sr12 Dec+60  samples=%d  alt=[%.2f,%.2f]  %.1fs" % (n, amin, amax, dt))
        elif code2 in (ERRGOTO_LIMITS, ERRGOTO_BELOWHORIZON, ERRGOTO_ABOVEOVERHEAD):
            log("  :MS# skipped (unreachable: %s, GL=%r)" % (errgoto_name(code2), gl.strip()))
        else:
            failed.append("%s: :MS# unexpected %s" % (label, errgoto_name(code2) if code2 is not None else "?"))
            abort_slew(sock)
    elif code == ERRGOTO_LIMITS:
        log("  :MS# LIMITS at gate — OK (no path)")
    else:
        failed.append("%s: :MS# unexpected %s" % (label, errgoto_name(code) if code is not None else "?"))
        abort_slew(sock)

    log(
        "  stats %s: horizon=%d overhead=%d inband_gate=%d limits_rej=%d path_ok=%d samples=%d"
        % (
            label,
            stats["horizon_ok"],
            stats["overhead_ok"],
            stats["inband_gate_ok"],
            stats["limits_reject"],
            stats["path_ok"],
            stats["path_samples"],
        )
    )
    return stats


def set_mount_type_and_restart(sock, emu, host, port, mount_type):
    try:
        sock.sendall((":S!%d#" % mount_type).encode("ascii"))
        time.sleep(0.3)
        try:
            sock.recv(64)
        except Exception:
            pass
    except Exception:
        pass
    try:
        sock.close()
    except Exception:
        pass
    time.sleep(0.8)
    if emu is not None:
        if emu.alive():
            time.sleep(1.0)
        if emu.alive():
            emu.stop()
        emu.start()
    return connect(host, port, timeout=20.0)


def ensure_mount_type(sock, emu, host, port, want_type, failed):
    if emu is None:
        log("  requesting mount type %d (:S!%d#) — restart mainunit_emu if process exits" % (want_type, want_type))
        try:
            sock.sendall((":S!%d#" % want_type).encode("ascii"))
            time.sleep(0.5)
            sock.recv(16)
        except Exception:
            pass
        try:
            sock.close()
        except Exception:
            pass
        time.sleep(1.0)
        try:
            return connect(host, port, timeout=5.0)
        except OSError:
            failed.append(
                "mount type %d: emulator exited after :S! (restart mainunit_emu or use --manage-emu)"
                % want_type
            )
            return None
    return set_mount_type_and_restart(sock, emu, host, port, want_type)


def main():
    manage, host, port = parse_args(sys.argv[1:])
    failed = []
    emu = None
    exe, emu_root = find_emu_exe()

    if manage:
        if not exe:
            log("ERROR: mainunit_emu.exe not found under %s — build with:" % emu_root)
            log("  pio run -d TeenAstroEmulator -e emu_mainunit")
            return 2
        emu = EmuProcess(exe, emu_root)
        log("Managing emulator: %s (cwd=%s)" % (exe, emu_root))
        emu.start()

    try:
        sock = connect(host, port)
    except OSError as e:
        log("ERROR: %s" % e)
        log("Start mainunit_emu or pass --manage-emu")
        if emu:
            emu.stop()
        return 3

    log(
        "Connected %s:%s — Nantes GOTO path safety (limits %d..%d deg, full-slew monitor)\n"
        % (host, port, TEST_MIN_ALT, TEST_MAX_ALT)
    )

    try:
        for mtype, mname in MOUNT_TYPES:
            log("=== Mount type %d (%s) ===" % (mtype, mname))
            sock = ensure_mount_type(sock, emu, host, port, mtype, failed)
            if sock is None:
                continue
            time.sleep(0.5)
            run_mount_type(sock, mname, failed)
            log("")
    finally:
        try:
            sock.close()
        except Exception:
            pass
        if emu:
            emu.stop()

    if failed:
        log("FAILED (%d):" % len(failed))
        for f in failed:
            log("  - %s" % f)
        return 1

    log(
        "ALL PASSED — Nantes, all mount types: gates OK; every accepted slew stayed "
        "within altitude limits for the entire movement."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
