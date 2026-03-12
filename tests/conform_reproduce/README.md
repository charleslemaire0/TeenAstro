# ASCOM Conform Sequence Reproduce Test

Reproduces the command sequence the ASCOM conformance checker sends before SlewToCoordinates, and logs how the main unit responds.

## Usage

```bash
# List available serial ports
python ascom_conform_sequence.py --list-ports

# Run on COM3 (default) at 57600 baud
python ascom_conform_sequence.py --port COM3 --baud 57600

# EXACT conformer reproduction (from ASCOM logs at C:\Users\...\ASCOM)
python ascom_conform_sequence.py --port COM3 --exact-conformer -o ascom_conform_log.txt

# FULL conformer: + Site, SlewSettleTime, UTCDate +1hr then revert (may reproduce GOTO failure)
python ascom_conform_sequence.py --port COM3 --full-conformer -o ascom_conform_log.txt

# Run on TCP (WiFi)
python ascom_conform_sequence.py --tcp 192.168.0.21

# Custom GOTO target (RA hours, Dec degrees)
python ascom_conform_sequence.py --ra 21.39 --dec 1.0

# Custom log file
python ascom_conform_sequence.py -o my_log.txt
```

## What it does

1. **Tracking ON** (`:Te#`)
2. **RightAscensionRate** (`:SXRr,val#`) – 0, +0.003, -0.003, +2.667, -2.667, 0
3. **DeclinationRate** (`:SXRd,val#`) – 0, +0.05, -0.05, +40, -40, 0
4. **TrackingRate** – Sidereal (`:TQ#`), Lunar (`:TL#`), Solar (`:TS#`), Sidereal (`:TQ#`)
5. **GOTO** – Set target (`:SrL...`, `:SdL...`), slew (`:MS#`), poll until done

## `--exact-conformer` (from ASCOM logs)

Matches the actual conformer sequence from ASCOM logs:

- **DeclinationRate before RightAscensionRate** (conformer order)
- **RA target = SiderealTime** (`:GSL#`) – conformer slews to meridian (RA = LST)
- **Tracking Off/On** (`:Td#`, wait 1s, `:Te#`) before slew
- **GXAS read-backs** after each rate set
- **Sd before Sr** (Dec before RA)
- **GXAS polling** every 250ms during slew

## `--full-conformer` (from ASCOM logs – Site, SlewSettleTime, UTCDate)

Adds what the conformer does before SlewToCoordinates:

- **SiteElevation** – Get (:Ge#), Set (:Se+00059#)
- **SiteLatitude / SiteLongitude** – Get (:Gtf#, :Ggf#), Set (:St+47:00:00#, :Sg+001:33:04#)
- **SlewSettleTime** – Get (:GXOS#), Set (:SXOS,1#)
- **UTCDate** – Get from GXAS, Set +1hr via :SXT2, ts#, then restore via :SXT2, ts#

Run with `--full-conformer` to try to reproduce the GOTO failure seen in the conformer.

## Output

Logs to `ascom_conform_log.txt` (or `-o` path). Each line is timestamped. Use this log to see how the mount responds to each command and whether GOTO completes.

## Dependencies

```bash
pip install pyserial
```
