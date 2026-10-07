"""Six-star Wallace alignment against mainunit_emu on TCP 9997.

Same procedure as the hardware capture: catalog targets stay true; from star 2
on the axes are left at Wallace's dial; motor angles are read when each star
is stored. :AW# is not sent.
"""
from __future__ import print_function

import math
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tests", "pointing_crosscheck"))
from tpoint_eq import DEG, Terms, mount_for_true  # noqa: E402

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 9997
OUT = os.path.join(ROOT, "tests", "pointing_crosscheck", "emu_obs.txt")
EXE = os.path.join(ROOT, "tests", "pointing_crosscheck", "pointing_fit.exe")

CATALOG = [
    ("Vega", 18 + 36 / 60 + 56 / 3600, 38 + 47 / 60),
    ("Deneb", 20 + 41 / 60 + 26 / 3600, 45 + 16 / 60),
    ("Altair", 19 + 50 / 60 + 47 / 3600, 8 + 52 / 60),
    ("Rasalhague", 17 + 34 / 60 + 56 / 3600, 12 + 34 / 60),
    ("Arcturus", 14 + 15 / 60 + 40 / 3600, 19 + 11 / 60),
    ("Alkaid", 13 + 47 / 60 + 32 / 3600, 49 + 19 / 60),
    ("Mizar", 13 + 23 / 60 + 56 / 3600, 54 + 55 / 60),
    ("Dubhe", 11 + 3 / 60 + 44 / 3600, 61 + 45 / 60),
    ("Alphecca", 15 + 34 / 60 + 41 / 3600, 26 + 43 / 60),
    ("Alioth", 12 + 54 / 60 + 2 / 3600, 55 + 58 / 60),
    ("Spica", 13 + 25 / 60 + 12 / 3600, -(11 + 9 / 60)),
    ("Etamin", 17 + 56 / 60 + 36 / 3600, 51 + 29 / 60),
    ("Albireo", 19 + 30 / 60 + 43 / 3600, 27 + 58 / 60),
    ("Sulafat", 18 + 58 / 60 + 56 / 3600, 32 + 41 / 60),
]
TERMS = Terms(ih=0.0, id_=0.7 * DEG / 60, ch=3 * DEG / 60,
              np_=-1.5 * DEG / 60, me=3 * DEG / 60, ma=2 * DEG / 60)


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


def dms(s):
    s = s.strip()
    sign = -1 if s[0] == "-" else 1
    s = s.lstrip("+-")
    d, rest = s.split("*")
    m, sec = rest.split(":")
    return sign * (abs(float(d)) + float(m) / 60 + float(sec) / 3600)


class Link(object):
    def __init__(self, host, port):
        self.s = socket.create_connection((host, port), 5)

    def close(self):
        self.s.close()

    def raw(self, cmd, wait=2.0):
        # Drop any leftover from a previous reply before sending.
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

    def gv(self, cmd):
        return self.raw(cmd).split(b"#", 1)[0].decode("ascii", "replace").strip("\x00")

    def digit(self, cmd):
        for b in self.raw(cmd, 3.0):
            if 48 <= b <= 57:
                return chr(b)
        return ""

    def moving(self):
        return b"\x7f" in self.raw(":D#", 1.0)

    def wait_still(self, limit=120):
        t0 = time.time()
        seen = False
        while time.time() - t0 < limit:
            if self.moving():
                seen = True
                time.sleep(0.05)
                continue
            if seen or time.time() - t0 > 0.4:
                return True
            time.sleep(0.05)
        return False


def goto(link, ra_deg, dec_deg):
    a = link.digit(":SrL%.6f#" % (ra_deg % 360.0))
    b = link.digit(":SdL%+.6f#" % dec_deg)
    if a != "1" or b != "1":
        print("  target refused", a, b, flush=True)
        return ""
    return link.digit(":MS#")


def read_T(link):
    return [float(link.gv(":GXA%d#" % i) or "0") for i in range(9)]


def read_head_as(link):
    # Wallace signs from the mount (CH, NP, ID arcseconds).
    return (float(link.gv(":GXAc#") or "0"),
            float(link.gv(":GXAp#") or "0"),
            float(link.gv(":GXAi#") or "0"))


def wallace_instr_goto(link, ra_deg, dec_deg, pier, terms):
    """Goto that parks encoders on plumbed ME/MA + HeadGeom CH/NP/ID dials.

    ME/MA use the plumbed pole transfer (setPoleError), not the tan/sec MA
    formula. Catalog sky stays true; the slew target is the sky the live
    T+head associates with those dial axes.
    """
    lst = float(link.gv(":GSL#"))
    ha_deg = math.remainder(lst * 15.0 - ra_deg, 360.0)
    flip = 1 if pier == "W" else 0
    T = read_T(link)
    ch, np_, id_ = read_head_as(link)
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
        raise RuntimeError("plumbedgoto parse failed: %s" % p.stdout)
    return goto(link, ra_t, dec_t), ra_t, dec_t


def go_home(link):
    print("home", link.digit(":hC#"), flush=True)
    t0 = time.time()
    while time.time() - t0 < 120:
        gw = link.gv(":GW#")
        print("  %.0fs" % (time.time() - t0), gw, link.gv(":GXP1#"), link.gv(":GXP2#"),
              link.gv(":GD#"), flush=True)
        if len(gw) >= 3 and gw[2] == "H":
            return True
        time.sleep(0.5)
    return False


def compare_pc(rows, lat):
    body = "\n".join(
        "%.8f %.8f %.8f %.8f %.8f" % (ra, dec, lst, dms(a1), dms(a2))
        for _n, ra, dec, lst, a1, a2, _p in rows
    ) + "\n"
    p = subprocess.run([EXE], input="raw\n%g\n%d\n%s" % (lat, len(rows), body),
                       text=True, capture_output=True, cwd=ROOT)
    print("\n== PC fit of emulator axes ==", flush=True)
    print(p.stdout, end="", flush=True)
    if p.returncode:
        print(p.stderr, flush=True)
        return

    terms = TERMS
    pc_rows = []
    for name, ra, dec_d, lst, a1, a2, pier in rows:
        ha = math.remainder(lst * 15.0 - ra, 360.0)
        flip = 1 if pier == "W" else 0
        side = -1.0 if flip else 1.0
        sided = Terms(ih=0, id_=terms.id_ * side, ch=terms.ch * side,
                      np_=terms.np_ * side, me=terms.me, ma=terms.ma)
        mha, mdec = mount_for_true(ha * DEG, dec_d * DEG, sided)
        pc_rows.append("%.8f %.8f %.8f %.8f %.8f %.8f %d" % (
            ha, dec_d, ha, dec_d, mha / DEG, mdec / DEG, flip))
    text = "\n".join(pc_rows) + "\n"

    def grab(cmd):
        q = subprocess.run([EXE], input=cmd, text=True, capture_output=True, cwd=ROOT)
        if q.returncode:
            raise SystemExit(q.stderr or q.stdout)
        keep = []
        for line in q.stdout.splitlines():
            if line.startswith(("rms", "cone", "perp", "idx2", "eq_el", "eq_az", "ok")):
                keep.append(line)
        return keep

    print("== pure PC one-shot ==", flush=True)
    for line in grab("fit\n%g\n%d\n%s" % (lat, len(rows), text)):
        print(line, flush=True)
    print("== pure PC session ==", flush=True)
    for line in grab("session\n%g\n0 0\n%d\n%s" % (lat, len(rows), text)):
        print(line, flush=True)


def main():
    link = Link(HOST, PORT)
    rows = []
    try:
        gw = link.gv(":GW#")
        print("GW", gw, "version", link.gv(":GVN#"), "dec", link.gv(":GD#"),
              "ME", link.gv(":GXAa#"), flush=True)
        if len(gw) < 3 or gw[2] != "H":
            print("not at home, refusing to start", flush=True)
            return 1
        # Match the hardware site and evening LST (~17.5 h) from the last live run.
        print("tz", link.digit(":SG-01.0#"),
              "lat", link.digit(":St+47*00:00#"),
              "lon", link.digit(":Sg-001*00:00#"), flush=True)
        print("date", link.digit(":SC10/02/26#"),
              "time", link.digit(":SL17:40:00#"), flush=True)
        lat = float(link.gv(":Gt#").split("*")[0])
        lst0 = float(link.gv(":GSL#"))
        print("lat", lat, "LST", "%.4f" % lst0, flush=True)

        sky = []
        for name, ra_h, dec_d in CATALOG:
            ra_h, dec_d = precess(ra_h, dec_d)
            ha = lst0 - ra_h
            while ha > 12:
                ha -= 24
            while ha < -12:
                ha += 24
            if abs(dec_d) > 80:
                continue
            alt = alt_deg(ha, dec_d, lat)
            if not (25 <= alt <= 70 and 1.0 <= abs(ha) <= 5.5):
                continue
            sky.append((name, ra_h * 15.0, dec_d, ha, alt))
        west = [s for s in sky if s[3] < 0][:3]
        east = [s for s in sky if s[3] > 0][:3]
        plan = west + east
        if len(west) < 3 or len(east) < 3:
            print("not enough stars", len(west), len(east), flush=True)
            for s in sky:
                print(" ", s[0], "%+.2fh" % s[3], "alt %.1f" % s[4])
            return 1
        print("plan", [(s[0], "%+.2fh" % s[3]) for s in plan], flush=True)
        print("A0", link.digit(":A0,r6#"),
              "ME", link.gv(":GXAa#"), "MA", link.gv(":GXAz#"), "W", link.gv(":GXAw#"),
              "dec", link.gv(":GD#"), flush=True)

        for i, (name, ra, dec, ha, alt) in enumerate(plan, 1):
            print("star %d %s true RA %.4f Dec %.4f" % (i, name, ra / 15.0, dec), flush=True)
            code = goto(link, ra, dec)
            print("  goto", code, flush=True)
            if code != "0" or not link.wait_still():
                print("  slew failed", flush=True)
                link.digit(":Q#")
                link.digit(":AB#")
                go_home(link)
                return 1
            pier = link.gv(":Gm#")[:1]
            print("  on star RA", link.gv(":GR#"), "Dec", link.gv(":GD#"), "pier", pier, flush=True)
            if i == 1:
                lst = float(link.gv(":GSL#"))
                ack = link.digit(":A1#")
                a1 = link.gv(":GXP1#")
                a2 = link.gv(":GXP2#")
                print("  A1", ack, "axes", a1, a2, flush=True)
            else:
                side = -1.0 if pier == "W" else 1.0
                code, ra_t, dec_t = wallace_instr_goto(link, ra, dec, pier, TERMS)
                print("  instr offset goto", code, "sign", "%+.0f" % side,
                      "sky", "%.4f" % (ra_t / 15.0), "%+.4f" % dec_t, flush=True)
                if code != "0" or not link.wait_still():
                    print("  offset slew failed", flush=True)
                    link.digit(":Q#")
                    link.digit(":AB#")
                    go_home(link)
                    return 1
                if link.digit(":SrL%.6f#" % ra) != "1" or link.digit(":SdL%+.6f#" % dec) != "1":
                    print("  restore target failed", flush=True)
                    link.digit(":AB#")
                    go_home(link)
                    return 1
                lst = float(link.gv(":GSL#"))
                a1 = link.gv(":GXP1#")
                a2 = link.gv(":GXP2#")
                ack = link.digit(":A%d#" % i)
                print("  A%d" % i, ack, "axes", a1, a2, "pier", link.gv(":Gm#"), flush=True)
            print("  ME", link.gv(":GXAa#"), "stars", link.gv(":GXAn#"), flush=True)
            rows.append((name, ra, dec, lst, a1, a2, pier))
            if ack != "1":
                print("  accept failed", flush=True)
                link.digit(":AB#")
                go_home(link)
                return 1

        print("AE", link.gv(":AE#"), "GW", link.gv(":GW#"), flush=True)
        for cmd in (":GXAc#", ":GXAp#", ":GXAi#", ":GXAa#", ":GXAz#", ":GXAw#",
                    ":GXAr#", ":GXAf#", ":GXAn#", ":GXAb#"):
            print(cmd, link.gv(cmd), flush=True)
        with open(OUT, "w", encoding="ascii") as fh:
            fh.write("lat %.6f\n" % lat)
            fh.write("%d\n" % len(rows))
            for name, ra, dec, lst, a1, a2, pier in rows:
                fh.write("%s %.8f %.8f %.8f %s %s %s\n" % (name, ra, dec, lst, a1, a2, pier))
        print("wrote", OUT, flush=True)
        print("not saved", flush=True)
        compare_pc(rows, lat)
        go_home(link)
        return 0
    except Exception:
        print("stopped on an error", flush=True)
        try:
            link.digit(":Q#")
            link.digit(":AB#")
            go_home(link)
        except Exception:
            pass
        raise
    finally:
        link.close()


if __name__ == "__main__":
    sys.exit(main())
