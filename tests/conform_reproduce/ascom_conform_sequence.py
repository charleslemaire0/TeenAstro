#!/usr/bin/env python3
"""
Reproduces the ASCOM conformance checker sequence before SlewToCoordinates.
Sends the same LX200 commands the driver sends and logs mount responses.
Run with: python ascom_conform_sequence.py [--port COM3] [--baud 57600] [--tcp 192.168.0.21]
Output: ascom_conform_log.txt
"""
import argparse
import base64
import datetime
import sys
import time
from datetime import datetime as dt, timedelta, timezone

try:
    import serial
    from serial.tools.list_ports import comports
except ImportError:
    print("pip install pyserial", file=sys.stderr)
    sys.exit(1)


def log(msg, logfile):
    ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    line = f"[{ts}] {msg}"
    print(line)
    logfile.write(line + "\n")
    logfile.flush()


def send_cmd(ser, cmd, logfile, expect_term=True):
    """Send LX200 command, read response, log both."""
    full_cmd = cmd if cmd.endswith("#") else cmd + "#"
    ser.write(full_cmd.encode("utf-8"))
    log(f"SEND: {full_cmd}", logfile)
    if expect_term:
        # pyserial: read_until(expected) uses ser.timeout; TCP: read_until(s, timeout)
        try:
            resp = ser.read_until(b"#", timeout=2).decode("utf-8", errors="replace").strip()
        except TypeError:
            resp = ser.read_until(b"#").decode("utf-8", errors="replace").strip()
        log(f"RECV: {resp}", logfile)
        return resp
    else:
        resp = ser.read(1).decode("utf-8", errors="replace")
        log(f"RECV: {resp}", logfile)
        return resp


def get_value(ser, cmd, logfile):
    """Send command, return response without #."""
    r = send_cmd(ser, cmd, logfile)
    return r.rstrip("#") if r else ""


def main():
    ap = argparse.ArgumentParser(description="Reproduce ASCOM conform sequence and log mount responses")
    ap.add_argument("--port", default="COM3", help="Serial port (e.g. COM3, /dev/ttyUSB0)")
    ap.add_argument("--baud", type=int, default=57600, help="Baud rate")
    ap.add_argument("--tcp", metavar="HOST", help="Use TCP (e.g. 192.168.0.21) instead of serial")
    ap.add_argument("--ra", default=21 + 23/60 + 39.1/3600, type=float, help="GOTO target RA (hours)")
    ap.add_argument("--dec", default=1.0, type=float, help="GOTO target Dec (degrees)")
    ap.add_argument("-o", "--output", default="ascom_conform_log.txt", help="Log file")
    ap.add_argument("--list-ports", action="store_true", help="List serial ports and exit")
    ap.add_argument("--conformer", action="store_true", help="Match conformer: GXAS read-backs, Dec before RA, 250ms GXAS slew polling")
    ap.add_argument("--exact-conformer", action="store_true", help="Exact conformer: + use LST as RA target, DeclinationRate before RARate, Tracking Off/On before slew")
    ap.add_argument("--full-conformer", action="store_true", help="FULL conformer: + SiteElevation, SiteLat/SiteLong attempts, SlewSettleTime, UTCDate +1hr then back (from ASCOM logs)")
    args = ap.parse_args()

    if args.list_ports:
        for p in comports():
            print(f"  {p.device}: {p.description}")
        return

    exact = getattr(args, "exact_conformer", False)
    full = getattr(args, "full_conformer", False)
    if full:
        exact = True
        args.conformer = True
    elif exact:
        args.conformer = True

    logpath = args.output
    with open(logpath, "w") as logf:
        log("=== ASCOM Conform Reproduce Test ===", logf)
        log(f"Port: {args.port}, Baud: {args.baud}", logf)
        if full:
            log("Mode: FULL conformer (+ Site/SlewSettleTime/UTCDate changes from ASCOM logs)", logf)
        elif exact:
            log("Mode: EXACT conformer (LST=RA target, DecRate before RARate, Tracking Off/On)", logf)
        ra_label = "LST from mount" if exact else f"{args.ra:.4f}h"
        log(f"GOTO target: RA={ra_label}, Dec={args.dec:.1f}deg", logf)
        log("", logf)

        if args.tcp:
            try:
                from telnetlib import Telnet
                tn = Telnet(args.tcp, 9999)
                log("Connected via TCP", logf)
                # Wrapper mimicking serial for LX200 (always read until #)
                class TcpWrap:
                    def __init__(self, tn): self.tn = tn
                    def write(self, b): self.tn.write(b)
                    def read_until(self, s, timeout=2): return self.tn.read_until(s, timeout)
                    def read(self, n): return (self.tn.read_until(b"#", timeout=2) or b"")[:n]
                    def close(self): self.tn.close()
                ser = TcpWrap(tn)
            except Exception as e:
                log(f"TCP connect failed: {e}", logf)
                return
        else:
            try:
                ser = serial.Serial(
                    port=args.port,
                    baudrate=args.baud,
                    bytesize=serial.EIGHTBITS,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    timeout=2,
                    write_timeout=2,
                )
                log("Connected via serial", logf)
            except Exception as e:
                log(f"Serial connect failed: {e}", logf)
                return

        try:
            # --- 1. Tracking on (Te) ---
            log("--- 1. Tracking ON ---", logf)
            send_cmd(ser, ":Te#", logf, expect_term=False)
            time.sleep(0.3)

            # Conformer order: DeclinationRate BEFORE RightAscensionRate
            sxd_rates = [(0, 0), (0.05, 500), (-0.05, -500), (40, 400000), (-40, -400000), (0, 0)]
            sxr_rates = [(0, 0), (0.00333, 33), (-0.00333, -33), (2.66667, 26667), (-2.66667, -26667), (0, 0)]
            if exact:
                log("--- 2. DeclinationRate (SXRd) ---", logf)
                for _, val in sxd_rates:
                    send_cmd(ser, f":SXRd,{val}#", logf, expect_term=False)
                    if args.conformer:
                        _ = get_value(ser, ":GXAS#", logf)
                        log("  (GXAS read-back after SXRd)", logf)
                    time.sleep(0.2)
                log("", logf)
                log("--- 3. RightAscensionRate (SXRr) ---", logf)
                for _, val in sxr_rates:
                    send_cmd(ser, f":SXRr,{val}#", logf, expect_term=False)
                    if args.conformer:
                        _ = get_value(ser, ":GXAS#", logf)
                        log("  (GXAS read-back after SXRr)", logf)
                    time.sleep(0.2)
            else:
                log("--- 2. RightAscensionRate (SXRr) ---", logf)
                for _, val in sxr_rates:
                    send_cmd(ser, f":SXRr,{val}#", logf, expect_term=False)
                    if args.conformer:
                        _ = get_value(ser, ":GXAS#", logf)
                        log("  (GXAS read-back after SXRr)", logf)
                    time.sleep(0.2)
                log("", logf)
                log("--- 3. DeclinationRate (SXRd) ---", logf)
                for _, val in sxd_rates:
                    send_cmd(ser, f":SXRd,{val}#", logf, expect_term=False)
                    if args.conformer:
                        _ = get_value(ser, ":GXAS#", logf)
                        log("  (GXAS read-back after SXRd)", logf)
                    time.sleep(0.2)
            log("", logf)

            # --- 3b. FULL conformer: Site, SlewSettleTime (from ASCOM logs) ---
            if full:
                log("--- 3b. SiteElevation, SiteLat/Long attempts, SlewSettleTime ---", logf)
                _ = get_value(ser, ":Ge#", logf)
                send_cmd(ser, ":Se+00059#", logf, expect_term=False)
                time.sleep(0.2)
                _ = get_value(ser, ":Gtf#", logf)
                send_cmd(ser, ":St+47:00:00#", logf, expect_term=False)
                time.sleep(0.2)
                _ = get_value(ser, ":Ggf#", logf)
                send_cmd(ser, ":Sg+001:33:04#", logf, expect_term=False)
                time.sleep(0.2)
                _ = get_value(ser, ":GXOS#", logf)
                send_cmd(ser, ":SXOS,1#", logf, expect_term=False)
                time.sleep(0.2)
                log("", logf)

            # --- 4. TrackingRate: Sidereal, Lunar, Solar, Sidereal ---
            log("--- 4. TrackingRate (TQ, TL, TS, TQ) ---", logf)
            for cmd in [":TQ#", ":TL#", ":TS#", ":TQ#"]:
                send_cmd(ser, cmd, logf, expect_term=False)
                if args.conformer:
                    _ = get_value(ser, ":GXAS#", logf)
                    log("  (GXAS read-back after " + cmd[:3] + ")", logf)
                time.sleep(0.2)
            log("", logf)

            # --- 4b. FULL conformer: UTCDate +1hr then back (from ASCOM logs - may cause GOTO failure) ---
            if full:
                log("--- 4b. UTCDate +1hr then revert (conformer does this before slew) ---", logf)
                raw = get_value(ser, ":GXAS#", logf)
                if len(raw) >= 88:
                    try:
                        dec = base64.b64decode(raw[:88])
                        if len(dec) >= 12:
                            y = 2000 + dec[11]
                            m = dec[9]
                            d = dec[10]
                            h, mi, sec = dec[6], dec[7], dec[8]
                            utc = dt(y, m, d, h, mi, sec, tzinfo=timezone.utc)
                            ts_orig = int(utc.timestamp())
                            utc_plus1 = utc + timedelta(hours=1)
                            ts_plus1 = int(utc_plus1.timestamp())
                            send_cmd(ser, f":SXT2,{ts_plus1}#", logf, expect_term=False)
                            time.sleep(0.3)
                            send_cmd(ser, f":SXT2,{ts_orig}#", logf, expect_term=False)
                            log(f"  SXT2 {ts_plus1} then {ts_orig}", logf)
                    except Exception as e:
                        log(f"  UTCDate change skipped: {e}", logf)
                time.sleep(0.3)
                log("", logf)

            # --- 5. GOTO target and slew ---
            log("--- 5. Set target and GOTO ---", logf)
            if exact:
                log("  (exact conformer: Tracking Off, wait, Tracking On)", logf)
                send_cmd(ser, ":Td#", logf, expect_term=False)
                time.sleep(1.0)
                send_cmd(ser, ":Te#", logf, expect_term=False)
                time.sleep(0.3)
                lst_str = get_value(ser, ":GSL#", logf)
                try:
                    ra_hours = float(lst_str)
                except ValueError:
                    ra_hours = args.ra
                    log(f"  (GSL parse failed, using --ra={ra_hours})", logf)
                ra_deg = ra_hours * 15.0
                log(f"  Using LST as RA target: {ra_hours:.6f}h = {ra_deg:.5f} deg", logf)
            else:
                ra_deg = args.ra * 15.0
            dec_val = args.dec
            sr_cmd = f":SrL{ra_deg:.5f}#"
            sd_cmd = f":SdL{dec_val:+.5f}#" if dec_val >= 0 else f":SdL{dec_val:.5f}#"
            if args.conformer:
                send_cmd(ser, sd_cmd, logf, expect_term=False)
                time.sleep(0.1)
                send_cmd(ser, sr_cmd, logf, expect_term=False)
            else:
                send_cmd(ser, sr_cmd, logf, expect_term=False)
                time.sleep(0.1)
                send_cmd(ser, sd_cmd, logf, expect_term=False)
            time.sleep(0.1)
            log("SEND: :MS#", logf)
            ser.write(b":MS#")
            try:
                resp = ser.read_until(b"#", timeout=2).decode("utf-8", errors="replace").strip()
            except TypeError:
                resp = ser.read_until(b"#").decode("utf-8", errors="replace").strip()
            log(f"RECV: {resp}", logf)
            if resp.startswith("0"):
                poll_interval = 0.25 if args.conformer else 1.0
                poll_cmd = ":GXAS#"
                log(f"GOTO started. Polling {poll_cmd} every {poll_interval}s...", logf)
                slew_start = time.time()
                while time.time() - slew_start < 120:
                    raw = get_value(ser, poll_cmd, logf)
                    slewing = None
                    if len(raw) >= 136:
                        try:
                            dec = base64.b64decode(raw[:136])
                            if len(dec) >= 1:
                                trk = dec[0] & 0x3
                                slewing = trk == 2
                                log(f"  GXAS trk={trk} slewing={slewing}", logf)
                        except Exception:
                            pass
                    if slewing is False:
                        log("Slew completed.", logf)
                        actual_ra = get_value(ser, ":GRL#", logf)
                        actual_dec = get_value(ser, ":GDL#", logf)
                        target_ra = get_value(ser, ":GrL#", logf)
                        target_dec = get_value(ser, ":GdL#", logf)
                        log(f"  Actual RA (GRL): {actual_ra}  Target RA (GrL): {target_ra}", logf)
                        log(f"  Actual Dec (GDL): {actual_dec}  Target Dec (GdL): {target_dec}", logf)
                        break
                    time.sleep(poll_interval)
                else:
                    log("Timeout (120s) waiting for slew to complete.", logf)
            else:
                log(f"GOTO rejected: {resp}", logf)
            log("", logf)
            log("=== Test done ===", logf)

        finally:
            if hasattr(ser, "close"):
                ser.close()
            elif hasattr(ser, "tn"):
                ser.tn.close()

    print(f"\nLog written to {logpath}")


if __name__ == "__main__":
    main()
