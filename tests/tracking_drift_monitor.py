"""
TeenAstro Tracking Drift Monitor

Connects to TeenAstro mount via TCP (LX200 protocol) and monitors RA drift
during sidereal tracking by polling RA, LST, axis position/target, and
tracking rates over time.

Usage:
    python tracking_drift_monitor.py --host <mount_ip> [--port 9999] [--duration 300] [--interval 1.0]

Results are logged to debug-adb7d4.log (NDJSON format) for analysis.
"""

import socket
import time
import json
import argparse
import sys
import os
import re

LOG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "debug-adb7d4.log")
SESSION_ID = "adb7d4"


def send_lx200(sock: socket.socket, cmd: str, timeout: float = 2.0) -> str:
    """Send an LX200 command and read response terminated by '#'."""
    sock.settimeout(timeout)
    sock.sendall(cmd.encode("ascii"))
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            chunk = sock.recv(256)
            if not chunk:
                break
            buf += chunk
            if b"#" in buf:
                break
        except socket.timeout:
            break
    resp = buf.decode("ascii", errors="replace")
    if resp.endswith("#"):
        resp = resp[:-1]
    return resp.strip()


def ra_hours_to_hms(h: float, decimal_seconds: int = 2) -> str:
    """Format decimal hours (RA) as HH:MM:SS.ss (with decimal seconds)."""
    if h < 0:
        h += 24
    if h >= 24:
        h -= 24
    hh = int(h)
    m = (h - hh) * 60
    mm = int(m)
    ss = (m - mm) * 60
    if decimal_seconds <= 0:
        return f"{hh:02d}:{mm:02d}:{int(round(ss)):02d}"
    fmt = f"{{ss:.{decimal_seconds}f}}"
    return f"{hh:02d}:{mm:02d}:{fmt.format(ss=ss)}"


def parse_hms(s: str) -> float:
    """Parse HH:MM:SS.ss or HH:MM.T into decimal hours."""
    m = re.match(r"([+-]?\d+):(\d+):(\d+\.?\d*)", s)
    if m:
        h, mi, sec = float(m.group(1)), float(m.group(2)), float(m.group(3))
        sign = -1 if s.startswith("-") else 1
        return sign * (abs(h) + mi / 60.0 + sec / 3600.0)
    m = re.match(r"([+-]?\d+):(\d+\.\d+)", s)
    if m:
        h, mi = float(m.group(1)), float(m.group(2))
        sign = -1 if s.startswith("-") else 1
        return sign * (abs(h) + mi / 60.0)
    try:
        return float(s)
    except ValueError:
        return float("nan")


def log_entry(entry: dict):
    """Append one NDJSON line to the log file."""
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(json.dumps(entry) + "\n")


def poll_mount(sock: socket.socket, run_id: str, sample_idx: int, t0: float):
    """Poll all relevant values from the mount and log them."""
    ts = time.time()
    elapsed = ts - t0

    # Use decimal commands for stable format (no precision toggle): :GRL# = RA in degrees, :GSL# = LST in hours
    ra_str = send_lx200(sock, ":GRL#")
    lst_str = send_lx200(sock, ":GSL#")
    dec_str = send_lx200(sock, ":GD#")

    # Debug values
    pos_a1_str = send_lx200(sock, ":GXDP0#")
    pos_a2_str = send_lx200(sock, ":GXDP1#")
    tgt_a1_str = send_lx200(sock, ":GXDP2#")
    tgt_a2_str = send_lx200(sock, ":GXDP3#")
    fstep_a1_str = send_lx200(sock, ":GXDR5#")
    req_rate_a1_str = send_lx200(sock, ":GXDR1#")
    cur_rate_a1_str = send_lx200(sock, ":GXDR3#")
    req_rate_ha_str = send_lx200(sock, ":GXRh#")

    # Tracking/refraction/alignment status
    status_str = send_lx200(sock, ":GXI#")
    refr_tracking_str = send_lx200(sock, ":GXrt#")
    refr_goto_str = send_lx200(sock, ":GXrg#")

    # Missed ticks counter (debug instrumentation)
    missed_ticks_str = send_lx200(sock, ":GXDW1#")

    # :GRL# = RA in decimal degrees; :GSL# = LST in decimal hours
    try:
        ra_deg = float(ra_str)
        ra_h = ra_deg / 15.0
    except ValueError:
        ra_h = parse_hms(ra_str)
    try:
        lst_h = float(lst_str)
    except ValueError:
        lst_h = parse_hms(lst_str)
    ha_h = lst_h - ra_h
    if ha_h < -12:
        ha_h += 24
    if ha_h > 12:
        ha_h -= 24

    def safe_float(s):
        try:
            return float(s)
        except (ValueError, TypeError):
            return None

    pos_a1 = safe_float(pos_a1_str)
    tgt_a1 = safe_float(tgt_a1_str)
    fstep_a1 = safe_float(fstep_a1_str)
    req_rate_a1 = safe_float(req_rate_a1_str)
    cur_rate_a1 = safe_float(cur_rate_a1_str)
    req_rate_ha = safe_float(req_rate_ha_str)

    missed_ticks = safe_float(missed_ticks_str)

    delta_target = None
    if pos_a1 is not None and tgt_a1 is not None:
        delta_target = tgt_a1 - pos_a1

    entry = {
        "sessionId": SESSION_ID,
        "id": f"poll_{sample_idx}",
        "timestamp": int(ts * 1000),
        "runId": run_id,
        "hypothesisId": "ALL",
        "location": "tracking_drift_monitor.py",
        "message": "tracking_sample",
        "data": {
            "sample": sample_idx,
            "elapsed_s": round(elapsed, 3),
            "ra_str": ra_str,
            "lst_str": lst_str,
            "dec_str": dec_str,
            "ra_hours": round(ra_h, 8),
            "lst_hours": round(lst_h, 8),
            "ha_hours": round(ha_h, 8),
            "ra_arcsec": round(ra_h * 3600 * 15, 4),
            "pos_a1": pos_a1,
            "tgt_a1": tgt_a1,
            "delta_target_a1": delta_target,
            "fstep_a1": fstep_a1,
            "req_rate_a1": req_rate_a1,
            "cur_rate_a1": cur_rate_a1,
            "req_rate_ha_x10000": req_rate_ha,
            "status": status_str,
            "refr_tracking": refr_tracking_str,
            "refr_goto": refr_goto_str,
            "missed_ticks": missed_ticks,
        },
    }
    log_entry(entry)

    return {
        "ra_h": ra_h,
        "lst_h": lst_h,
        "ha_h": ha_h,
        "pos_a1": pos_a1,
        "tgt_a1": tgt_a1,
        "elapsed": elapsed,
        "ra_str": ra_str,
        "lst_str": lst_str,
        "ra_display": ra_hours_to_hms(ra_h),
        "lst_display": f"{lst_h:.4f}h",
        "status": status_str,
        "refr_tracking": refr_tracking_str,
        "req_rate_a1": req_rate_a1,
        "cur_rate_a1": cur_rate_a1,
        "missed_ticks": missed_ticks,
    }


def analyze_and_log(samples: list, run_id: str):
    """Compute drift statistics from collected samples and log summary."""
    if len(samples) < 2:
        print("Not enough samples for analysis.")
        return

    first = samples[0]
    last = samples[-1]
    dt = last["elapsed"] - first["elapsed"]
    if dt <= 0:
        print("No time elapsed.")
        return

    dra_h = last["ra_h"] - first["ra_h"]
    if dra_h > 12:
        dra_h -= 24
    if dra_h < -12:
        dra_h += 24
    dra_arcsec = dra_h * 3600 * 15

    dlst_h = last["lst_h"] - first["lst_h"]
    if dlst_h < 0:
        dlst_h += 24
    dha_h = last["ha_h"] - first["ha_h"]

    ra_drift_rate_arcsec_per_min = (dra_arcsec / dt) * 60

    # Compute RA jitter (standard deviation of short-term RA differences)
    ra_diffs = []
    for i in range(1, len(samples)):
        diff = samples[i]["ra_h"] - samples[i - 1]["ra_h"]
        if diff > 12:
            diff -= 24
        if diff < -12:
            diff += 24
        ra_diffs.append(diff * 3600 * 15)

    ra_diff_mean = sum(ra_diffs) / len(ra_diffs) if ra_diffs else 0
    ra_diff_var = sum((d - ra_diff_mean) ** 2 for d in ra_diffs) / len(ra_diffs) if ra_diffs else 0
    ra_jitter_arcsec = ra_diff_var ** 0.5

    # Check tracking rates
    rates_a1 = [s["req_rate_a1"] for s in samples if s.get("req_rate_a1") is not None]
    rate_min = min(rates_a1) if rates_a1 else None
    rate_max = max(rates_a1) if rates_a1 else None

    # Check target-pos deltas
    deltas = [s["tgt_a1"] - s["pos_a1"] for s in samples
              if s.get("tgt_a1") is not None and s.get("pos_a1") is not None]
    delta_min = min(deltas) if deltas else None
    delta_max = max(deltas) if deltas else None

    # Missed ticks
    missed_values = [s.get("missed_ticks") for s in samples if s.get("missed_ticks") is not None]
    missed_last = missed_values[-1] if missed_values else None
    missed_first = missed_values[0] if missed_values else None
    missed_during_test = None
    if missed_last is not None and missed_first is not None:
        missed_during_test = missed_last - missed_first

    # Refraction/alignment status
    refr_values = set(s.get("refr_tracking", "") for s in samples)
    status_values = set(s.get("status", "")[:4] for s in samples)

    summary = {
        "sessionId": SESSION_ID,
        "id": "summary",
        "timestamp": int(time.time() * 1000),
        "runId": run_id,
        "hypothesisId": "ALL",
        "location": "tracking_drift_monitor.py",
        "message": "drift_summary",
        "data": {
            "total_samples": len(samples),
            "duration_s": round(dt, 1),
            "ra_first_str": first["ra_str"],
            "ra_last_str": last["ra_str"],
            "ra_total_drift_arcsec": round(dra_arcsec, 4),
            "ra_drift_rate_arcsec_per_min": round(ra_drift_rate_arcsec_per_min, 4),
            "ra_jitter_arcsec": round(ra_jitter_arcsec, 4),
            "lst_change_hours": round(dlst_h, 6),
            "ha_change_hours": round(dha_h, 6),
            "rate_a1_range": [rate_min, rate_max],
            "delta_target_range": [delta_min, delta_max],
            "refraction_for_tracking": list(refr_values),
            "tracking_status_chars": list(status_values),
            "missed_ticks_during_test": missed_during_test,
            "missed_ticks_total": missed_last,
        },
    }
    log_entry(summary)

    print("\n" + "=" * 60)
    print("TRACKING DRIFT ANALYSIS")
    print("=" * 60)
    print(f"Duration:              {dt:.1f} s")
    print(f"Samples:               {len(samples)}")
    print(f"RA (first):            {first['ra_str']}")
    print(f"RA (last):             {last['ra_str']}")
    print(f"RA total drift:        {dra_arcsec:.4f} arcsec")
    print(f"RA drift rate:         {ra_drift_rate_arcsec_per_min:.4f} arcsec/min")
    print(f"RA jitter (1-sigma):   {ra_jitter_arcsec:.4f} arcsec")
    print(f"LST change:            {dlst_h:.6f} hours")
    print(f"HA change:             {dha_h:.6f} hours")
    print(f"A1 rate range:         [{rate_min}, {rate_max}]")
    print(f"A1 delta(tgt-pos):     [{delta_min}, {delta_max}]")
    print(f"Refraction tracking:   {refr_values}")
    print(f"Status:                {status_values}")
    print(f"Missed ticks (test):   {missed_during_test}")
    print(f"Missed ticks (total):  {missed_last}")

    if abs(ra_drift_rate_arcsec_per_min) > 0.1:
        print(f"\n** DRIFT DETECTED: {ra_drift_rate_arcsec_per_min:.4f} arcsec/min **")
        if rates_a1 and (rate_min != rate_max or abs(rates_a1[0]) != 1.0):
            print("   -> Hypothesis B likely: tracking rate is not exactly 1.0")
        if 'y' in refr_values:
            print("   -> Hypothesis B confirmed: refraction for tracking is ON")
        if deltas and (delta_max - delta_min) > 10:
            print("   -> Hypothesis A/E likely: target-pos delta varies significantly")
        if missed_during_test is not None and missed_during_test > 0:
            print(f"   -> Hypothesis A/D confirmed: {missed_during_test} ticks missed during test")
    else:
        print("\n   No significant RA drift detected.")
        if missed_during_test is not None and missed_during_test > 0:
            print(f"   (But {missed_during_test} ticks were missed - compensated by fix)")

    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description="TeenAstro Tracking Drift Monitor")
    parser.add_argument("--host", required=True, help="Mount IP address or hostname")
    parser.add_argument("--port", type=int, default=9999, help="TCP port (default: 9999)")
    parser.add_argument("--duration", type=int, default=300, help="Monitoring duration in seconds (default: 300)")
    parser.add_argument("--interval", type=float, default=2.0, help="Polling interval in seconds (default: 2.0)")
    args = parser.parse_args()

    run_id = f"drift_{int(time.time())}"

    print(f"Connecting to {args.host}:{args.port}...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((args.host, args.port))
    except Exception as e:
        print(f"Connection failed: {e}")
        sys.exit(1)

    print(f"Connected. Monitoring for {args.duration}s at {args.interval}s intervals...")
    print(f"Logging to: {LOG_PATH}")

    log_entry({
        "sessionId": SESSION_ID,
        "id": "start",
        "timestamp": int(time.time() * 1000),
        "runId": run_id,
        "hypothesisId": "ALL",
        "location": "tracking_drift_monitor.py",
        "message": "monitoring_start",
        "data": {
            "host": args.host,
            "port": args.port,
            "duration": args.duration,
            "interval": args.interval,
        },
    })

    # Set high precision mode
    send_lx200(sock, ":U#", timeout=0.5)
    time.sleep(0.2)
    send_lx200(sock, ":U#", timeout=0.5)

    # Reset missed ticks counter
    send_lx200(sock, ":GXDW2#", timeout=0.5)

    samples = []
    t0 = time.time()
    sample_idx = 0

    try:
        while time.time() - t0 < args.duration:
            try:
                result = poll_mount(sock, run_id, sample_idx, t0)
                samples.append(result)
                sample_idx += 1

                status = result.get("status", "?")
                tracking_on = status[0] in ('1', '3') if len(status) > 0 else False
                if not tracking_on:
                    print(f"  [{result['elapsed']:.1f}s] WARNING: Tracking appears OFF (status={status})")
                else:
                    drift_so_far = 0
                    if len(samples) > 1:
                        dra = result["ra_h"] - samples[0]["ra_h"]
                        if dra > 12: dra -= 24
                        if dra < -12: dra += 24
                        drift_so_far = dra * 3600 * 15
                    mt = result.get('missed_ticks', '?')
                    print(f"  [{result['elapsed']:6.1f}s] RA={result.get('ra_display', result['ra_str']):>10s}  LST={result.get('lst_display', result['lst_str']):>10s}  "
                          f"drift={drift_so_far:+.3f}\"  rate={result.get('req_rate_a1','?')}  missed={mt}")

            except Exception as e:
                print(f"  Poll error: {e}")
                log_entry({
                    "sessionId": SESSION_ID,
                    "id": f"error_{sample_idx}",
                    "timestamp": int(time.time() * 1000),
                    "runId": run_id,
                    "hypothesisId": "ALL",
                    "location": "tracking_drift_monitor.py",
                    "message": "poll_error",
                    "data": {"error": str(e), "sample": sample_idx},
                })

            time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\nMonitoring interrupted.")

    finally:
        analyze_and_log(samples, run_id)
        sock.close()
        print(f"\nLog saved to: {LOG_PATH}")


if __name__ == "__main__":
    main()
