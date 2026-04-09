#!/usr/bin/env python3
"""
Exercise mainunit_emu (or hardware on port 9997): unpark, first goto, wait until
not busy (:D#), second goto. Optional :Q# path.

Usage:
  python emu_goto_sequence_test.py [host] [port]
Defaults: 127.0.0.1 9997
"""
from __future__ import print_function

import socket
import sys
import time

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 9997
TIMEOUT_S = 120.0


def send_recv(sock, cmd):
    sock.sendall(cmd.encode("ascii"))
    time.sleep(0.06)
    buf = b""
    sock.settimeout(2.0)
    try:
        while True:
            b = sock.recv(8192)
            if not b:
                break
            buf += b
            if b"#" in buf:
                break
    except socket.timeout:
        pass
    return buf.decode("ascii", errors="replace")


def is_slewing_d_reply(reply):
    # :D# -> 0x7f# if moving, else first byte 0 then #
    if not reply:
        return False
    return len(reply) >= 1 and ord(reply[0]) == 0x7F


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5.0)
    t0 = time.time()
    s.connect((HOST, PORT))
    print("connected to %s:%s" % (HOST, PORT))

    def tx(label, cmd):
        r = send_recv(s, cmd)
        print("%-12s %s -> %r" % (label, cmd, r[:120] if len(r) > 120 else r))
        return r

    tx("unpark", ":hR#")
    tx("Sr", ":Sr06:00:00#")
    tx("Sd", ":Sd+45*00:00#")
    r = tx("MS_first", ":MS#")
    if r and r[0].isdigit() and r[0] != "0":
        print("ERROR: first :MS# failed with code", r[0])
        s.close()
        return 1

    print("Waiting until :D# reports not slewing (includes settle after arrival)...")
    while time.time() - t0 < TIMEOUT_S:
        d = send_recv(s, ":D#")
        if not is_slewing_d_reply(d):
            print("  idle/still :D# -> %r" % (d[:20],))
            break
        time.sleep(0.12)
    else:
        print("TIMEOUT waiting for slew/settle to finish")
        s.close()
        return 2

    tx("Sr2", ":Sr07:00:00#")
    tx("Sd2", ":Sd+40*00:00#")
    r2 = tx("MS_second", ":MS#")
    code = r2[0] if r2 else "?"
    print("Second :MS# first char (ERRGOTO wire): %r (0 = OK)" % code)

    tx("GXAS", ":GXAS#")
    s.close()
    print("done in %.1fs" % (time.time() - t0,))
    return 0 if (r2 and r2[0] == "0") else 3


if __name__ == "__main__":
    sys.exit(main() or 0)
