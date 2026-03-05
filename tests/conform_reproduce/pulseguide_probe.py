#!/usr/bin/env python3
"""
pulseguide_probe.py - Direct TeenAstro main-unit pulse-guiding tester.

- Opens a serial connection to the main unit
- Enables tracking
- Sends a pulse-guiding command (:Mgn<ms># / :Mgs<ms># / :Mge<ms># / :Mgw<ms>#)
- Polls :GXAS# and measures how long the PulseGuiding flag stays true
"""

import argparse
import base64
import json
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path

import serial
import struct

try:
    # Keep release/version in sync with build_firmware.py if available
    from build_firmware import RELEASE_VERSION as FW_RELEASE
except Exception:
    FW_RELEASE = "1.6"


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MAIN_ENV = "240_5160"
DEFAULT_MAIN_HEX_BASE = f"TeenAstro_{FW_RELEASE}_240_TMC5160"
DEFAULT_MAIN_BOARD = "TEENSY31"  # Matches TeenAstroUploader for 2.4 TMC2130/5160
LOG_PATH = REPO_ROOT / "debug-97e435.log"
DEBUG_SESSION_ID = "97e435"


def log_debug(hypothesis_id: str, message: str, data: dict, run_id: str = "pre-fix") -> None:
    """Append a single NDJSON debug log line to debug-97e435.log."""
    entry = {
        "sessionId": DEBUG_SESSION_ID,
        "id": f"log_{int(time.time() * 1000)}_{hypothesis_id}",
        "timestamp": int(time.time() * 1000),
        "location": "pulseguide_probe.py",
        "message": message,
        "data": data,
        "runId": run_id,
        "hypothesisId": hypothesis_id,
    }
    try:
        with LOG_PATH.open("a", encoding="utf-8") as f:
            f.write(json.dumps(entry) + "\n")
    except Exception:
        # Logging must never break the guiding test
        pass


def read_reply_until_hash(ser: serial.Serial, timeout: float = 2.0) -> str:
    """Read ASCII reply up to terminating '#' from TeenAstro."""
    end_time = time.time() + timeout
    buf = bytearray()
    while time.time() < end_time:
        b = ser.read(1)
        if not b:
            continue
        if b == b"#":
            break
        buf += b
    else:
        raise TimeoutError("Timed out waiting for '#' terminator.")
    return buf.decode("ascii", errors="replace").strip()


def send_command(ser: serial.Serial, body: str, expect_reply: bool = True, timeout: float = 2.0) -> str | None:
    """
    Send a LX200-style command of the form :<body># and optionally read the reply.

    Examples:
      body="GXAS"    → sends ":GXAS#"
      body="Mgn1000" → sends ":Mgn1000#"
    """
    cmd = f":{body}#".encode("ascii")
    ser.write(cmd)
    ser.flush()
    if not expect_reply:
        return None
    return read_reply_until_hash(ser, timeout=timeout)


def fetch_gxas_packet(ser: serial.Serial) -> bytes:
    """Fetch and validate a single :GXAS# snapshot. Returns the 66-byte binary payload."""
    b64 = send_command(ser, "GXAS", expect_reply=True, timeout=2.0)
    if not b64:
        raise RuntimeError("Empty GXAS reply.")

    raw = b64.strip()
    # TeenAstro spec: exactly 88 base64 chars (→ 66 bytes) plus '#'
    # But if there are stray characters, search all 88-char windows for a valid packet.
    candidates: list[str] = []
    if len(raw) == 88:
        candidates = [raw]
    elif len(raw) > 88:
        for start in range(0, len(raw) - 87):
            candidates.append(raw[start : start + 88])
    else:
        raise RuntimeError(f"GXAS reply too short for 66-byte packet: len={len(raw)}, data='{raw}'")

    last_error: Exception | None = None
    for idx, c in enumerate(candidates):
        try:
            pkt = base64.b64decode(c, validate=False)
        except Exception as exc:
            last_error = exc
            continue
        if len(pkt) != 66:
            last_error = RuntimeError(f"decode len {len(pkt)} (candidate {idx}, total candidates={len(candidates)})")
            continue
        xor_chk = 0
        for b in pkt[:65]:
            xor_chk ^= b
        if xor_chk == pkt[65]:
            if idx != 0 or len(raw) != 88:
                print(
                    f"NOTE: GXAS valid window at offset {idx} within raw reply (len={len(raw)}).",
                    file=sys.stderr,
                )
            return pkt
        last_error = RuntimeError(
            f"checksum mismatch for candidate {idx} (len(raw)={len(raw)})"
        )

    raise RuntimeError(
        f"GXAS decode failed for all candidates (len(raw)={len(raw)}, raw='{raw}'): {last_error}"
    )


def gxas_pulse_guiding(pkt: bytes) -> bool:
    """Return True if GXAS 'PulseGuiding' flag is set (byte 2, bit 7)."""
    if len(pkt) < 3:
        raise ValueError("GXAS packet too short.")
    return bool(pkt[2] & 0x80)


def gxas_dec_deg(pkt: bytes) -> float:
    """Return Dec (degrees) from GXAS packet (bytes 16-19, little-endian float32)."""
    if len(pkt) < 20:
        raise ValueError("GXAS packet too short for Dec.")
    return struct.unpack("<f", pkt[16:20])[0]


def gxas_ra_hours(pkt: bytes) -> float:
    """Return RA (hours) from GXAS packet (bytes 12-15, little-endian float32)."""
    if len(pkt) < 16:
        raise ValueError("GXAS packet too short for RA.")
    return struct.unpack("<f", pkt[12:16])[0]


def gxas_lst_hours(pkt: bytes) -> float:
    """Return LST (hours) from GXAS packet (bytes 28-31, little-endian float32)."""
    if len(pkt) < 32:
        raise ValueError("GXAS packet too short for LST.")
    return struct.unpack("<f", pkt[28:32])[0]


def query_guide_rate0(ser: serial.Serial) -> float:
    """Query guideRates[0] via :GXR0# (returns rate multiple of sidereal)."""
    reply = send_command(ser, "GXR0", expect_reply=True, timeout=2.0)
    if not reply:
        raise RuntimeError("Empty reply from :GXR0#")
    try:
        return float(reply)
    except ValueError as exc:
        raise RuntimeError(f"Failed to parse GXR0 reply '{reply}'") from exc


def set_guide_rate0(ser: serial.Serial, rate: float) -> None:
    """
    Set guideRates[0] via :SXR0,NN# where NN = rate*100 (0.01 resolution).

    In firmware, val is 1..255 and guideRates[0] = val / 100.
    Reply is a single '1' or '0' without '#' terminator.
    """
    if rate <= 0.0 or rate >= 2.55:
        raise ValueError("guide rate 0 must be between 0.01 and 2.54 (exclusive)")
    val = int(round(rate * 100.0))
    cmd = f":SXR0,{val}#".encode("ascii")
    ser.write(cmd)
    ser.flush()
    # Read a single byte reply ('1' or '0'); do not wait for '#'
    ack = ser.read(1)
    if not ack:
        print(f"Warning: no reply from :SXR0,{val}#", file=sys.stderr)
    elif ack not in (b"0", b"1"):
        print(f"Warning: unexpected reply to :SXR0,{val}#: {ack!r}", file=sys.stderr)


def query_dec_guiding_diagnostics(ser: serial.Serial) -> dict:
    """
    Query internal Dec-axis guiding parameters via GX debug commands.

    Uses:
      :GXDR7# -> guideA2.absRate
      :GXDR8# -> guideA2.speedMultiplier
      :GXDR9# -> guideA2.getAmount() (signed steps per guiding tick when moving)
    """
    abs_rate = speed_mult = amount = None
    try:
        r = send_command(ser, "GXDR7", expect_reply=True, timeout=2.0)
        abs_rate = float(r) if r is not None and r != "" else None
    except Exception:
        abs_rate = None
    try:
        r = send_command(ser, "GXDR8", expect_reply=True, timeout=2.0)
        speed_mult = float(r) if r is not None and r != "" else None
    except Exception:
        speed_mult = None
    try:
        r = send_command(ser, "GXDR9", expect_reply=True, timeout=2.0)
        amount = float(r) if r is not None and r != "" else None
    except Exception:
        amount = None
    return {
        "guideA2_absRate": abs_rate,
        "guideA2_speedMultiplier": speed_mult,
        "guideA2_amount": amount,
    }


def query_axis_intervals(ser: serial.Serial) -> dict:
    """
    Query axis1 and axis2 interval_Step_Cur via GX debug:
      :GXDRA# -> axes.staA1.interval_Step_Cur (RA)
      :GXDRB# -> axes.staA2.interval_Step_Cur (Dec)
    """
    a1_cur = a2_cur = None
    try:
        r = send_command(ser, "GXDRA", expect_reply=True, timeout=2.0)
        a1_cur = float(r) if r is not None and r != "" else None
    except Exception:
        a1_cur = None
    try:
        r = send_command(ser, "GXDRB", expect_reply=True, timeout=2.0)
        a2_cur = float(r) if r is not None and r != "" else None
    except Exception:
        a2_cur = None
    return {
        "axis1_interval_Step_Cur": a1_cur,
        "axis2_interval_Step_Cur": a2_cur,
    }


def query_axis1_interval_state(ser: serial.Serial) -> dict:
    """Query axis1 interval (and axis2) for logging; same as query_axis_intervals."""
    return query_axis_intervals(ser)


def query_axis1_delta_state(ser: serial.Serial) -> dict:
    """
    Query axis1 position, target, and deltaTarget via GXDP debug:
      :GXDP0# -> axis1 pos
      :GXDP2# -> axis1 target
      :GXDP4# -> axis1 deltaTarget (target - pos, after updateDeltaTarget()).
    """
    pos = target = delta = None
    try:
        r = send_command(ser, "GXDP0", expect_reply=True, timeout=2.0)
        pos = int(r) if r is not None and r != "" else None
    except Exception:
        pos = None
    try:
        r = send_command(ser, "GXDP2", expect_reply=True, timeout=2.0)
        target = int(r) if r is not None and r != "" else None
    except Exception:
        target = None
    try:
        r = send_command(ser, "GXDP4", expect_reply=True, timeout=2.0)
        delta = int(r) if r is not None and r != "" else None
    except Exception:
        delta = None
    return {
        "axis1_pos": pos,
        "axis1_target": target,
        "axis1_deltaTarget": delta,
    }


def query_motor_geometry(ser: serial.Serial) -> dict:
    """
    Query gear, step per rotation, and microstep for both axes via GXMG/GXMS/GXMM.

    Returns a dict with raw parameters and derived stepsPerRot/stepsPerCentiSecond
    for RA (axis 'R') and Dec (axis 'D').
    """
    def _axis_params(axis_char: str) -> tuple[int, int, int]:
        gear = int(send_command(ser, f"GXMG{axis_char}", expect_reply=True, timeout=2.0))
        step_rot = int(send_command(ser, f"GXMS{axis_char}", expect_reply=True, timeout=2.0))
        micro = int(send_command(ser, f"GXMM{axis_char}", expect_reply=True, timeout=2.0))
        return gear, step_rot, micro

    gear_R, stepRot_R, micro_R = _axis_params("R")
    gear_D, stepRot_D, micro_D = _axis_params("D")

    def _derived(gear: int, step_rot: int, micro: int) -> tuple[float, float]:
        steps_per_rot = (gear / 1000.0) * step_rot * (2.0 ** micro)
        steps_per_centi = steps_per_rot / 8640000.0
        return steps_per_rot, steps_per_centi

    stepsRot_R, stepsCenti_R = _derived(gear_R, stepRot_R, micro_R)
    stepsRot_D, stepsCenti_D = _derived(gear_D, stepRot_D, micro_D)

    geom = {
        "RA": {
            "gear": gear_R,
            "stepRot": stepRot_R,
            "micro": micro_R,
            "stepsPerRot": stepsRot_R,
            "stepsPerCentiSecond": stepsCenti_R,
        },
        "Dec": {
            "gear": gear_D,
            "stepRot": stepRot_D,
            "micro": micro_D,
            "stepsPerRot": stepsRot_D,
            "stepsPerCentiSecond": stepsCenti_D,
        },
    }

    # Log for later analysis: verifies whether geometry and per-tick amounts match.
    log_debug(
        hypothesis_id="H_geom",
        message="motor_geometry",
        data=geom,
        run_id="geom_check",
    )

    return geom


def run_ra_e_w_test(ser: serial.Serial, duration_ms: int, guide_rate: float | None) -> None:
    """
    Test RA East/West guiding with tracking OFF and ON.

    - With tracking OFF (:Td#), apply East and West pulses and compare pure guiding offsets.
    - With tracking ON (:Te#), apply East and West pulses and compare offsets (guide+tracking).
    """
    print("\n=== RA E/W symmetry test ===")
    if guide_rate is None or guide_rate <= 0.0 or guide_rate >= 2.55:
        # Default: use current GXR0
        guide_rate = query_guide_rate0(ser)
    print(f"Using guide rate 0 (GXR0) = {guide_rate:.4f}x")
    set_guide_rate0(ser, guide_rate)
    time.sleep(0.5)
    confirmed = query_guide_rate0(ser)
    print(f"Confirmed GXR0: {confirmed:.4f}x")

    def measure_ra_delta(direction: str, tracking_on: bool) -> float:
        if tracking_on:
            enable_tracking(ser)
        else:
            disable_tracking(ser)
        time.sleep(0.5)

        # Keep geometry simple: sync to zenith before each pulse.
        sync_mount_to_zenith(ser)
        time.sleep(0.2)

        initial_pkt = fetch_gxas_packet(ser)
        initial_ra_h = gxas_ra_hours(initial_pkt)
        initial_lst_h = gxas_lst_hours(initial_pkt)

        # For tracking OFF (pure guiding), capture axis interval and deltaTarget around the pulse.
        if not tracking_on:
            state_before = query_axis1_interval_state(ser)
            dt_before = query_axis1_delta_state(ser)
            print(
                f"  BEFORE {direction.upper()}: "
                f"A1_int={state_before.get('axis1_interval_Step_Cur')} µs, "
                f"A2_int={state_before.get('axis2_interval_Step_Cur')} µs, "
                f"pos={dt_before.get('axis1_pos')}, "
                f"target={dt_before.get('axis1_target')}, "
                f"deltaTarget={dt_before.get('axis1_deltaTarget')}"
            )
            log_debug(
                hypothesis_id="H_DT1",
                message="axis1_before_pulse",
                data={
                    "direction": direction,
                    "tracking_on": tracking_on,
                    **state_before,
                    **dt_before,
                },
                run_id="ra_interval",
            )

        body = f"Mg{direction}{duration_ms}"
        print(f"\n[{'TRACK ON' if tracking_on else 'TRACK OFF'}] Sending :{body}#")
        t0 = time.time()
        send_command(ser, body, expect_reply=False)

        # Poll GXAS until PulseGuiding goes false or timeout
        max_elapsed = duration_ms / 1000.0 + 10.0
        seen_start = False
        logged_interval_during = False
        while True:
            now = time.time()
            elapsed = now - t0
            try:
                pkt = fetch_gxas_packet(ser)
                pg = gxas_pulse_guiding(pkt)
            except Exception as exc:
                print(f"{elapsed:8.3f}  ERROR reading GXAS: {exc}", file=sys.stderr)
                pg = False

            if not tracking_on and pg and not logged_interval_during:
                state_during = query_axis1_interval_state(ser)
                dt_during = query_axis1_delta_state(ser)
                print(
                    f"  DURING {direction.upper()}: "
                    f"A1_int={state_during.get('axis1_interval_Step_Cur')} µs, "
                    f"A2_int={state_during.get('axis2_interval_Step_Cur')} µs, "
                    f"pos={dt_during.get('axis1_pos')}, "
                    f"target={dt_during.get('axis1_target')}, "
                    f"deltaTarget={dt_during.get('axis1_deltaTarget')}"
                )
                log_debug(
                    hypothesis_id="H_DT1",
                    message="axis1_during_pulse",
                    data={
                        "direction": direction,
                        "tracking_on": tracking_on,
                        **state_during,
                        **dt_during,
                    },
                    run_id="ra_interval",
                )
                logged_interval_during = True

            if pg and not seen_start:
                seen_start = True
            if seen_start and not pg:
                break
            if elapsed > max_elapsed:
                print(f"Timeout ({max_elapsed:.1f}s) while waiting for PulseGuiding to finish.")
                break
            time.sleep(0.1)

        if not tracking_on:
            state_after = query_axis1_interval_state(ser)
            dt_after = query_axis1_delta_state(ser)
            print(
                f"  AFTER {direction.upper()}: "
                f"A1_int={state_after.get('axis1_interval_Step_Cur')} µs, "
                f"A2_int={state_after.get('axis2_interval_Step_Cur')} µs, "
                f"pos={dt_after.get('axis1_pos')}, "
                f"target={dt_after.get('axis1_target')}, "
                f"deltaTarget={dt_after.get('axis1_deltaTarget')}"
            )
            log_debug(
                hypothesis_id="H_DT1",
                message="axis1_after_pulse",
                data={
                    "direction": direction,
                    "tracking_on": tracking_on,
                    **state_after,
                    **dt_after,
                },
                run_id="ra_interval",
            )

        final_pkt = fetch_gxas_packet(ser)
        final_ra_h = gxas_ra_hours(final_pkt)
        final_lst_h = gxas_lst_hours(final_pkt)

        # Normalise to small range (no need to wrap for these small pulses)
        def _wrap_hours(x: float) -> float:
            while x < -12.0:
                x += 24.0
            while x > 12.0:
                x -= 24.0
            return x

        # When tracking is OFF, the natural quantity is HA, not RA.
        # HA = LST - RA. When tracking is ON (as in ASCOM Conform), RA change
        # is what we care about.
        if tracking_on:
            delta_ra_h = final_ra_h - initial_ra_h
            delta_ra_arcsec = delta_ra_h * 15.0 * 3600.0
            print(
                f"RA change {direction.upper()} (track on): "
                f"{delta_ra_h:+.6f} h = {delta_ra_arcsec:+.2f}\""
            )
            return delta_ra_arcsec
        else:
            initial_ha_h = _wrap_hours(initial_lst_h - initial_ra_h)
            final_ha_h = _wrap_hours(final_lst_h - final_ra_h)
            delta_ha_h = _wrap_hours(final_ha_h - initial_ha_h)
            delta_ha_arcsec = delta_ha_h * 15.0 * 3600.0
            print(
                f"HA change {direction.upper()} (track off): "
                f"{delta_ha_h:+.6f} h = {delta_ha_arcsec:+.2f}\""
            )
            return delta_ha_arcsec

    # Tracking OFF measurements (pure guiding): measure HA changes
    print("\n--- Tracking OFF (pure guiding, HA) ---")
    ha_e_off = measure_ra_delta("e", tracking_on=False)
    ha_w_off = measure_ra_delta("w", tracking_on=False)

    log_debug(
        hypothesis_id="H_RA_sym",
        message="ha_guiding_track_off",
        data={
            "guide_rate": confirmed,
            "duration_ms": duration_ms,
            "ha_east_arcsec": ha_e_off,
            "ha_west_arcsec": ha_w_off,
        },
        run_id="ra_symmetry",
    )

    # Tracking ON measurements (guiding + tracking): measure RA changes
    print("\n--- Tracking ON (guiding + tracking) ---")
    ra_e_on = measure_ra_delta("e", tracking_on=True)
    ra_w_on = measure_ra_delta("w", tracking_on=True)

    log_debug(
        hypothesis_id="H_RA_sym",
        message="ra_guiding_track_on",
        data={
            "guide_rate": confirmed,
            "duration_ms": duration_ms,
            "ra_east_arcsec": ra_e_on,
            "ra_west_arcsec": ra_w_on,
        },
        run_id="ra_symmetry",
    )


def sync_mount_to_zenith(ser: serial.Serial) -> None:
    """
    Sync the mount so that the current mechanical pointing is treated as the zenith.

    Zenith has Dec = site latitude and RA = local sidereal time. We:
      - Query site latitude via :Gt#
      - Query LST via :GS#
      - Set target Dec via :Sd...
      - Set target RA via :Sr...
      - Sync EQ via :CM#
    """
    # Get latitude (Dec of zenith)
    lat = send_command(ser, "Gt", expect_reply=True, timeout=2.0)
    if not lat:
        raise RuntimeError("Empty reply from :Gt# when syncing to zenith.")

    # Get sidereal time (RA of zenith)
    lst = send_command(ser, "GS", expect_reply=True, timeout=2.0)
    if not lst:
        raise RuntimeError("Empty reply from :GS# when syncing to zenith.")

    print(f"Syncing mount to zenith: lat={lat}, LST={lst}")

    # Set target Dec to latitude (format from :Gt# is accepted by :Sd)
    for body in (f"Sd{lat}",):
        cmd = f":{body}#".encode("ascii")
        ser.write(cmd)
        ser.flush()
        ack = ser.read(1)
        if not ack:
            print(f"Warning: no reply from :{body}#", file=sys.stderr)

    # Set target RA to LST (format from :GS# is accepted by :Sr)
    for body in (f"Sr{lst}",):
        cmd = f":{body}#".encode("ascii")
        ser.write(cmd)
        ser.flush()
        ack = ser.read(1)
        if not ack:
            print(f"Warning: no reply from :{body}#", file=sys.stderr)

    # Sync EQ to current target via :CM#. Reply is "N/A#" or nothing; ignore timeout.
    try:
        _ = send_command(ser, "CM", expect_reply=True, timeout=2.0)
    except TimeoutError:
        # Some firmware builds may not send a reply to :CM#; that's acceptable here.
        pass


def enable_tracking(ser: serial.Serial) -> None:
    """Turn tracking ON via :Te# (TeenAstro extension), ignoring reply."""
    try:
        send_command(ser, "Te", expect_reply=False)
    except Exception as exc:
        print(f"Warning: failed to send :Te# for tracking ON: {exc}", file=sys.stderr)


def disable_tracking(ser: serial.Serial) -> None:
    """Turn tracking OFF via :Td# (TeenAstro extension), ignoring reply."""
    try:
        send_command(ser, "Td", expect_reply=False)
    except Exception as exc:
        print(f"Warning: failed to send :Td# for tracking OFF: {exc}", file=sys.stderr)


def build_mainunit() -> None:
    """Build TeenAstro MainUnit firmware via build_firmware.py (target=main)."""
    cmd = [sys.executable, str(REPO_ROOT / "build_firmware.py"), "--target", "main"]
    print("Building TeenAstro MainUnit firmware...")
    result = subprocess.run(cmd, cwd=str(REPO_ROOT))
    if result.returncode != 0:
        raise RuntimeError("MainUnit build failed.")


def _find_teensy_tools_dir() -> Path | None:
    """Locate teensy_post_compile.exe in the TeenAstroUploader tree."""
    uploader_root = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader"
    # 1) Common bin output locations
    candidates = [
        uploader_root / "bin" / "Release",
        uploader_root / "bin" / "Debug",
        uploader_root,
    ]
    for d in candidates:
        exe = d / "teensy_post_compile.exe"
        if exe.is_file():
            return d
    # 2) Fallback: scan a couple of levels under uploader_root
    if uploader_root.is_dir():
        for sub in uploader_root.rglob("teensy_post_compile.exe"):
            return sub.parent
    return None


def flash_mainunit_and_wait(wait_seconds: float = 10.0) -> None:
    """
    Flash the MainUnit firmware using teensy_post_compile.exe, then wait.

    Expects:
      - Hex in TeenAstroUploader/TeenAstroUploader/<FW_RELEASE>_latest/
      - teensy_post_compile.exe in TeenAstroUploader/TeenAstroUploader/bin/*
    """
    tools_dir = _find_teensy_tools_dir()
    if tools_dir is None:
        print("teensy_post_compile.exe not found; skipping flash.", file=sys.stderr)
        return

    hex_dir = REPO_ROOT / "TeenAstroUploader" / "TeenAstroUploader" / f"{FW_RELEASE}_latest"
    hex_path = hex_dir / f"{DEFAULT_MAIN_HEX_BASE}.hex"
    if not hex_path.is_file():
        raise FileNotFoundError(f"Hex file not found: {hex_path}")

    teensy_post = tools_dir / "teensy_post_compile.exe"
    base_args = [
        str(teensy_post),
        f"-file={DEFAULT_MAIN_HEX_BASE}",
        f"-path={str(hex_dir)}",
        f"-tools={str(tools_dir)}",
        f"-board={DEFAULT_MAIN_BOARD}",
    ]

    print(f"Flashing {hex_path} ...")
    # First invocation (no reboot), like TeenAstroUploader
    subprocess.run(base_args, check=True)
    # Second invocation with reboot
    subprocess.run(base_args + ["-reboot"], check=True)

    print(f"Waiting {wait_seconds:.1f}s after flashing...")
    time.sleep(wait_seconds)


def run_pulseguide_test(
    ser: serial.Serial,
    direction: str,
    duration_ms: int,
    poll_interval: float,
    extra_timeout: float,
    guide_rate_multiple: float,
) -> dict | None:
    direction = direction.lower()
    if direction not in ("n", "s", "e", "w"):
        raise ValueError("Direction must be one of: n, s, e, w.")

    # Ensure mount is synced and tracking at a well-defined position (zenith),
    # then turn tracking on (recommended for realistic behaviour).
    sync_mount_to_zenith(ser)
    enable_tracking(ser)
    time.sleep(0.5)

    print("Fetching initial GXAS state...")
    initial_pkt = fetch_gxas_packet(ser)
    initial_pg = gxas_pulse_guiding(initial_pkt)
    # For N/S pulses we can also measure Dec offset
    initial_dec_deg = gxas_dec_deg(initial_pkt)
    print(f"Initial PulseGuiding = {initial_pg}")

    # Debug: capture raw axis target/pos in steps before pulse
    before_target_a2 = before_pos_a2 = 0
    before_target_a1 = before_pos_a1 = 0
    if direction in ("n", "s"):
        try:
            # :GXDP3# -> axis 2 target, :GXDP1# -> axis 2 pos
            before_target_a2 = int(send_command(ser, "GXDP3", expect_reply=True, timeout=2.0))
            before_pos_a2 = int(send_command(ser, "GXDP1", expect_reply=True, timeout=2.0))
        except Exception:
            pass
    elif direction in ("e", "w"):
        try:
            # :GXDP2# -> axis 1 target, :GXDP0# -> axis 1 pos
            before_target_a1 = int(send_command(ser, "GXDP2", expect_reply=True, timeout=2.0))
            before_pos_a1 = int(send_command(ser, "GXDP0", expect_reply=True, timeout=2.0))
        except Exception:
            pass

    body = f"Mg{direction}{duration_ms}"
    print(f"\nSending pulse guide command :{body}#")
    t0 = time.time()
    utc0 = datetime.utcnow().isoformat(timespec="milliseconds") + "Z"
    send_command(ser, body, expect_reply=False)
    print(f"Command sent at {utc0}")

    # Immediately after issuing the pulse, capture guiding diagnostics
    # to see what absRate / amount are actually used during this run.
    diag_after = query_dec_guiding_diagnostics(ser) if direction in ("n", "s") else {}
    if diag_after:
        log_debug(
            hypothesis_id="H5_H6",
            message="dec_guiding_diagnostics_after_Mg",
            data={
                "guide_rate": guide_rate_multiple,
                **diag_after,
            },
            run_id="pulse_sweep",
        )

    seen_start = False
    start_t = None
    end_t = None

    # Poll GXAS until PulseGuiding goes true then false, or we hit timeout
    max_elapsed = duration_ms / 1000.0 + extra_timeout
    print("\nElapsed(s)  PulseGuiding")
    while True:
        now = time.time()
        elapsed = now - t0
        try:
            pkt = fetch_gxas_packet(ser)
            pg = gxas_pulse_guiding(pkt)
        except Exception as exc:
            print(f"{elapsed:8.3f}  ERROR reading GXAS: {exc}", file=sys.stderr)
            pg = False

        print(f"{elapsed:8.3f}  {pg}")
        if pg and not seen_start:
            seen_start = True
            start_t = now
        if seen_start and not pg:
            end_t = now
            break
        if elapsed > max_elapsed:
            print(f"\nTimeout reached ({max_elapsed:.1f}s). PulseGuiding still {pg}.")
            break
        time.sleep(poll_interval)

    print("\n=== Summary ===")
    print(f"Requested pulse duration: {duration_ms} ms")

    if not seen_start:
        print("PulseGuiding flag never became True in GXAS.")
        return None

    if end_t is None:
        print("PulseGuiding flag did not return to False before timeout.")
        return None

    on_time = (end_t - start_t) * 1000.0
    total_from_command = (end_t - t0) * 1000.0

    print(f"Measured PulseGuiding ON-time: {on_time:.1f} ms")
    print(f"From command send -> PulseGuiding False: {total_from_command:.1f} ms")
    print(f"Excess over requested duration: {on_time - duration_ms:.1f} ms")

    # If we are guiding in Dec (north/south), measure the actual Dec offset in arcseconds.
    if direction in ("n", "s"):
        # Fetch a fresh GXAS snapshot after guiding has finished
        final_pkt = fetch_gxas_packet(ser)
        final_dec_deg = gxas_dec_deg(final_pkt)
        delta_dec_arcsec = (final_dec_deg - initial_dec_deg) * 3600.0

        # Expected magnitude: 15 arcsec/s * guide_rate_multiple * duration_s
        duration_s = duration_ms / 1000.0
        expected_arcsec_mag = 15.0 * guide_rate_multiple * duration_s

        # Debug: capture Dec-axis target/pos in steps after pulse
        after_target_a2 = after_pos_a2 = 0
        try:
            after_target_a2 = int(send_command(ser, "GXDP3", expect_reply=True, timeout=2.0))
            after_pos_a2 = int(send_command(ser, "GXDP1", expect_reply=True, timeout=2.0))
        except Exception:
            pass

        print("\n=== Distance check (Dec axis) ===")
        print(f"Initial Dec: {initial_dec_deg:.6f} deg")
        print(f"Final   Dec: {final_dec_deg:.6f} deg")
        print(f"Measured Dec offset: {delta_dec_arcsec:.2f} arcsec")
        print(f"Expected magnitude (guide_rate={guide_rate_multiple}x): {expected_arcsec_mag:.2f} arcsec")
        diff_arcsec = abs(delta_dec_arcsec) - expected_arcsec_mag
        print(f"Difference (|measured|-expected): {diff_arcsec:.2f} arcsec")

        # Debug log for hypotheses H1/H2/H4: does Dec-axis target / pos move as expected?
        log_debug(
            hypothesis_id="H1_H2_H4",
            message="pulse_guide_dec_result",
            data={
                "guide_rate": guide_rate_multiple,
                "duration_ms": duration_ms,
                "initial_dec_deg": initial_dec_deg,
                "final_dec_deg": final_dec_deg,
                "measured_dec_arcsec": delta_dec_arcsec,
                "expected_dec_arcsec": expected_arcsec_mag,
                "before_target_a2": before_target_a2,
                "before_pos_a2": before_pos_a2,
                "after_target_a2": after_target_a2,
                "after_pos_a2": after_pos_a2,
                "target_a2_delta": after_target_a2 - before_target_a2,
                "pos_a2_delta": after_pos_a2 - before_pos_a2,
            },
            run_id="pulse_sweep",
        )

        return {
            "guide_rate": guide_rate_multiple,
            "duration_ms": duration_ms,
            "measured_arcsec": delta_dec_arcsec,
            "expected_arcsec": expected_arcsec_mag,
            "difference_arcsec": diff_arcsec,
        }

    # For E/W pulses, log RA-axis step deltas even though we don't yet convert
    # them to sky arcseconds in this helper.
    if direction in ("e", "w"):
        after_target_a1 = after_pos_a1 = 0
        try:
            after_target_a1 = int(send_command(ser, "GXDP2", expect_reply=True, timeout=2.0))
            after_pos_a1 = int(send_command(ser, "GXDP0", expect_reply=True, timeout=2.0))
        except Exception:
            pass
        log_debug(
            hypothesis_id="H_RA",
            message="pulse_guide_ra_steps",
            data={
                "guide_rate": guide_rate_multiple,
                "duration_ms": duration_ms,
                "before_target_a1": before_target_a1,
                "before_pos_a1": before_pos_a1,
                "after_target_a1": after_target_a1,
                "after_pos_a1": after_pos_a1,
                "target_a1_delta": after_target_a1 - before_target_a1,
                "pos_a1_delta": after_pos_a1 - before_pos_a1,
            },
            run_id="pulse_sweep",
        )

    return None


def run_guide_rate_sweep(
    ser: serial.Serial,
    direction: str,
    duration_ms: int,
    poll_interval: float,
    extra_timeout: float,
) -> None:
    """
    Sweep several values of guide rate 0:
    - Read current :GXR0#
    - For each test rate, set :SXR0,NN#, perform a pulse guide, and verify displacement.
    - Restore original guide rate 0 at the end.

    For N/S (Dec) pulses, this computes and prints sky offsets.
    For E/W (RA) pulses, it logs internal axis-1 step deltas (H_RA) only.
    """
    if direction not in ("n", "s", "e", "w"):
        raise ValueError("Direction must be one of: n, s, e, w.")

    # Sync once to zenith before starting the sweep so that Dec measurements
    # are taken from a well-defined sky position.
    sync_mount_to_zenith(ser)

    original_rate = query_guide_rate0(ser)
    print(f"Original guide rate 0 (GXR0): {original_rate:.4f}x")

    # Define a set of test rates including the original
    candidate_rates = [original_rate, 0.05, 0.1, 0.25, 0.5, 1.0]
    # Deduplicate while preserving order and staying within allowed range
    seen = set()
    test_rates: list[float] = []
    for r in candidate_rates:
        if r <= 0.0 or r >= 2.55:
            continue
        key = round(r, 4)
        if key not in seen:
            seen.add(key)
            test_rates.append(r)

    print(f"Testing guideRates[0] values: {', '.join(f'{r:.4f}' for r in test_rates)}")

    results: list[dict] = []
    try:
        for r in test_rates:
            print("\n----------------------------------------")
            print(f"Setting guide rate 0 to {r:.4f}x via :SXR0#")
            set_guide_rate0(ser, r)
            # Small delay to let firmware apply rate
            time.sleep(0.5)
            # Verify by reading back
            confirmed = query_guide_rate0(ser)
            print(f"Confirmed GXR0: {confirmed:.4f}x")

            # Capture internal guiding diagnostics on Dec axis before the pulse.
            diag = query_dec_guiding_diagnostics(ser)
            log_debug(
                hypothesis_id="H5_H6",
                message="dec_guiding_diagnostics_before_pulse",
                data={
                    "requested_GXR0": r,
                    "confirmed_GXR0": confirmed,
                    **diag,
                },
                run_id="pulse_sweep",
            )

            res = run_pulseguide_test(
                ser,
                direction=direction,
                duration_ms=duration_ms,
                poll_interval=poll_interval,
                extra_timeout=extra_timeout,
                guide_rate_multiple=confirmed,
            )
            if res is not None:
                results.append(res)
    finally:
        # Restore original rate
        try:
            print("\nRestoring original guide rate 0...")
            set_guide_rate0(ser, original_rate)
            time.sleep(0.5)
            restored = query_guide_rate0(ser)
            print(f"Restored GXR0: {restored:.4f}x")
        except Exception as exc:
            print(f"Warning: failed to restore original guide rate 0: {exc}", file=sys.stderr)

    # Summarize
    if results:
        print("\n=== Guide rate sweep summary ===")
        for r in results:
            print(
                f"rate={r['guide_rate']:.4f}x, "
                f"measured={r['measured_arcsec']:.2f}\" "
                f"expected={r['expected_arcsec']:.2f}\" "
                f"diff={r['difference_arcsec']:.2f}\""
            )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Direct TeenAstro main-unit pulse-guiding performance tester."
    )
    parser.add_argument(
        "--port",
        required=True,
        help="Serial port of TeenAstro main unit (e.g. COM3, COM4).",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=57600,
        help="Baud rate (default: 57600).",
    )
    parser.add_argument(
        "--direction",
        "-d",
        choices=["n", "s", "e", "w"],
        default="n",
        help="Guiding direction: n, s, e, or w (default: n = north).",
    )
    parser.add_argument(
        "--duration",
        "-t",
        type=int,
        default=1000,
        help="Pulse duration in milliseconds (default: 1000).",
    )
    parser.add_argument(
        "--poll-interval",
        "-p",
        type=float,
        default=0.1,
        help="GXAS polling interval in seconds (default: 0.1).",
    )
    parser.add_argument(
        "--extra-timeout",
        type=float,
        default=10.0,
        help="Extra seconds beyond pulse duration before giving up (default: 10.0).",
    )
    parser.add_argument(
        "--guide-rate-multiple",
        type=float,
        default=1.0,
        help="Guide rate as a multiple of sidereal (default: 1.0x). Used for distance check.",
    )
    parser.add_argument(
        "--build-flash",
        action="store_true",
        help="Build MainUnit firmware and flash via teensy_post_compile.exe before testing.",
    )
    parser.add_argument(
        "--rate-sweep",
        action="store_true",
        help="Sweep several values of guide rate 0 (GXR0/SXR0) and verify displacement.",
    )
    parser.add_argument(
        "--ra-e-w-test",
        action="store_true",
        help="Run RA East/West symmetry test with tracking OFF and ON.",
    )
    parser.add_argument(
        "--all-tests",
        action="store_true",
        help="Run RA E/W (HA+RA) and Dec N/S tests in a single run.",
    )

    args = parser.parse_args()

    try:
        if args.build_flash:
            try:
                build_mainunit()
                flash_mainunit_and_wait(wait_seconds=10.0)
            except Exception as exc:
                print(f"Build/flash step failed: {exc}", file=sys.stderr)

        with serial.Serial(
            port=args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1,
        ) as ser:
            print(f"Opened {ser.port} @ {args.baud} baud.")
            # Small delay to let the mount settle after opening the port / potential reset
            time.sleep(1.0)

            # Capture and log motor geometry once at start so we can verify
            # that RA and Dec guiding amounts are theoretically identical.
            geom = query_motor_geometry(ser)
            print("Motor geometry (RA/Dec):")
            print(json.dumps(geom, indent=2))

            if args.all_tests:
                # 1) RA East/West: tracking OFF → HA, tracking ON → RA
                run_ra_e_w_test(
                    ser,
                    duration_ms=args.duration,
                    guide_rate=args.guide_rate_multiple,
                )
                # 2) Dec North/South: tracking ON (uses run_pulseguide_test)
                print("\n=== Dec N/S test (tracking ON) ===")
                for d in ("n", "s"):
                    print(f"\n--- Dec pulse {d.upper()} ---")
                    run_pulseguide_test(
                        ser,
                        direction=d,
                        duration_ms=args.duration,
                        poll_interval=args.poll_interval,
                        extra_timeout=args.extra_timeout,
                        guide_rate_multiple=args.guide_rate_multiple,
                    )
            elif args.ra_e_w_test:
                run_ra_e_w_test(
                    ser,
                    duration_ms=args.duration,
                    guide_rate=args.guide_rate_multiple,
                )
            elif args.rate_sweep:
                run_guide_rate_sweep(
                    ser,
                    direction=args.direction,
                    duration_ms=args.duration,
                    poll_interval=args.poll_interval,
                    extra_timeout=args.extra_timeout,
                )
            else:
                run_pulseguide_test(
                    ser,
                    direction=args.direction,
                    duration_ms=args.duration,
                    poll_interval=args.poll_interval,
                    extra_timeout=args.extra_timeout,
                    guide_rate_multiple=args.guide_rate_multiple,
                )
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

