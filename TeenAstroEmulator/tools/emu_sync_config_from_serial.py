#!/usr/bin/env python3
"""
Read TeenAstro mount configuration from a serial port (hardware) and apply the same
motor/rate/limit/refraction/options as encoded in :GXCS# to the MainUnit emulator on TCP.

Does not clone the full EEPROM (alignment, park, etc.). Site lat/long is optional.

Mount type from :GW# is normally NOT applied: :S!n# triggers reboot_unit, and the PC
emulator's reboot() exits the process (TCP dies). Use --apply-mount-type only if you
will restart mainunit_emu yourself after the script (or you target real hardware).

Usage:
  python emu_sync_config_from_serial.py [--serial COM3] [--emu-host 127.0.0.1] [--emu-port 9997]

Requires pyserial for --serial.
"""
from __future__ import print_function

import argparse
import base64
import socket
import struct
import sys
import time

try:
    import serial
except ImportError:
    serial = None


def read_until_hash(ser, max_bytes=4096, timeout=2.0):
    ser.timeout = 0.05
    end = time.time() + timeout
    buf = b""
    while time.time() < end and len(buf) < max_bytes:
        chunk = ser.read(4096)
        if chunk:
            buf += chunk
            if b"#" in buf:
                break
        time.sleep(0.02)
    return buf


def serial_query(port, baud, cmd):
    """Send :CMD# on serial; return reply string (with #)."""
    s = serial.Serial(port, baud, bytesize=8, parity="N", stopbits=1, timeout=1)
    try:
        s.reset_input_buffer()
        s.write(cmd.encode("ascii"))
        s.flush()
        time.sleep(0.12)
        raw = read_until_hash(s)
        return raw.decode("ascii", errors="replace")
    finally:
        s.close()


def tcp_send_recv(sock, cmd, timeout=3.0):
    sock.sendall(cmd.encode("ascii"))
    time.sleep(0.05)
    buf = b""
    sock.settimeout(timeout)
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


def ok_reply(r):
    if not r:
        return False
    return r[0] == "1"


def parse_gxcs(b64_with_hash):
    s = b64_with_hash.strip()
    if s.endswith("#"):
        s = s[:-1]
    raw = base64.b64decode(s)
    if len(raw) != 90:
        raise ValueError("GXCS payload len %d (want 90)" % len(raw))
    return raw


def u16le(p, o):
    return p[o] | (p[o + 1] << 8)


def u32le(p, o):
    return p[o] | (p[o + 1] << 8) | (p[o + 2] << 16) | (p[o + 3] << 24)


def i16le(p, o):
    v = u16le(p, o)
    if v >= 0x8000:
        v -= 0x10000
    return v


def f32le(p, o):
    return struct.unpack_from("<f", p, o)[0]


def yn(b):
    return "y" if b else "n"


def gw_to_mount_cmd(gw):
    """First char of :GW# reply: G=GEM(1), P=FORK(2), A=AltAz or ForkAlt(3), L=undefined."""
    if not gw:
        return None
    c = gw[0]
    m = {"G": 1, "P": 2, "A": 3, "L": 0}
    idx = m.get(c)
    if idx is None or idx == 0:
        return None
    return ":S!%d#" % idx


def build_commands_from_gxcs(pkt):
    cmds = []

    def axis_block(base, letter):
        g = u32le(pkt, base + 0)
        step_rot = u16le(pkt, base + 4)
        backlash = u16le(pkt, base + 6)
        bl_rate = u16le(pkt, base + 8)
        low_c = u16le(pkt, base + 10)
        high_c = u16le(pkt, base + 12)
        micro = pkt[base + 14]
        flags = pkt[base + 15]
        rev = flags & 1
        silent = (flags >> 1) & 1
        cmds.append(":SXMG,%s,%u#" % (letter, g))
        cmds.append(":SXMS,%s,%u#" % (letter, step_rot))
        cmds.append(":SXMB,%s,%d#" % (letter, backlash))
        cmds.append(":SXMb,%s,%d#" % (letter, bl_rate))
        cmds.append(":SXMc,%s,%u#" % (letter, low_c))
        cmds.append(":SXMC,%s,%u#" % (letter, high_c))
        cmds.append(":SXMM,%s,%u#" % (letter, micro))
        cmds.append(":SXMm,%s,%u#" % (letter, silent))
        cmds.append(":SXMR,%s,%u#" % (letter, rev))

    axis_block(0, "R")
    axis_block(16, "D")

    for i in range(4):
        fv = f32le(pkt, 32 + 4 * i)
        cmds.append(":SXR%d,%.6f#" % (i, fv))
    cmds.append(":SXRA,%.6f#" % f32le(pkt, 48))
    cmds.append(":SXRX,%u#" % u16le(pkt, 52))
    cmds.append(":SXRD,%u#" % pkt[54])
    cmds.append(":SXOS,%u#" % pkt[55])

    me = i16le(pkt, 56)
    mw = i16le(pkt, 58)
    cmds.append(":SXLE,%d#" % me)
    cmds.append(":SXLW,%d#" % mw)

    a1min = i16le(pkt, 60)
    a1max = i16le(pkt, 62)
    a2min = i16le(pkt, 64)
    a2max = i16le(pkt, 66)
    cmds.append(":SXLB,%d#" % a1max)
    cmds.append(":SXLD,%d#" % a2max)
    cmds.append(":SXLA,%d#" % a1min)
    cmds.append(":SXLC,%d#" % a2min)

    cmds.append(":SXLU,%u#" % u16le(pkt, 68))

    i8a = pkt[70]
    if i8a >= 128:
        i8a -= 256
    i8b = pkt[71]
    if i8b >= 128:
        i8b -= 256
    cmds.append(":SXLH,%d#" % i8a)
    cmds.append(":SXLO,%d#" % i8b)
    cmds.append(":SXLS,%d#" % pkt[72])

    rf = pkt[73]
    cmds.append(":SXrt,%s#" % yn((rf >> 0) & 1))
    cmds.append(":SXrg,%s#" % yn((rf >> 1) & 1))
    cmds.append(":SXrp,%s#" % yn((rf >> 2) & 1))

    # Do not send :SXOI# here — it forces reboot_unit and would interrupt the rest.

    return cmds


def main():
    ap = argparse.ArgumentParser(description="Sync GXCS-based config from serial to emulator TCP")
    ap.add_argument("--serial", default="COM3", help="Serial port (hardware)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--emu-host", default="127.0.0.1")
    ap.add_argument("--emu-port", type=int, default=9997)
    ap.add_argument(
        "--apply-mount-type",
        action="store_true",
        help="Send :S!n# from :GW# (WARNING: emulator exits on reboot — restart mainunit_emu manually)",
    )
    args = ap.parse_args()

    if serial is None:
        print("pyserial required: pip install pyserial", file=sys.stderr)
        return 1

    print("Reading :GXCS# / :GW# from %s ..." % args.serial)
    r_cs = serial_query(args.serial, args.baud, ":GXCS#")
    r_gw = serial_query(args.serial, args.baud, ":GW#")
    if not r_cs or "#" not in r_cs:
        print("Bad :GXCS# reply: %r" % (r_cs[:80],), file=sys.stderr)
        return 1
    pkt = parse_gxcs(r_cs)
    cmds = build_commands_from_gxcs(pkt)

    mount_cmd = gw_to_mount_cmd(r_gw) if args.apply_mount_type else None
    print("  :GW# -> %r  => mount cmd %s" % (r_gw.strip(), mount_cmd))

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(15.0)
    sock.connect((args.emu_host, args.emu_port))
    print("Connected emulator %s:%s" % (args.emu_host, args.emu_port))

    def tx(label, c):
        out = tcp_send_recv(sock, c)
        ok = ok_reply(out)
        print("  %-26s %s -> %s %r" % (label, c[:48] + ("..." if len(c) > 48 else ""), "OK" if ok else "??", out[:60]))
        return ok

    if mount_cmd:
        tx("mount_type", mount_cmd)
        sock.close()
        print("Waiting for emulator reboot after :S!# ...")
        time.sleep(4.0)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(30.0)
        for attempt in range(20):
            try:
                sock.connect((args.emu_host, args.emu_port))
                break
            except OSError:
                time.sleep(0.5)
        else:
            print("Could not reconnect to emulator after reboot.", file=sys.stderr)
            return 1

    tx("unpark", ":hR#")
    tx("track_on", ":Te#")

    for i, c in enumerate(cmds):
        lab = "gxcs_%04d" % i
        tx(lab, c)

    sock.close()
    print("Done. Emulator should mirror hardware :GXCS# fields (motors/rates/limits/refraction/mount index).")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
