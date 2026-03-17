# Cockpit SDL Pixel-Budget Audit

**File:** `TeenAstroEmulator/emu/cockpit_sdl.h`  
**Layout rules:**
- CW = 900 px, CH = 700 px
- CHAR_W = 12 px, CHAR_H = 14 px, LINE_H = 20 px
- PAD = 12 px, TAB_H = 30 px
- VAL_X = 282 px (value start in drawKV)
- Label start = PAD + 8 = 20 px
- Max label length: 21 chars (262 px / 12)
- Max value length: 51 chars (618 px / 12)
- Tab width = textWidth(name) + 20, gap = 2 px

---

## 1. Tab Bar (g_tabNames)

| Tab         | Chars | Width (text+20) |
|-------------|-------|-----------------|
| Overview    | 8     | 116             |
| Park/Home   | 9     | 128             |
| Tracking    | 8     | 116             |
| Guiding     | 7     | 104             |
| Axes        | 4     | 68              |
| Alignment   | 9     | 128             |
| Site/Limits | 10    | 140             |

**Total:** 116+128+116+104+68+128+140 = 800 px  
**Gaps (6×2):** 12 px  
**Total tab bar:** 812 px  
**Available:** 900 px  
**Status:** OK (88 px margin)

---

## 2. drawKV Labels (max 21 chars)

All labels checked. Longest: "Coordinate Conversion (CoordConv)" is a section title, not a drawKV label.

Longest drawKV labels:
- "Requested DEC Rate" (18)
- "A1 Requested Rate" (18)
- "A2 Requested Rate" (18)
- "Pole Tracking Dist" (18)
- "Axis 2 (DEC / Altitude)" — section title, 22 chars

**Status:** All drawKV labels ≤ 21 chars. OK.

---

## 3. drawKV Values (max 51 chars)

### Overview tab
| Label            | Value source / format                         | Worst-case chars | Width | Status |
|------------------|------------------------------------------------|------------------|-------|--------|
| Mount Type       | mtNames[]                                      | 9 ("Fork-Alt")   | 108   | OK     |
| Meridian Flip    | flipNames[]                                    | 6 ("Always")     | 72    | OK     |
| Push-To          | ptNames[]                                      | 7 ("Alt/Az")     | 84    | OK     |
| Focuser          | "Connected"/"None"                             | 9                | 108   | OK     |
| GNSS             | "Connected"/"None"                             | 9                | 108   | OK     |
| Tracking         | "%s  (Mode: %s)"                               | ~20              | 240   | OK     |
| Park Status      | parkNames[]                                    | 9 ("Parking...") | 108   | OK     |
| GoTo State       | gotoNames[]                                    | 11 ("GoTo AltAz")| 132   | OK     |
| Guiding          | guidNames[]                                    | 9 ("Recenter")   | 108   | OK     |
| Last Error       | errNames[]                                     | 10 ("Meridian")  | 120   | OK     |
| Pole Side        | poleNames[]                                    | 9 ("Not Valid")  | 108   | OK     |
| Target RA (deg)  | "%.6f"                                         | ~14              | 168   | OK     |
| Target DEC (deg) | "%.6f"                                         | ~14              | 168   | OK     |
| Current Alt      | "%.4f"                                         | ~12              | 144   | OK     |
| Current Azm      | "%.4f"                                         | ~12              | 144   | OK     |
| LST              | fmtHMS                                         | ~14              | 168   | OK     |

### Park/Home tab
| Label           | Value source / format                         | Worst-case chars | Status |
|-----------------|------------------------------------------------|------------------|--------|
| Status          | parkNames[]                                    | 9                | OK     |
| Park Saved      | "YES"/"NO"                                     | 3                | OK     |
| Park Position   | "A1: %ld steps   A2: %ld steps"                | 42               | OK     |
| Park Position   | "(last parked pos in EEPROM)"                  | 27               | OK     |
| Park Position   | "Not saved"                                    | 9                | OK     |
| Backlash Phase  | blNames[]                                      | 4 ("Done")       | OK     |
| Settle Duration | "%u ms"                                        | ~8               | OK     |
| Settling        | "YES"/"no"                                     | 3                | OK     |
| Home Saved      | "YES"/"NO"                                     | 3                | OK     |
| At Home         | "YES"/"no"                                     | 3                | OK     |
| Home Mount      | "YES"/"no"                                     | 3                | OK     |
| Target RA/DEC   | fmtDeg                                         | ~17              | OK     |
| Target Alt/Azm  | fmtDeg                                         | ~17              | OK     |
| Target Pole Side| poleNames[]                                    | 9                | OK     |

### Tracking tab
| Label               | Value source / format              | Worst-case chars | Status |
|---------------------|------------------------------------|------------------|--------|
| Enabled             | "YES"/"NO"                         | 3                | OK     |
| Last State          | "ON"/"OFF"                         | 3                | OK     |
| Mode                | modes[]                            | 6 ("Target")     | OK     |
| Sidereal Clock      | "%.4f"                             | ~12              | OK     |
| Compensation        | tcNames[]                          | 8 ("RA+DEC")     | OK     |
| Requested HA Rate    | "%.6f arcsec/s"                    | ~23              | OK     |
| Requested DEC Rate   | "%.6f arcsec/s"                    | ~23              | OK     |
| Stored RA Rate      | "%ld"                              | 10               | OK     |
| Stored DEC Rate     | "%ld"                              | 10               | OK     |
| A1/A2 Current Rate  | "%.6f"                             | ~14              | OK     |
| A1/A2 Requested Rate| "%.6f"                             | ~14              | OK     |
| GoTo State          | gotoNames[]                        | 11               | OK     |
| Abort Slew          | "ACTIVE"/"no"                      | 6                | OK     |
| Spiral Active       | "YES"/"no"                         | 3                | OK     |
| Spiral FOV          | "%.2f deg"                         | ~16              | OK     |

### Guiding tab
| Label           | Value source / format                    | Worst-case chars | Status |
|-----------------|------------------------------------------|------------------|--------|
| State           | guidNames[]                              | 9                | OK     |
| Last State      | guidNames[]                              | 9                | OK     |
| Active Rate     | "%s  (index %d)"                         | ~23              | OK     |
| Recenter Rate   | "Index %d"                               | ~13              | OK     |
| Pulse Guide Rate| "%.6f"                                   | ~14              | OK     |
| Rate[0-4]       | "%.6f"                                   | ~14              | OK     |
| Accel Distance  | "%.4f deg"                               | ~20              | OK     |
| Status          | "BUSY"/"Idle"                            | 4                | OK     |
| Direction       | "FW"/"BW"/"-"                            | 2                | OK     |
| Braking         | "YES"/"no"                               | 3                | OK     |
| Abs Rate        | "%.6f"                                   | ~14              | OK     |
| Eff Rate        | "%.6f"                                   | ~14              | OK     |
| Duration        | "%lu ms"                                 | ~13              | OK     |

### Axes tab
| Label               | Value source / format                         | Worst-case chars | Status |
|---------------------|------------------------------------------------|------------------|--------|
| Position (steps)    | "%ld"                                          | 10               | OK     |
| Target (steps)      | "%.1f"                                         | ~12              | OK     |
| Delta to Target     | "%ld"                                          | 10               | OK     |
| Step Interval       | "%.4f"                                         | ~12              | OK     |
| Sidereal Interval   | "%.4f"                                         | ~12              | OK     |
| Acceleration        | "%.4f"                                         | ~12              | OK     |
| Frac Steps/Tick     | "%.6f"                                         | ~14              | OK     |
| Enabled             | "YES"/"NO"                                     | 3                | OK     |
| Fault               | "FAULT"/"OK"                                   | 5                | OK     |
| Direction           | "FW (+)"/"BW (-)"                             | 7                | OK     |
| Backlash Moved/In   | "%d / %d"                                      | ~15              | OK     |
| Backlash Active     | "YES"/"no"                                     | 3                | OK     |
| Steps/Rotation      | "%ld"                                          | 10               | OK     |
| Steps/Degree        | "%.4f"                                         | ~12              | OK     |
| Steps/ArcSec        | "%.6f"                                         | ~14              | OK     |
| Limits (steps)      | "%ld .. %ld"                                  | ~24              | OK     |
| Limits (deg)        | "%.2f .. %.2f"                                | ~28              | OK     |
| Pole Def (steps)    | "%ld"                                          | 10               | OK     |
| Home Def (steps)    | "%ld"                                          | 10               | OK     |
| Half Rotation       | "%ld"                                          | 10               | OK     |
| Motors Enabled      | "YES"/"NO"                                     | 3                | OK     |
| **A1 Mechanics**    | "Gear:%lu  Steps:%u  uStep:%d"                | ~43              | OK     |
| **A1 Settings**     | "Rev:%s  Silent:%s  Hi:%umA  Lo:%umA"         | ~39              | OK     |
| A2 Mechanics        | same as A1                                    | ~43              | OK     |
| A2 Settings         | same as A1                                    | ~39              | OK     |
| **Backlash Config** | "A1:%d (rate %d)  A2:%d (rate %d)"           | **51**           | **TIGHT** |
| Reboot Pending      | "YES"/"no"                                     | 3                | OK     |

### Alignment tab
| Label             | Value source / format                    | Worst-case chars | Status |
|-------------------|------------------------------------------|------------------|--------|
| Valid             | "YES"/"NO"                               | 3                | OK     |
| Max Align Stars   | "%d"                                     | 2                | OK     |
| Auto by Sync      | "YES"/"no"                               | 3                | OK     |
| Phase             | phaseNames[]                             | 9 ("Recenter")   | OK     |
| Current Star #    | "%d"                                     | 2                | OK     |
| Star Name         | alignStarName[16] or "(none)"            | 15               | OK     |
| Ready             | "YES"/"NO"                               | 3                | OK     |
| Reference Stars   | "%d"                                     | 2                | OK     |
| Alignment Error   | "%.8f rad"                               | ~20              | OK     |
| Alignment Error   | "N/A (not ready)"                        | 15               | OK     |
| Row 0/1/2         | "[%+.6f  %+.6f  %+.6f]"                  | ~36              | OK     |
| (matrix)          | "Not computed yet"                       | 16               | OK     |
| For Pole/GoTo/Tracking | "ON"/"OFF"                            | 3                | OK     |
| Temperature       | "%.1f C"                                 | ~10              | OK     |
| Pressure          | "%.1f kPa"                               | ~11              | OK     |

### Site/Limits tab
| Label              | Value source / format                    | Worst-case chars | Status |
|--------------------|------------------------------------------|------------------|--------|
| Site Name          | siteName() [max 14 chars]                | 14               | OK     |
| Site Index         | "Index %d"                               | ~13              | OK     |
| Latitude/Longitude | fmtDeg                                   | ~17              | OK     |
| Elevation          | "%d m"                                   | ~8               | OK     |
| UTC Offset         | "%.1f h"                                 | ~10              | OK     |
| Hemisphere         | "North"/"South"                          | 6                | OK     |
| LST                | fmtHMS                                   | ~14              | OK     |
| UTC Date/Time      | "%04d-%02d-%02d %02d:%02d:%02d UTC"     | 25               | OK     |
| Sidereal Timer     | "%ld sidereal ticks"                     | ~26              | OK     |
| Missed Ticks       | "%ld"                                    | 10               | OK     |
| Min/Max Altitude   | "%d deg"                                 | ~8               | OK     |
| Meridian E/W GOTO  | "%ld min past"                           | ~20              | OK     |
| Under Pole Limit   | "%.2f deg"                               | ~16              | OK     |
| Pole Tracking Dist | "%d deg"                                 | ~8               | OK     |
| Sync Mode          | esNames[]                                | 7 ("Always")     | OK     |
| Encoder Enabled    | "YES"/"NO"                               | 3                | OK     |
| Encoder A1/A2      | "%.2f pulse/deg  Rev: %s"                | ~30              | OK     |
| Worst Loop Time    | "%ld us"                                 | ~13              | OK     |

---

## 4. Button Text vs Button Width

| Location      | Button text   | Chars | Text width | Button width | Status |
|---------------|---------------|-------|------------|--------------|--------|
| Overview      | Track OFF/ON  | 9     | 108 px     | 120 px       | OK (12 px margin) |
| Overview      | Unpark/Park   | 6/4   | 72/48 px   | 110 px       | OK     |
| Overview      | Abort         | 5     | 60 px      | 80 px        | OK (20 px margin) |
| Overview      | Stop Guide    | 10    | 120 px     | 140 px       | OK (20 px margin) |
| Park/Home     | Unpark/Park   | 6/4   | 72/48 px   | 100 px       | OK     |
| Tracking      | Track OFF/ON  | 9     | 108 px     | 120 px       | OK     |
| Guiding       | Stop Guide    | 10    | 120 px     | 140 px       | OK     |

---

## 5. Section Titles (drawSection)

All section titles start at PAD (12 px). Max width = 900 - 12 = 888 px → 74 chars.

| Section title                              | Chars | Width | Status |
|--------------------------------------------|-------|-------|--------|
| Mount Configuration                        | 20    | 240   | OK     |
| Quick Status                                | 12    | 144   | OK     |
| Current Coordinates                         | 20    | 240   | OK     |
| Park Status                                 | 12    | 144   | OK     |
| Home Status                                 | 12    | 144   | OK     |
| User-Defined Target                         | 19    | 228   | OK     |
| Sidereal Tracking                           | 17    | 204   | OK     |
| Tracking Rates                              | 14    | 168   | OK     |
| Effective Axis Rates                        | 20    | 240   | OK     |
| GoTo / Spiral                               | 13    | 156   | OK     |
| Guiding State                               | 13    | 156   | OK     |
| Guide Rates                                 | 11    | 132   | OK     |
| Guide Axis 1 (RA)                           | 16    | 192   | OK     |
| Guide Axis 2 (DEC)                          | 17    | 204   | OK     |
| Axis 1 (RA / Azimuth)                        | 21    | 252   | OK     |
| Axis 1 Geometry                             | 15    | 180   | OK     |
| Axis 2 (DEC / Altitude)                      | 22    | 264   | OK     |
| Axis 2 Geometry                             | 15    | 180   | OK     |
| Motor Configuration                         | 20    | 240   | OK     |
| Alignment Status                            | 16    | 192   | OK     |
| Coordinate Conversion (CoordConv)            | 31    | 372   | OK     |
| Transformation Matrix T                     | 23    | 276   | OK     |
| Inverse Matrix Tinv                         | 18    | 216   | OK     |
| Refraction Settings                         | 19    | 228   | OK     |
| Site Definition                             | 14    | 168   | OK     |
| Time                                        | 4     | 48    | OK     |
| Operational Limits                          | 19    | 228   | OK     |
| Encoders                                    | 8     | 96    | OK     |
| Performance                                 | 11    | 132   | OK     |

---

## 6. Other drawText Calls

| Location        | Text                                      | Chars | Start | End   | Status |
|----------------|-------------------------------------------|-------|-------|-------|--------|
| Overview title | "TeenAstro MainUnit v%s  (Emulator)"       | ~42*  | 12    | 516   | OK     |

*Assumes FirmwareNumber up to ~10 chars (e.g. "1.6.1-beta").

---

## Summary of Issues

### Overflow
- **None** – no element exceeds its budget.

### Tight (< 5 px margin)
- **Backlash Config** (Axes tab): value `"A1:%d (rate %d)  A2:%d (rate %d)"` can reach **51 chars** (e.g. `A1:-32768 (rate -32768)  A2:-32768 (rate -32768)`).  
  - 51 × 12 = 612 px; available = 618 px → **6 px margin**.  
  - **Recommendation:** shorten format (e.g. `A1:%d r%d  A2:%d r%d`) or truncate.

### Near limit (5–15 px margin)
- **Tab bar:** 812 px used of 900 px → 88 px margin. OK.
- **Track OFF/ON** button: 108 px text in 120 px → 12 px margin. OK.

---

## Audit Complete

All text elements checked. One tight case: **Backlash Config** value at 51 chars (6 px margin). All others within budget.
