#!/usr/bin/env python3
"""
Regression test for TeenAstro GOTO accuracy on RA/Dec.

Goals:
- Flash the TeenAstro main unit firmware (via PlatformIO) before the test run.
- Connect to the mount on a serial port (default: COM3).
- Command a GOTO to a given RA/Dec.
- Wait for the slew to complete using :GXAS# tracking state.
- Read both *actual* and *target* RA/Dec from the mount.
- Assert that the final actual coordinates match the target coordinates within a small tolerance.

This is intended to catch regressions where the internal GOTO path (which must
account for Earth rotation while slewing) no longer lands on the requested
equatorial coordinates.

Usage (from repository root):

  python tests/conform_reproduce/test_goto_regression.py \
      --port COM3 \
      --baud 57600 \
      --ra-hours 21.3942 \
      --dec-deg 1.0 \
      --flash

Exit status:
- 0 on success (GOTO within tolerance)
- 1 on any failure (flash, serial, timeout, or accuracy assertion)
"""

import argparse
import base64
import math
import subprocess
import sys
import time
from pathlib import Path


try:
    import serial  # type: ignore
except ImportError:
    print("pyserial is required. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def log(msg: str) -> None:
    """Simple console logger with timestamp."""
    now = time.strftime("%H:%M:%S")
    print(f"[{now}] {msg}")


def flash_firmware(env: str | None = None) -> None:
    """
    Flash the TeenAstro main unit using PlatformIO.

    By default, uses the env from TeenAstroMainUnit/platformio.ini (default_envs).
    You can override with --pio-env if needed.
    """
    repo_root = Path(__file__).resolve().parents[2]
    pio_dir = repo_root / "TeenAstroMainUnit"

    if not pio_dir.is_dir():
        raise RuntimeError(f"TeenAstroMainUnit directory not found at {pio_dir}")

    cmd = ["platformio", "run", "-t", "upload"]
    if env:
        cmd.extend(["-e", env])

    log(f"Flashing firmware with: {' '.join(cmd)} (cwd={pio_dir})")
    try:
        result = subprocess.run(
            cmd,
            cwd=str(pio_dir),
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
    except FileNotFoundError as e:
        raise RuntimeError(
            "platformio CLI not found. Please install PlatformIO (`pip install platformio` "
            "or use the VSCode/CLI integration) before running this test with --flash."
        ) from e

    print(result.stdout)
    if result.returncode != 0:
        raise RuntimeError(f"PlatformIO flash failed with exit code {result.returncode}")


def send_cmd(ser: serial.Serial, cmd: str, expect_term: bool = True) -> str:
    """
    Send an LX200/TeenAstro command and return the reply (including '#').

    If expect_term is False, reads a single byte (used for some legacy commands).
    """
    full_cmd = cmd if cmd.endswith("#") else cmd + "#"
    ser.write(full_cmd.encode("utf-8"))
    if expect_term:
        # read until '#' or timeout
        try:
            raw = ser.read_until(b"#", timeout=2)
        except TypeError:
            # Older pyserial: read_until has no timeout kwarg
            raw = ser.read_until(b"#")
    else:
        raw = ser.read(1)
    return raw.decode("utf-8", errors="replace")


def get_value(ser: serial.Serial, cmd: str) -> str:
    """Send command and return response *without* trailing '#'."""
    resp = send_cmd(ser, cmd)
    return resp.rstrip("#\r\n") if resp else ""


def parse_float(value: str) -> float:
    """Parse a float from mount reply, raising on failure."""
    try:
        return float(value.strip())
    except ValueError as e:
        raise RuntimeError(f"Cannot parse float from mount reply: {value!r}") from e


def angular_diff_deg(a_deg: float, b_deg: float) -> float:
    """
    Smallest angular separation between two angles in degrees, handling RA wrap.
    """
    diff = (a_deg - b_deg + 180.0) % 360.0 - 180.0
    return abs(diff)


def wait_for_slew_complete(
    ser: serial.Serial,
    max_wait_s: float = 120.0,
    poll_interval_s: float = 1.0,
) -> bool:
    """
    Poll :GXAS# to detect when the mount finishes slewing.
    GXAS byte 0 bits 0-1: 0=off, 1=tracking, 2=slewing.

    Returns True if slew finished within max_wait_s, False on timeout.
    """
    log("Waiting for slew to complete (polling :GXAS#)...")
    start = time.time()
    while time.time() - start < max_wait_s:
        b64 = get_value(ser, ":GXAS#")
        if not b64 or len(b64) < 136:
            log("Empty or short :GXAS# reply, continuing...")
        else:
            try:
                pkt = base64.b64decode(b64)
                trk = pkt[0] & 0x3 if len(pkt) > 0 else 0
            except Exception:
                log(f"Unexpected :GXAS# reply (decode error)")
                trk = 0
            slewing = trk == 2
            log(f":GXAS# -> trk={trk} slewing={slewing}")
            if not slewing:
                log("Slew reported complete.")
                return True
        time.sleep(poll_interval_s)
    log(f"Slew did not complete within {max_wait_s} seconds.")
    return False


def parse_hms_to_hours(hms: str) -> float:
    """Parse an HH:MM[:SS] string to decimal hours."""
    parts = hms.strip().split(":")
    if len(parts) < 2:
        raise RuntimeError(f"Cannot parse HMS from reply: {hms!r}")
    try:
        h = float(parts[0])
        m = float(parts[1])
        s = float(parts[2]) if len(parts) > 2 else 0.0
    except ValueError as e:
        raise RuntimeError(f"Cannot parse HMS from reply: {hms!r}") from e
    return h + m / 60.0 + s / 3600.0


def run_goto_regression(
    port: str,
    baud: int,
    ra_hours: float,
    dec_deg: float,
    tol_arcsec: float,
    ha_hours: float | None = None,
) -> None:
    """
    Perform a single GOTO and assert final RA/Dec are within tolerance.

    If ha_hours is provided, the script will:
    - Query the mount sidereal time (:GS#),
    - Convert the requested hour angle to RA using RA = LST - HA,
    - Then command the GOTO to that RA/Dec.
    """
    log(f"Connecting to mount on {port} @ {baud} baud...")
    ser = serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=2,
        write_timeout=2,
    )

    try:
        # Ensure tracking is ON
        log("Enabling tracking (:Te#)...")
        send_cmd(ser, ":Te#", expect_term=False)
        time.sleep(0.3)

        # Determine RA either directly (ra_hours) or from hour angle + LST
        if ha_hours is not None:
            lst_raw = get_value(ser, ":GS#")
            lst_hours = parse_hms_to_hours(lst_raw)
            ra_hours_effective = (lst_hours - ha_hours) % 24.0
            log(
                f"Using HA input: HA={ha_hours:.6f}h, LST={lst_hours:.6f}h -> "
                f"RA={ra_hours_effective:.6f}h"
            )
        else:
            ra_hours_effective = ra_hours

        ra_deg = ra_hours_effective * 15.0

        # Set target coordinates using decimal-degree LX200 extensions (:SrL / :SdL)
        sr_cmd = f":SrL{ra_deg:.5f}#"
        sd_cmd = f":SdL{dec_deg:+.5f}#" if dec_deg >= 0 else f":SdL{dec_deg:.5f}#"
        log(
            f"Setting target RA/Dec: RA={ra_hours_effective:.6f}h "
            f"({ra_deg:.5f} deg), Dec={dec_deg:+.5f} deg"
        )
        send_cmd(ser, sr_cmd, expect_term=False)
        time.sleep(0.1)
        send_cmd(ser, sd_cmd, expect_term=False)
        time.sleep(0.1)

        # Start GOTO
        log("Starting GOTO (:MS#)...")
        ser.write(b":MS#")
        try:
            raw = ser.read_until(b"#", timeout=2)
        except TypeError:
            raw = ser.read_until(b"#")
        ms_reply = raw.decode("utf-8", errors="replace").strip()
        log(f":MS# reply: {ms_reply!r}")
        if not ms_reply.startswith("0"):
            raise RuntimeError(f"GOTO rejected by mount: reply={ms_reply!r}")

        # Wait for slew completion
        if not wait_for_slew_complete(ser):
            raise RuntimeError("Timeout waiting for slew to complete")

        # Read final actual and target RA/Dec
        actual_ra_deg = parse_float(get_value(ser, ":GRL#"))
        actual_dec_deg = parse_float(get_value(ser, ":GDL#"))
        target_ra_deg = parse_float(get_value(ser, ":GrL#"))
        target_dec_deg = parse_float(get_value(ser, ":GdL#"))

        log(
            f"Final Actual RA/Dec: RA={actual_ra_deg:.6f} deg, Dec={actual_dec_deg:+.6f} deg"
        )
        log(
            f"Final Target RA/Dec: RA={target_ra_deg:.6f} deg, Dec={target_dec_deg:+.6f} deg"
        )

        # Compute residuals
        ra_err_arcsec = angular_diff_deg(actual_ra_deg, target_ra_deg) * 3600.0
        dec_err_arcsec = abs(actual_dec_deg - target_dec_deg) * 3600.0

        log(
            f"Residuals: dRA={ra_err_arcsec:.2f}\"  dDec={dec_err_arcsec:.2f}\" "
            f"(tolerance={tol_arcsec:.2f}\")"
        )

        if ra_err_arcsec > tol_arcsec or dec_err_arcsec > tol_arcsec:
            raise AssertionError(
                f"GOTO accuracy regression: RA error={ra_err_arcsec:.2f}\", "
                f"Dec error={dec_err_arcsec:.2f}\", tolerance={tol_arcsec:.2f}\""
            )

        log("GOTO regression PASSED: final RA/Dec within tolerance.")

    finally:
        try:
            ser.close()
        except Exception:
            pass


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="TeenAstro GOTO RA/Dec regression test")
    ap.add_argument("--port", default="COM3", help="Serial port (e.g. COM3, /dev/ttyUSB0)")
    ap.add_argument("--baud", type=int, default=57600, help="Serial baud rate")
    ap.add_argument(
        "--ra-hours",
        type=float,
        default=21 + 23 / 60 + 39.1 / 3600,
        help="Target RA in hours (default: 21h23m39.1s). "
        "Ignored if --ha-hours is provided.",
    )
    ap.add_argument(
        "--ha-hours",
        type=float,
        help="Target hour angle in hours (if set, overrides --ra-hours). "
        "HA is defined as HA = LST - RA.",
    )
    ap.add_argument(
        "--dec-deg",
        type=float,
        default=1.0,
        help="Target Dec in degrees (default: +1.0)",
    )
    ap.add_argument(
        "--tol-arcsec",
        type=float,
        default=30.0,
        help="Maximum allowed RA/Dec error at end of GOTO (arcseconds)",
    )
    ap.add_argument(
        "--flash",
        action="store_true",
        help="Flash TeenAstro firmware via PlatformIO before running the test",
    )
    ap.add_argument(
        "--pio-env",
        metavar="ENV",
        help="Optional PlatformIO environment name (e.g. 240_5160). "
        "If omitted, uses default_envs from platformio.ini.",
    )

    args = ap.parse_args(argv)

    try:
        if args.flash:
            flash_firmware(env=args.pio_env)
            # Give the Teensy / mount time to reboot and enumerate the serial port
            log("Flash complete. Waiting 30 seconds before connecting to mount...")
            time.sleep(30.0)
        run_goto_regression(
            port=args.port,
            baud=args.baud,
            ra_hours=args.ra_hours,
            dec_deg=args.dec_deg,
            tol_arcsec=args.tol_arcsec,
            ha_hours=args.ha_hours,
        )
    except Exception as e:
        log(f"TEST FAILED: {e}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

