#!/usr/bin/env python3
"""Emulator vs PC alignment identity harness.

Runs the MainUnit emulator through the planned session matrix, dumps :GXAo
observations, replays them with pointing_fit (twostar / stored), and stops on
the first mismatch. :AW# is never sent.

Usage:
  py -3 -u tests/pointing_crosscheck/align_parity.py [--manage-emu] [host] [port]
"""
from __future__ import print_function

import math
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.dirname(__file__))
from tpoint_eq import DEG, Terms, mount_for_true  # noqa: E402

HOST = "127.0.0.1"
PORT = 9997
MANAGE_EMU = False
OUT_DIR = os.path.join(ROOT, "tests", "pointing_crosscheck", "out")
EXE = os.path.join(ROOT, "tests", "pointing_crosscheck", "pointing_fit.exe")
EMU_EXE = os.path.join(ROOT, "TeenAstroEmulator", ".pio", "build", "emu", "mainunit_emu.exe")

LAT = 47.0
TERMS = Terms(ih=0.0, id_=0.7 * DEG / 60, ch=3 * DEG / 60,
              np_=-1.5 * DEG / 60, me=3 * DEG / 60, ma=2 * DEG / 60)

# Head / RMS within 0.01"; ME/MA / W within 1".
TOL_HEAD = 0.01
TOL_RMS = 0.01
TOL_POLE_AS = 1.0

CATALOG = [
    ("Deneb", 20 + 41 / 60 + 26 / 3600, 45 + 16 / 60),
    ("Altair", 19 + 50 / 60 + 47 / 3600, 8 + 52 / 60),
    ("Albireo", 19 + 30 / 60 + 43 / 3600, 27 + 58 / 60),
    ("Arcturus", 14 + 15 / 60 + 40 / 3600, 19 + 11 / 60),
    ("Alkaid", 13 + 47 / 60 + 32 / 3600, 49 + 19 / 60),
    ("Mizar", 13 + 23 / 60 + 56 / 3600, 54 + 55 / 60),
    ("Alphecca", 15 + 34 / 60 + 41 / 3600, 26 + 43 / 60),
    ("Rasalhague", 17 + 34 / 60 + 56 / 3600, 12 + 34 / 60),
    ("Sulafat", 18 + 58 / 60 + 56 / 3600, 32 + 41 / 60),
    ("Etamin", 17 + 56 / 60 + 36 / 3600, 51 + 29 / 60),
]


def log(msg):
    print(msg, flush=True)


def precess(ra_h, dec_d, years=26.8):
    ra = math.radians(ra_h * 15.0)
    dec = math.radians(dec_d)
    dra = (3.07496 + 1.33621 * math.sin(ra) * math.tan(dec)) * years
    ddec = 20.0431 * math.cos(ra) * years
    return ra_h + dra / 3600.0, dec_d + ddec / 3600.0


def alt_deg(ha_h, dec_d, lat_d):
    ha = math.radians(ha_h * 15.0)
    dec = math.radians(dec_d)
    lat = math.radians(lat_d)
    s = math.sin(lat) * math.sin(dec) + math.cos(lat) * math.cos(dec) * math.cos(ha)
    return math.degrees(math.asin(max(-1.0, min(1.0, s))))


def dms_to_deg(s):
    s = (s or "").strip().rstrip("#")
    if not s:
        return 0.0
    sign = -1.0 if s[0] == "-" else 1.0
    s = s.lstrip("+-").replace("*", ":").replace("'", ":")
    parts = s.split(":")
    d = float(parts[0])
    m = float(parts[1]) if len(parts) > 1 else 0.0
    sec = float(parts[2]) if len(parts) > 2 else 0.0
    return sign * (abs(d) + m / 60.0 + sec / 3600.0)


class Link(object):
    def __init__(self, host, port):
        self.s = socket.create_connection((host, port), 8)

    def close(self):
        try:
            self.s.close()
        except Exception:
            pass

    def raw(self, cmd, wait=2.0):
        self.s.settimeout(0.01)
        try:
            while True:
                junk = self.s.recv(8192)
                if not junk:
                    break
        except socket.timeout:
            pass
        self.s.sendall(cmd.encode("ascii"))
        buf = b""
        end = time.time() + wait
        while time.time() < end:
            self.s.settimeout(0.1)
            try:
                chunk = self.s.recv(8192)
                if not chunk:
                    break
                buf += chunk
                if b"#" in buf:
                    break
            except socket.timeout:
                if buf:
                    break
        return buf

    def gv(self, cmd, wait=2.0):
        return self.raw(cmd, wait).split(b"#", 1)[0].decode("ascii", "replace").strip("\x00")

    def digit(self, cmd, wait=3.0):
        for b in self.raw(cmd, wait):
            if 48 <= b <= 57:
                return chr(b)
        return ""

    def moving(self):
        return b"\x7f" in self.raw(":D#", 1.0)

    def wait_still(self, limit=180):
        t0 = time.time()
        last1 = last2 = None
        idle = 0
        while time.time() - t0 < limit:
            busy = self.moving()
            try:
                a1 = dms_to_deg(self.gv(":GXP1#"))
                a2 = dms_to_deg(self.gv(":GXP2#"))
            except Exception:
                a1 = a2 = None
            moved = False
            if a1 is not None and last1 is not None:
                moved = abs(a1 - last1) > 0.02 or abs(a2 - last2) > 0.02
            if not busy and not moved:
                idle += 1
                if idle >= 3:
                    return True
            else:
                idle = 0
            last1, last2 = a1, a2
            time.sleep(0.2)
        return False


class EmuProcess(object):
    def __init__(self):
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
        cwd = os.path.dirname(EMU_EXE)
        self.proc = subprocess.Popen(
            [EMU_EXE], cwd=cwd, env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        time.sleep(1.2)


def rebuild_tools():
    gpp = os.path.join(os.path.expanduser("~"),
                       ".platformio", "packages", "toolchain-gccmingw32", "bin", "g++.exe")
    cmd = [
        gpp, "-std=c++14", "-O2", "-D_USE_MATH_DEFINES", "-DNATIVE_HAL_BUILD",
        "-I" + os.path.join(ROOT, "TeenAstroEmulator", "shim"),
        "-I" + os.path.join(ROOT, "TeenAstroMainUnit"),
        "-I" + os.path.join(ROOT, "libraries", "TeenAstroLA3"),
        "-I" + os.path.join(ROOT, "libraries", "svd3"),
        "-I" + os.path.join(ROOT, "libraries", "TeenAstroCoord"),
        "-I" + os.path.join(ROOT, "libraries", "TeenAstroCoordConv"),
        "-I" + os.path.join(ROOT, "libraries", "TeenAstroMath", "src"),
        "-o", EXE,
        os.path.join(ROOT, "tests", "pointing_crosscheck", "pointing_fit.cpp"),
    ]
    log("rebuild pointing_fit")
    subprocess.check_call(cmd)
    log("rebuild emulator")
    pio = os.path.join(os.path.expanduser("~"), ".platformio", "penv", "Scripts", "pio.exe")
    # Unity build may not notice MainUnit .cpp edits; force a recompile.
    main_o = os.path.join(ROOT, "TeenAstroEmulator", ".pio", "build", "emu", "src", "emu_mainunit_main.o")
    if os.path.isfile(main_o):
        os.remove(main_o)
    subprocess.check_call([pio, "run", "-d", "TeenAstroEmulator", "-e", "emu_mainunit"], cwd=ROOT)


def setup_site(link):
    link.digit(":SG-01.0#")
    link.digit(":St+47*00:00#")
    link.digit(":Sg-001*00:00#")
    link.digit(":SC10/02/26#")
    link.digit(":SL17:40:00#")
    # Fast slew / short settle for the matrix (SXRX is an integer rate code).
    link.digit(":SXRX,9999#")
    link.digit(":SXRA,1#")
    link.digit(":SXOS,0#")


def plan_stars(link, n, need_3plus3=False):
    lat = float(link.gv(":Gt#").split("*")[0])
    lst0 = float(link.gv(":GSL#"))
    sky = []
    for name, ra_h, dec_d in CATALOG:
        ra_h, dec_d = precess(ra_h, dec_d)
        ha = lst0 - ra_h
        while ha > 12:
            ha -= 24
        while ha < -12:
            ha += 24
        alt = alt_deg(ha, dec_d, lat)
        if not (20 <= alt <= 75 and 0.8 <= abs(ha) <= 5.8):
            continue
        sky.append((name, ra_h * 15.0, dec_d, ha, alt))
    west = [s for s in sky if s[3] < 0]
    east = [s for s in sky if s[3] > 0]
    if need_3plus3:
        if len(west) < 3 or len(east) < 3:
            raise RuntimeError("not enough pier sides for 3+3: W=%d E=%d" % (len(west), len(east)))
        # Interleave so axis2 lands on both sides of the pole (GXAb → 3,3).
        # A west-then-east block can leave four stars on one mechanical side.
        return [west[0], east[0], west[1], east[1], west[2], east[2]]
    # Prefer alternating pier when possible, else fill from available sky.
    plan = []
    wi, ei = 0, 0
    while len(plan) < n and (wi < len(west) or ei < len(east)):
        if len(plan) % 2 == 0 and wi < len(west):
            plan.append(west[wi]); wi += 1
        elif ei < len(east):
            plan.append(east[ei]); ei += 1
        elif wi < len(west):
            plan.append(west[wi]); wi += 1
        else:
            break
    if len(plan) < n:
        raise RuntimeError("only %d stars available, need %d" % (len(plan), n))
    return plan[:n]


def goto(link, ra_deg, dec_deg):
    time.sleep(0.15)
    a = link.digit(":SrL%.6f#" % (ra_deg % 360.0))
    b = link.digit(":SdL%+.6f#" % dec_deg)
    if a != "1" or b != "1":
        return "S"
    for _ in range(12):
        code = link.digit(":MS#")
        if code == "5":
            time.sleep(0.25)
            continue
        return code
    return "5"


def go_home(link):
    link.digit(":hC#")
    t0 = time.time()
    while time.time() - t0 < 120:
        gw = link.gv(":GW#")
        if len(gw) >= 3 and gw[2] == "H":
            return True
        time.sleep(0.25)
    return False


def _read_T(link):
    return [float(link.gv(":GXA%d#" % i) or "0") for i in range(9)]


def _wallace_instr_goto(link, ra_deg, dec_deg, pier, terms):
    """Plumbed ME/MA + HeadGeom CH/NP/ID dials under the live T+head."""
    lst = float(link.gv(":GSL#"))
    ha_deg = math.remainder(lst * 15.0 - ra_deg, 360.0)
    flip = 1 if pier == "W" else 0
    T = _read_T(link)
    ch = float(link.gv(":GXAc#") or "0")
    np_ = float(link.gv(":GXAp#") or "0")
    id_ = float(link.gv(":GXAi#") or "0")
    am = terms.as_arcmin()
    lat = float(link.gv(":Gt#").split("*")[0])
    body = (
        "plumbedgoto\n%g\n%g\n%s\n%g %g %g %g %g %d\n%g %g %g %g %g\n"
    ) % (
        lat, lst, " ".join("%.10f" % x for x in T),
        ch, np_, id_, ha_deg, dec_deg, flip,
        am["ME"], am["MA"], am["CH"], am["NP"], am["ID"],
    )
    p = subprocess.run([EXE], input=body, text=True, capture_output=True, cwd=ROOT)
    if p.returncode != 0:
        raise RuntimeError("plumbedgoto failed: %s%s" % (p.stdout, p.stderr))
    ra_t = dec_t = None
    for line in p.stdout.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] == "ra":
            ra_t = float(parts[1])
        if len(parts) == 2 and parts[0] == "dec":
            dec_t = float(parts[1])
    if ra_t is None or dec_t is None:
        raise RuntimeError("plumbedgoto parse: %s" % p.stdout)
    return goto(link, ra_t, dec_t)


def accept_star(link, i, name, ra, dec, apply_wallace):
    code = goto(link, ra, dec)
    if code != "0" or not link.wait_still():
        raise RuntimeError("slew failed for %s (%s)" % (name, code))
    pier = link.gv(":Gm#")[:1]
    if apply_wallace and i > 1:
        code = _wallace_instr_goto(link, ra, dec, pier, TERMS)
        if code != "0" or not link.wait_still():
            raise RuntimeError("offset slew failed for %s" % name)
        if link.digit(":SrL%.6f#" % ra) != "1" or link.digit(":SdL%+.6f#" % dec) != "1":
            raise RuntimeError("restore target failed for %s" % name)
    ack = link.digit(":A%d#" % i, wait=5.0)
    if ack != "1":
        raise RuntimeError(":A%d# rejected for %s (%r)" % (i, name, ack))
    return pier


def dump_gxa(link):
    n = int(link.gv(":GXAn#") or "0")
    stars = []
    for i in range(1, n + 1):
        line = link.gv(":GXAo,%d#" % i)
        parts = [float(x) for x in line.split(",")]
        if len(parts) != 4:
            raise RuntimeError("bad GXAo,%d: %r" % (i, line))
        stars.append(tuple(parts))
    terms = {
        "CH": float(link.gv(":GXAc#") or "0"),
        "NP": float(link.gv(":GXAp#") or "0"),
        "ID": float(link.gv(":GXAi#") or "0"),
        "ME": dms_to_deg(link.gv(":GXAa#")),
        "MA": dms_to_deg(link.gv(":GXAz#")),
        "W": dms_to_deg(link.gv(":GXAw#")),
        "RMS": float(link.gv(":GXAr#") or "0"),
        "mask": link.gv(":GXAf#"),
        "sides": link.gv(":GXAb#"),
        "n": n,
    }
    return stars, terms


def pc_replay(lat, stars, mode):
    body = "%d\n" % len(stars)
    for az, alt, a1, a2 in stars:
        body += "%.10f %.10f %.10f %.10f\n" % (az, alt, a1, a2)
    inp = "%s\n%g\n%s" % (mode, lat, body)
    p = subprocess.run([EXE], input=inp, text=True, capture_output=True, cwd=ROOT)
    if p.returncode != 0:
        raise RuntimeError("pointing_fit failed:\n%s\n%s" % (p.stdout, p.stderr))
    tag = "twostar" if mode == "twostar" else "batch"
    out = {}
    for line in p.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0] == tag:
            key = parts[1]
            if key == "ok":
                out["ok"] = parts[2]
            elif key == "mask":
                out["mask"] = int(parts[2])
            else:
                out[key] = float(parts[2])
    out["_raw"] = p.stdout
    return out


def mask_to_str(m):
    if isinstance(m, str):
        return m
    return "%s%s%s" % (
        "C" if m & 1 else "-",
        "P" if m & 2 else "-",
        "I" if m & 4 else "-",
    )


def compare(mount, pc, case_name):
    errs = []
    for k in ("CH", "NP", "ID"):
        a = mount.get(k, 0.0)
        b = pc.get(k, 0.0)
        if abs(a - b) > TOL_HEAD:
            errs.append("%s mount=%.6f pc=%.6f" % (k, a, b))
    for k in ("ME", "MA", "W"):
        a = mount.get(k, 0.0) * 3600.0
        b = pc.get(k, 0.0) * 3600.0
        if abs(a - b) > TOL_POLE_AS:
            errs.append("%s mount=%.3f\" pc=%.3f\"" % (k, a, b))
    a = mount.get("RMS", 0.0)
    b = pc.get("RMS", 0.0)
    # Two-star path leaves mount RMS at 0; compare only when either side fitted.
    if max(abs(a), abs(b)) > 1e-6 and abs(a - b) > TOL_RMS:
        errs.append("RMS mount=%.6f pc=%.6f" % (a, b))
    mm = mount.get("mask", "---")
    pm = mask_to_str(pc.get("mask", 0))
    # Ignore mask when both heads are essentially zero (two-star close).
    if max(abs(mount.get("CH", 0)), abs(mount.get("NP", 0)), abs(mount.get("ID", 0)),
           abs(pc.get("CH", 0)), abs(pc.get("NP", 0)), abs(pc.get("ID", 0))) > TOL_HEAD:
        if mm != pm:
            errs.append("mask mount=%s pc=%s" % (mm, pm))
    return errs


def run_case(link, case):
    name = case["name"]
    n = case["n"]
    start = case["start"]
    need33 = case.get("need_3plus3", False)
    log("\n=== CASE %s  start=%s n=%d ===" % (name, start, n))
    setup_site(link)
    if not go_home(link):
        raise RuntimeError("not at home before %s" % name)
    plan = plan_stars(link, n, need_3plus3=need33)
    log("plan: " + ", ".join("%s %+.2fh" % (s[0], s[3]) for s in plan))
    ack = link.digit(start)
    if ack != "1":
        raise RuntimeError("%s rejected (%r)" % (start, ack))
    # :A0 clears the model and syncs at home; give the ISR a beat, then continue.
    time.sleep(0.5)
    for i, (sname, ra, dec, ha, alt) in enumerate(plan, 1):
        pier = accept_star(link, i, sname, ra, dec, apply_wallace=True)
        log("  star %d %s pier %s ME %s" % (i, sname, pier, link.gv(":GXAa#")))
    stars, mount = dump_gxa(link)
    mode = "twostar" if n == 2 else "stored"
    pc = pc_replay(LAT, stars, mode)
    path = os.path.join(OUT_DIR, "parity_%s.txt" % name)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("case %s\nstart %s\n" % (name, start))
        fh.write("mount %s\n" % mount)
        fh.write("pc %s\n" % {k: v for k, v in pc.items() if k != "_raw"})
        fh.write("gxa_o\n")
        for i, row in enumerate(stars, 1):
            fh.write("%d %.10f %.10f %.10f %.10f\n" % (i, row[0], row[1], row[2], row[3]))
        fh.write("pc_raw\n%s\n" % pc.get("_raw", ""))
    log("mount CH %.3f NP %.3f ID %.3f ME %.4f MA %.4f RMS %.3f mask %s sides %s" % (
        mount["CH"], mount["NP"], mount["ID"], mount["ME"], mount["MA"],
        mount["RMS"], mount["mask"], mount["sides"]))
    log("pc    CH %.3f NP %.3f ID %.3f ME %.4f MA %.4f RMS %.3f mask %s" % (
        pc.get("CH", 0), pc.get("NP", 0), pc.get("ID", 0),
        pc.get("ME", 0), pc.get("MA", 0), pc.get("RMS", 0),
        mask_to_str(pc.get("mask", 0))))
    errs = compare(mount, pc, name)
    if need33 and mount.get("sides") != "3,3":
        errs.append("sides want 3,3 got %s" % mount.get("sides"))
    go_home(link)
    if errs:
        log("FAIL %s:" % name)
        for e in errs:
            log("  " + e)
        return False
    log("PASS %s" % name)
    return True


CASES = [
    {"name": "2exact", "start": ":A0,2#", "n": 2},
    {"name": "2redund3", "start": ":A0,r3#", "n": 3},
    {"name": "2redund4", "start": ":A0,r4#", "n": 4},
    {"name": "2redund5", "start": ":A0,r5#", "n": 5},
    {"name": "2redund6", "start": ":A0,r6#", "n": 6},
    {"name": "4exact", "start": ":A0,r4#", "n": 4},
    {"name": "4redund5", "start": ":A0,r5#", "n": 5},
    {"name": "4redund6", "start": ":A0,r6#", "n": 6},
    {"name": "3plus3", "start": ":A0,r6#", "n": 6, "need_3plus3": True},
]


def main(argv):
    global HOST, PORT, MANAGE_EMU
    args = list(argv[1:])
    if "--manage-emu" in args:
        MANAGE_EMU = True
        args.remove("--manage-emu")
    if args:
        HOST = args[0]
    if len(args) > 1:
        PORT = int(args[1])

    os.makedirs(OUT_DIR, exist_ok=True)
    rebuild_tools()
    emu = EmuProcess()
    if MANAGE_EMU:
        emu.start()
    else:
        # Restart so the freshly built binary is used.
        for p in subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq mainunit_emu.exe"],
            capture_output=True, text=True
        ).stdout.splitlines():
            if "mainunit_emu.exe" in p.lower() or "mainunit_emu.exe" in p:
                subprocess.run(["taskkill", "/IM", "mainunit_emu.exe", "/F"],
                               capture_output=True)
                time.sleep(0.5)
                break
        emu.start()

    link = None
    try:
        for _ in range(30):
            try:
                link = Link(HOST, PORT)
                break
            except OSError:
                time.sleep(0.3)
        if link is None:
            raise RuntimeError("cannot connect to %s:%d" % (HOST, PORT))
        log("connected version %s" % link.gv(":GVN#"))
        for case in CASES:
            if not run_case(link, case):
                return 1
        log("\nALL CASES PASSED")
        return 0
    finally:
        if link is not None:
            try:
                link.digit(":Q#")
                link.digit(":AB#")
                go_home(link)
            except Exception:
                pass
            link.close()
        emu.stop()


if __name__ == "__main__":
    sys.exit(main(sys.argv) or 0)
