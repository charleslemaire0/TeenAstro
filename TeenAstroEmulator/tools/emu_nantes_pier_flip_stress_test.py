#!/usr/bin/env python3
"""
GEM pier-side stress test at Nantes — max slew speed, many hours, forced flips.

For each local hour (and half-hour), goto an eastern HA then a western HA so the
mount must change pier (E <-> W). Entire slew paths are altitude-monitored.

Also maxes emulator slew rate (:SXRX), minimizes accel distance (:SXRA), settle (:SXOS,0).

Usage:
  python -u emu_nantes_pier_flip_stress_test.py [--manage-emu] [host] [port]
"""
from __future__ import print_function

import os
import socket
import subprocess
import sys
import time

ERRGOTO_NONE = 0
ERRGOTO_SLEWING = 5

NANTES_LAT = ":St+47:13:00#"
NANTES_LON = ":Sg-001:33:00#"
TEST_MIN_ALT = 0
TEST_MAX_ALT = 85
ALT_TOL = 0.6

# HA magnitudes that sit outside a 60 min (~15 deg) meridian window → force pier change
HA_EAST_DEG = -40.0   # east of meridian → pier E (north hemisphere)
HA_WEST_DEG = 40.0    # west of meridian → pier W
DEC_DEG = 40.0

# Local hours to sweep (0..23 step 0.5 → 48 epochs)
HOUR_STEP = 0.5

SLEW_TIMEOUT_S = 90.0
SAMPLE_PERIOD_S = 0.06


def log(msg):
    print(msg)
    try:
        sys.stdout.flush()
    except Exception:
        pass


def send_recv(sock, cmd, wait=0.03):
    sock.sendall(cmd.encode("ascii"))
    time.sleep(wait)
    buf = b""
    deadline = time.time() + 2.0
    sock.settimeout(0.1)
    while time.time() < deadline:
        try:
            chunk = sock.recv(16384)
            if not chunk:
                break
            buf += chunk
            if b"#" in buf:
                break
            if len(buf) >= 1 and b"#" not in buf:
                sock.settimeout(0.04)
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


def send_noreply(sock, cmd, wait=0.1):
    try:
        sock.sendall(cmd.encode("ascii"))
    except Exception:
        pass
    time.sleep(wait)
    sock.settimeout(0.04)
    try:
        while sock.recv(4096):
            pass
    except Exception:
        pass


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
            time.sleep(0.2)
    raise OSError("connect failed: %s" % last)


def find_emu_exe():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.abspath(os.path.join(here, ".."))
    for c in (
        os.path.join(root, ".pio", "build", "emu", "mainunit_emu.exe"),
        os.path.join(root, ".pio", "build", "emu_mainunit", "mainunit_emu.exe"),
    ):
        if os.path.isfile(c):
            return c, root
    return None, root


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
        time.sleep(0.3)

    def start(self):
        self.stop()
        env = os.environ.copy()
        env.setdefault("SDL_VIDEODRIVER", "dummy")
        self.proc = subprocess.Popen(
            [self.exe], cwd=self.cwd, env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        time.sleep(1.0)

    def alive(self):
        return self.proc is not None and self.proc.poll() is None


def parse_dms(reply):
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
        d = float(parts[0])
        m = float(parts[1]) if len(parts) > 1 else 0.0
        sec = float(parts[2]) if len(parts) > 2 else 0.0
        return sign * (d + m / 60.0 + sec / 3600.0)
    except Exception:
        return None


def parse_hms_hours(reply):
    """Parse HH:MM:SS# or HHhMMmSS# into decimal hours."""
    s = reply.strip().rstrip("#")
    if not s:
        return None
    s = s.replace("h", ":").replace("m", ":").replace("s", "")
    parts = s.split(":")
    try:
        h = float(parts[0])
        m = float(parts[1]) if len(parts) > 1 else 0.0
        sec = float(parts[2]) if len(parts) > 2 else 0.0
        return h + m / 60.0 + sec / 3600.0
    except Exception:
        return None


def fmt_sr(ra_h):
    ra_h = ra_h % 24.0
    total = int(round(ra_h * 3600.0)) % (24 * 3600)
    hh = total // 3600
    mm = (total % 3600) // 60
    ss = total % 60
    return ":Sr%02d:%02d:%02d#" % (hh, mm, ss)


def fmt_sd(dec_deg):
    sign = "+" if dec_deg >= 0 else "-"
    a = abs(dec_deg)
    d = int(a)
    m = int(round((a - d) * 60.0))
    if m == 60:
        d += 1
        m = 0
    return ":Sd%s%02d*%02d:00#" % (sign, d, m)


def fmt_sl(hour_f):
    """Local time :SLHH:MM:SS# from decimal hours."""
    hour_f = hour_f % 24.0
    total = int(round(hour_f * 3600.0)) % (24 * 3600)
    hh = total // 3600
    mm = (total % 3600) // 60
    ss = total % 60
    return ":SL%02d:%02d:%02d#" % (hh, mm, ss)


def get_pier(sock):
    r = send_recv(sock, ":Gm#")
    return r[0] if r else "?"


def get_lst_hours(sock):
    r = send_recv(sock, ":GS#")
    return parse_hms_hours(r)


def is_slewing(sock):
    r = send_recv(sock, ":D#", wait=0.02)
    return bool(r) and ord(r[0]) == 0x7F


def abort_slew(sock):
    send_noreply(sock, ":Q#", wait=0.12)


def parse_errgoto(reply):
    if not reply:
        return None
    return ord(reply[0]) - ord("0")


def monitor_path(sock, tag, failed):
    t0 = time.time()
    samples = 0
    amin, amax = 1e9, -1e9
    saw = False
    idle = 0
    while time.time() - t0 < SLEW_TIMEOUT_S:
        alt = parse_dms(send_recv(sock, ":GA#", wait=0.015))
        if alt is not None:
            samples += 1
            amin = min(amin, alt)
            amax = max(amax, alt)
            if alt < TEST_MIN_ALT - ALT_TOL or alt > TEST_MAX_ALT + ALT_TOL:
                failed.append(
                    "%s DURING SLEW alt=%.2f outside [%d,%d] (n=%d t=%.1f)"
                    % (tag, alt, TEST_MIN_ALT, TEST_MAX_ALT, samples, time.time() - t0)
                )
                abort_slew(sock)
                return False, samples, amin, amax, time.time() - t0
        moving = is_slewing(sock)
        if moving:
            saw = True
            idle = 0
        else:
            idle += 1
            if saw and idle >= 2:
                return True, samples, amin, amax, time.time() - t0
            if (not saw) and samples > 0 and (time.time() - t0) > 0.35:
                return True, samples, amin, amax, time.time() - t0
        time.sleep(SAMPLE_PERIOD_S)
    failed.append("%s slew timeout (%.0fs, samples=%d)" % (tag, SLEW_TIMEOUT_S, samples))
    abort_slew(sock)
    return False, samples, amin, amax, time.time() - t0


def goto_ha_dec(sock, lst_h, ha_deg, dec_deg, tag, failed, expect_pier=None):
    """Goto equatorial target at given HA/Dec; monitor full path; check pier."""
    ra_h = (lst_h - ha_deg / 15.0) % 24.0
    send_recv(sock, fmt_sr(ra_h))
    send_recv(sock, fmt_sd(dec_deg))
    # Clear preferred pier so firmware chooses / flips as needed
    send_recv(sock, ":SmN#")
    code = parse_errgoto(send_recv(sock, ":MS#"))
    if code == ERRGOTO_SLEWING:
        abort_slew(sock)
        code = parse_errgoto(send_recv(sock, ":MS#"))
    if code != ERRGOTO_NONE:
        failed.append("%s :MS# failed code=%s HA=%.1f RA=%.3fh" % (tag, code, ha_deg, ra_h))
        return False, None
    ok, n, amin, amax, dt = monitor_path(sock, tag, failed)
    pier = get_pier(sock)
    if not ok:
        return False, pier
    if expect_pier and pier != expect_pier:
        failed.append("%s expected pier %s got %s (HA=%+.1f)" % (tag, expect_pier, pier, ha_deg))
        return False, pier
    log(
        "  OK %s HA=%+.1f pier=%s samples=%d alt=[%.1f,%.1f] %.1fs"
        % (tag, ha_deg, pier, n, amin, amax, dt)
    )
    return True, pier


def max_speed_setup(sock, failed):
    # Max slew, min accel distance, no settle
    r1 = send_recv(sock, ":SXRX,9999#")
    r2 = send_recv(sock, ":SXRA,1#")   # 0.1 deg accel
    r3 = send_recv(sock, ":SXOS,0#")
    r4 = send_recv(sock, ":SXLE,60#")
    r5 = send_recv(sock, ":SXLW,60#")
    log("  speed  SXRX=%r SXRA=%r SXOS=%r  meridian E/W 60 min" % (r1, r2, r3))
    if not (r1.startswith("1") and r2.startswith("1") and r3.startswith("1")):
        failed.append("max-speed setup failed RX=%r RA=%r OS=%r" % (r1, r2, r3))
    # Confirm rate via GXRX if present
    gx = send_recv(sock, ":GXRX#")
    log("  GXRX (max rate)=%r  LE=%r LW=%r" % (gx.strip(), r4, r5))


def ensure_gem(sock, emu, host, port, failed):
    """Force mount type GEM (:S!1#) and reconnect."""
    try:
        sock.sendall(b":S!1#")
        time.sleep(0.4)
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
    try:
        return connect(host, port, timeout=20.0)
    except OSError as e:
        failed.append("GEM restart failed: %s" % e)
        return None


def setup_site(sock, failed):
    send_recv(sock, ":hF#")
    time.sleep(0.08)
    if not send_recv(sock, NANTES_LAT).startswith("1") or not send_recv(sock, NANTES_LON).startswith("1"):
        failed.append("Nantes site set failed")
    send_recv(sock, ":Sh0#")
    send_recv(sock, ":So85#")
    send_recv(sock, ":SC03/20/26#")  # fixed date for LST stability with SL sweep
    send_recv(sock, ":hR#")
    send_noreply(sock, ":Te#", wait=0.05)
    gt = send_recv(sock, ":Gt#")
    gg = send_recv(sock, ":Gg#")
    log("  site Gt=%r Gg=%r" % (gt.strip(), gg.strip()))


def parse_args(argv):
    manage = "--manage-emu" in argv
    args = [a for a in argv if a != "--manage-emu"]
    host = args[0] if len(args) > 0 else "127.0.0.1"
    port = int(args[1]) if len(args) > 1 else 9997
    return manage, host, port


def main():
    manage, host, port = parse_args(sys.argv[1:])
    failed = []
    flips = 0
    path_ok = 0
    epochs = 0

    emu = None
    exe, root = find_emu_exe()
    if manage:
        if not exe:
            log("ERROR: mainunit_emu.exe not found")
            return 2
        emu = EmuProcess(exe, root)
        log("Managing emulator: %s" % exe)
        emu.start()

    try:
        sock = connect(host, port)
    except OSError as e:
        log("ERROR: %s" % e)
        if emu:
            emu.stop()
        return 3

    log("Connected %s:%s — pier-flip stress @ Nantes, max slew\n" % (host, port))
    try:
        sock = ensure_gem(sock, emu, host, port, failed)
        if sock is None:
            return 1
        time.sleep(0.4)
        setup_site(sock, failed)
        max_speed_setup(sock, failed)

        hours = []
        h = 0.0
        while h < 24.0 - 1e-9:
            hours.append(h)
            h += HOUR_STEP

        log("  sweeping %d local times, HA %+g / %+g deg, Dec %+g\n"
            % (len(hours), HA_EAST_DEG, HA_WEST_DEG, DEC_DEG))

        prev_pier = None
        for hour in hours:
            epochs += 1
            send_recv(sock, fmt_sl(hour))
            lst = get_lst_hours(sock)
            if lst is None:
                failed.append("hour=%.1f: could not read :GS#" % hour)
                continue
            gl = send_recv(sock, ":GL#")
            tag_base = "H%04.1f LST=%.3f" % (hour, lst)
            log("--- local %s (GL=%s) ---" % (tag_base, gl.strip()))

            # Sky east of meridian (HA<0): TeenAstro reports pier W (POLE_OVER) in north hemisphere.
            # Sky west of meridian (HA>0): pier E (POLE_UNDER).
            ok_e, pier_e = goto_ha_dec(
                sock, lst, HA_EAST_DEG, DEC_DEG,
                tag_base + " EAST", failed, expect_pier="W",
            )
            if ok_e:
                path_ok += 1
            if prev_pier and pier_e and pier_e != prev_pier:
                flips += 1
            prev_pier = pier_e

            # Refresh LST after slew (clock advances)
            lst2 = get_lst_hours(sock)
            if lst2 is None:
                lst2 = lst

            ok_w, pier_w = goto_ha_dec(
                sock, lst2, HA_WEST_DEG, DEC_DEG,
                tag_base + " WEST", failed, expect_pier="E",
            )
            if ok_w:
                path_ok += 1
            if pier_e and pier_w and pier_e != pier_w:
                flips += 1
                log("  FLIP %s->%s at local hour %.1f" % (pier_e, pier_w, hour))
            elif ok_e and ok_w and pier_e == pier_w:
                failed.append(
                    "hour=%.1f: no pier change EAST-target pier=%s WEST-target pier=%s"
                    % (hour, pier_e, pier_w)
                )
            prev_pier = pier_w

            # Explicit :MF# periodically when on pier E after west target
            if int(hour * 2) % 6 == 0 and pier_w == "E":
                code = parse_errgoto(send_recv(sock, ":MF#"))
                if code == ERRGOTO_NONE:
                    ok_f, n, amin, amax, dt = monitor_path(sock, tag_base + " MF", failed)
                    pier_f = get_pier(sock)
                    if ok_f:
                        path_ok += 1
                        if pier_w and pier_f and pier_f != pier_w:
                            flips += 1
                        log(
                            "  OK MF %s->%s samples=%d alt=[%.1f,%.1f] %.1fs"
                            % (pier_w, pier_f, n, amin, amax, dt)
                        )
                    prev_pier = pier_f
                else:
                    log("  :MF# skipped/failed code=%s" % code)

    finally:
        try:
            sock.close()
        except Exception:
            pass
        if emu:
            emu.stop()

    log(
        "\nSummary: epochs=%d path_ok=%d pier_flips=%d failures=%d"
        % (epochs, path_ok, flips, len(failed))
    )
    if failed:
        log("FAILED (%d):" % len(failed))
        for f in failed[:40]:
            log("  - %s" % f)
        if len(failed) > 40:
            log("  ... +%d more" % (len(failed) - 40))
        return 1
    log("ALL PASSED — max-speed pier flips at many hours, paths within limits.")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
