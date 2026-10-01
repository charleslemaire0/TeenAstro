#!/usr/bin/env python3
"""
MoveAxis stop deceleration on mainunit_emu (TCP 9997).

Release 1.5 stopped :M1/:M2 rate 0 with brake() only. Timer.ino then ramped
tmp_guideRate down to takeupRate and pulled the target in with
breakMoveHighRate / breakMoveLowRate. Both axes share that ramp, so they
slow at the same sidereal-rate slope:

    |d(rate)/dt| = maxSlew^2 / (480 * DegreesForAcceleration)

(clockRatio 0.01 s, stepsPerDegree/stepsPerSecond = 240). An instant setIdle()
on stop skips the ramp: the rate snaps to 0, or stays at the slew rate.

This test spins each axis above takeup, stops it, and checks:
  - the motor rate (sidereal interval / current step interval) falls over time
  - the fall matches the 1.5 slope
  - axis 1 and axis 2 slopes agree

Usage:
  python -u emu_moveaxis_decel_test.py --manage-emu [host] [port]
"""
from __future__ import print_function

import base64
import os
import socket
import subprocess
import sys
import time

TAKEUP_DEFAULT = 8.0
# Commanded rate in sidereal multiples. Must sit well above takeup so the
# high-rate brake path (the 1.5 mimic) is the one that runs.
CMD_RATE = 80
GUIDING_OFF = 0
GUIDING_AT_RATE = 4


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass


def send_recv(sock, cmd, wait=0.02):
    sock.sendall(cmd.encode("ascii"))
    if wait:
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
            if buf and b"#" not in buf:
                sock.settimeout(0.04)
                try:
                    more = sock.recv(16384)
                    if more:
                        buf += more
                        if b"#" in more or b"#" in buf:
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
    if len(raw) < 4:
        return None
    return raw


def read_guiding(sock):
    raw = b64_decode_gxas(send_recv(sock, ":GXAS#", wait=0.01))
    if raw is None:
        return None
    return (raw[3] >> 5) & 0x7


def cmd_ok_short(reply):
    if not reply:
        return False, "?"
    ch = reply.strip()[0]
    return ch == "1", ch


def moveaxis(sock, axis, rate):
    cmd = ":M%d%+d#" % (axis, int(rate))
    reply = send_recv(sock, cmd, wait=0.02)
    return cmd_ok_short(reply)


def read_interval(sock, axis):
    cmd = ":GXDRA#" if axis == 1 else ":GXDRB#"
    return parse_float_reply(send_recv(sock, cmd, wait=0.0))


def read_sidereal_interval(sock, axis):
    cmd = ":GXDP6#" if axis == 1 else ":GXDP7#"
    return parse_float_reply(send_recv(sock, cmd, wait=0.0))


def motor_rate(cur, sid):
    """Sidereal multiples from step intervals. 1.0 is sidereal tracking."""
    if cur is None or sid is None or cur <= 1.0 or sid <= 0.0:
        return None
    return sid / cur


def expected_slope(max_slew, degrees):
    """Sidereal-rate units per second, Release 1.5 Timer.ino ramp."""
    if max_slew is None or degrees is None or degrees <= 0.0:
        return None
    return (max_slew * max_slew) / (480.0 * degrees)


def setup(sock):
    send_recv(sock, ":St+47:13:00#")
    send_recv(sock, ":Sg-001:33:00#")
    send_recv(sock, ":SG+01#")
    send_recv(sock, ":SC03/24/26#")
    send_recv(sock, ":SL21:00:00#")
    send_recv(sock, ":Q#", wait=0.05)
    send_recv(sock, ":hR#", wait=0.1)
    send_recv(sock, ":Td#", wait=0.05)
    prev_max = parse_float_reply(send_recv(sock, ":GXRX#"))
    prev_deg = parse_float_reply(send_recv(sock, ":GXRA#"))
    # Known accel so the 1.5 ramp lasts long enough to sample over TCP.
    send_recv(sock, ":SXRX,300#", wait=0.1)
    send_recv(sock, ":SXRA,30#", wait=0.1)
    max_slew = parse_float_reply(send_recv(sock, ":GXR4#"))
    degrees = parse_float_reply(send_recv(sock, ":GXRA#"))
    takeup = parse_float_reply(send_recv(sock, ":GXRB#"))
    if takeup is None or takeup < 1.0:
        takeup = TAKEUP_DEFAULT
    return prev_max, prev_deg, max_slew, degrees, takeup


def restore(sock, prev_max, prev_deg):
    if prev_max is not None and prev_max > 1.0:
        send_recv(sock, ":SXRX,%d#" % int(round(prev_max)), wait=0.05)
    if prev_deg is not None and prev_deg > 0.0:
        send_recv(sock, ":SXRA,%d#" % int(round(prev_deg * 10.0)), wait=0.05)


def stop_both(sock):
    moveaxis(sock, 1, 0)
    moveaxis(sock, 2, 0)
    t0 = time.time()
    while time.time() - t0 < 6.0:
        g = read_guiding(sock)
        if g == GUIDING_OFF:
            return True
        time.sleep(0.05)
    return False


def wait_cruise(sock, axis, sid, cmd_rate, timeout_s=4.0):
    t0 = time.time()
    last = None
    while time.time() - t0 < timeout_s:
        last = motor_rate(read_interval(sock, axis), sid)
        # Wait until the accel ramp has actually arrived, not merely passed takeup.
        if last is not None and last >= 0.90 * cmd_rate:
            # Let the ramp finish at the commanded rate before the stop.
            time.sleep(0.35)
            settled = motor_rate(read_interval(sock, axis), sid)
            return settled if settled is not None else last
        time.sleep(0.03)
    return last


def sample_stop(sock, axis, sid, timeout_s=5.0):
    """Return list of (t_sec, rate, guiding) from the moment of rate 0."""
    t0 = time.perf_counter()
    ok, ch = moveaxis(sock, axis, 0)
    samples = []
    if not ok:
        return samples, ch
    while time.perf_counter() - t0 < timeout_s:
        rate = motor_rate(read_interval(sock, axis), sid)
        guiding = read_guiding(sock)
        t = time.perf_counter() - t0
        samples.append((t, rate, guiding))
        if (
            guiding == GUIDING_OFF
            and rate is not None
            and rate < 2.0
            and len(samples) >= 3
        ):
            break
        # Tight poll: the two reads above already pace the loop.
    return samples, ch


def high_rate_slope(samples, takeup):
    """Slope (sidereal/s, positive when slowing) while still above takeup."""
    pts = [(t, r) for (t, r, _g) in samples if r is not None and r > takeup + 1.0]
    if len(pts) < 2:
        return None, pts
    t0, r0 = pts[0]
    t1, r1 = pts[-1]
    dt = t1 - t0
    if dt < 0.05:
        return None, pts
    return (r0 - r1) / dt, pts


def check_axis(name, samples, reply_ch, cruise, cmd_rate, takeup, exp_slope):
    notes = []
    fails = []
    if reply_ch != "1":
        fails.append("stop reply=%s" % reply_ch)
        return fails, notes
    rates = [r for (_t, r, _g) in samples if r is not None]
    if len(rates) < 3:
        fails.append("only %d rate samples" % len(rates))
        return fails, notes

    t_end = samples[-1][0]
    g_end = samples[-1][2]
    r0 = rates[0]
    r_min = min(rates)
    notes.append(
        "cruise=%.1f first=%.1f min=%.1f end=%.1f n=%d t=%.3fs g=%s"
        % (cruise, r0, r_min, rates[-1], len(rates), t_end, g_end)
    )

    # 1.5 keeps braking after the stop command. An immediate setIdle snaps
    # the interval to maxInterval before the first sample.
    if r0 < max(takeup + 4.0, 0.45 * cruise):
        fails.append(
            "rate already %.1f on first sample after stop (cruise %.1f); ramp skipped"
            % (r0, cruise)
        )

    drop = r0 - r_min
    if drop < 0.35 * cmd_rate:
        fails.append(
            "rate only fell %.1f (from %.1f); deceleration mimic did not run"
            % (drop, r0)
        )

    if t_end < 0.08:
        fails.append("idle in %.3fs (instant stop, no deceleration)" % t_end)
    if g_end != GUIDING_OFF or rates[-1] > 2.0:
        fails.append(
            "did not settle: guiding=%s rate=%.2f after %.2fs"
            % (g_end, rates[-1], t_end)
        )

    # While above takeup the 1.5 ramp is monotonic (one tick of noise allowed).
    above = [r for r in rates if r > takeup + 1.0]
    inversions = 0
    for a, b in zip(above, above[1:]):
        if b > a + 3.0:
            inversions += 1
    if len(above) >= 3 and inversions > 1:
        fails.append("rate rose during brake (%d steps up)" % inversions)

    slope, pts = high_rate_slope(samples, takeup)
    if slope is None:
        fails.append("not enough samples above takeup %.0f to measure the ramp" % takeup)
    else:
        notes.append(
            "slope=%.1f /s over %.3fs (%d pts above takeup)"
            % (slope, pts[-1][0] - pts[0][0], len(pts))
        )
        if exp_slope and exp_slope > 0:
            ratio = slope / exp_slope
            notes.append("slope/1.5model=%.2f (model %.1f /s)" % (ratio, exp_slope))
            if ratio < 0.45 or ratio > 2.2:
                fails.append(
                    "slope %.1f /s is not the 1.5 ramp (expected %.1f /s)"
                    % (slope, exp_slope)
                )
    return fails, notes


def run(sock):
    prev_max, prev_deg, max_slew, degrees, takeup = setup(sock)
    log(
        "maxSlew=%s degAcc=%s takeup=%s"
        % (max_slew, degrees, takeup)
    )
    exp = expected_slope(max_slew, degrees)
    log("1.5 model |d(rate)/dt| = %.1f sidereal/s" % (exp or 0.0))
    if max_slew is None or max_slew < CMD_RATE + 5:
        log("FAIL: GXR4 too low for commanded rate %d" % CMD_RATE)
        restore(sock, prev_max, prev_deg)
        return 1

    slopes = {}
    failed = False
    try:
        for axis in (1, 2):
            if not stop_both(sock):
                log("FAIL axis%d: could not idle before start" % axis)
                failed = True
                break
            sid = read_sidereal_interval(sock, axis)
            if sid is None or sid <= 0:
                log("FAIL axis%d: no sidereal interval" % axis)
                failed = True
                break
            ok, ch = moveaxis(sock, axis, CMD_RATE)
            if not ok:
                log("FAIL axis%d: :M%d+%d# reply=%s" % (axis, axis, CMD_RATE, ch))
                failed = True
                break
            cruise = wait_cruise(sock, axis, sid, CMD_RATE)
            log("axis%d cruise rate=%.1f (cmd %d)" % (axis, cruise or -1, CMD_RATE))
            if cruise is None or cruise < 0.90 * CMD_RATE:
                log("FAIL axis%d: did not accelerate to rate (got %s)" % (axis, cruise))
                moveaxis(sock, axis, 0)
                failed = True
                break
            samples, ch = sample_stop(sock, axis, sid)
            fails, notes = check_axis(
                "axis%d" % axis, samples, ch, cruise, CMD_RATE, takeup, exp
            )
            for n in notes:
                log("  axis%d %s" % (axis, n))
            slope, _pts = high_rate_slope(samples, takeup)
            slopes[axis] = slope
            if fails:
                failed = True
                for f in fails:
                    log("FAIL axis%d: %s" % (axis, f))
            else:
                log("PASS axis%d deceleration matches the 1.5 ramp" % axis)
    finally:
        stop_both(sock)
        restore(sock, prev_max, prev_deg)

    if failed:
        return 1
    s1 = slopes.get(1)
    s2 = slopes.get(2)
    if s1 is None or s2 is None or s1 <= 0 or s2 <= 0:
        log("FAIL: missing slopes %s" % slopes)
        return 1
    ratio = s1 / s2
    log("axis1 slope=%.1f /s  axis2 slope=%.1f /s  ratio=%.2f" % (s1, s2, ratio))
    if ratio < 0.65 or ratio > 1.55:
        log("FAIL: axes did not decelerate the same (ratio %.2f)" % ratio)
        return 1
    log("PASS both axes decelerate like Release 1.5 and match each other")
    return 0


def parse_args(argv):
    manage = False
    args = []
    for a in argv:
        if a == "--manage-emu":
            manage = True
        else:
            args.append(a)
    host = args[0] if args else "127.0.0.1"
    port = int(args[1]) if len(args) > 1 else 9997
    return manage, host, port


def main():
    manage, host, port = parse_args(sys.argv[1:])
    exe, emu_root = find_emu_exe()
    emu = None
    if manage:
        if not exe:
            log("ERROR: mainunit_emu.exe not found. pio run -d TeenAstroEmulator -e emu_mainunit")
            return 2
        log("Using emu: %s" % exe)
        emu = EmuProcess(exe, os.path.dirname(exe))
        emu.start()
        host = "127.0.0.1"
    try:
        sock = connect(host, port)
    except OSError as e:
        log("ERROR: %s" % e)
        if emu:
            emu.stop()
        return 2
    try:
        return run(sock)
    finally:
        try:
            sock.close()
        except Exception:
            pass
        if emu:
            emu.stop()


if __name__ == "__main__":
    sys.exit(main())
