# Tracking drift test

This document describes how to verify that the mount tracks without RA drift when running sidereal tracking (no alignment correction, no refraction).

## Background

RA is computed as **RA = LST − HA**. During sidereal tracking, the motor advances HA at the same rate as LST, so RA should stay constant. If the main loop is blocked (e.g. by command processing or the focuser `delay(20)` in `:GXAS#`), multiple sidereal ticks can occur before `onSiderealTick()` runs. The firmware now compensates by applying **target += fstep × elapsed** so all missed ticks are applied in one go.

## Script: `tracking_drift_monitor.py`

A Python script that connects to the mount via TCP (LX200 protocol) and records RA, LST, tracking rates, and missed-tick count over time.

### Requirements

- Python 3
- Mount powered on, on WiFi, and **tracking** (sidereal, no refraction/alignment)

### Usage

```bash
python tests/tracking_drift_monitor.py --host <MOUNT_IP> [--port 9999] [--duration 300] [--interval 2.0]
```

| Option      | Default | Description                    |
|------------|---------|--------------------------------|
| `--host`   | (required) | Mount IP or hostname        |
| `--port`   | 9999    | TCP port (LX200)              |
| `--duration` | 300   | Monitoring duration (seconds) |
| `--interval` | 2.0   | Poll interval (seconds)       |

Example:

```bash
python tests/tracking_drift_monitor.py --host 192.168.1.28 --duration 300 --interval 2.0
```

### Commands used

- **:GRL#** – RA in decimal degrees (stable, no precision toggle)
- **:GSL#** – LST in decimal hours
- **:GXDW1#** – Missed sidereal ticks count (debug)
- **:GXDW2#** – Reset missed ticks counter at run start

### Output

- Live line each poll: RA (HH:MM:SS.ss), LST, drift vs first sample, rate, missed ticks
- At the end: summary with total drift (arcsec), drift rate (arcsec/min), jitter, and missed ticks

### Interpreting results

- **No significant drift:** Drift rate &lt; ~0.1 arcsec/min and small jitter → tracking is correct.
- **Missed ticks:** If the script or app polls often (e.g. `:GXAS#` with focuser), many ticks can be missed; the firmware compensates, so RA should still be stable.
- **High drift rate:** If drift rate is large and persistent, investigate tracking rate, refraction, or alignment settings.

### Log file

Sample data is appended in NDJSON form to `debug-adb7d4.log` (or the log path set for the debug session) for later analysis.

## Firmware debug commands

- **:GXDW1#** – Get accumulated missed sidereal ticks since last reset
- **:GXDW2#** – Reset missed ticks counter

These are intended for verification and can be removed or kept as optional diagnostics.
