# Bulk packets (GXAS / GXCS)

Binary state and config in one call. Base64-encoded little-endian packets; last byte = XOR checksum of preceding bytes.

---

## :GXAS# — State (102 bytes → 136 base64 chars + #)

| Bytes | Content |
|-------|---------|
| 0–2 | Status: tracking, sidereal, park, atHome, pierSide, guiding rate, mount type, guiding EW/NS, rate comp, pulse guiding |
| 3 | GNSS flags |
| 4 | Error code |
| 5 | Enable flags (encoders, motors, focuser, …) |
| 6–11 | UTC (h, m, s, month, day, year) |
| 12–83 | 9× float64 LE: RA, Dec, Alt, Az, LST, targetRA, targetDec, trackRateRa, trackRateDec |
| 84–91 | Stored rates (int32), focuser (pos, speed) |
| 98 | Timezone (int8 ×10) |
| 99 | Alignment reference count (0–2) |
| 100 | Alignment phase (bits 0–1), star number (bits 2–4), **GotoState** (bits 5–7: 0=none, 1=EQ, 2=Alt-Az, 3=meridian flip; see `CommandEnums.h`) |
| 101 | XOR checksum (bytes 0–100) |

---

## :GXCS# — Config (90 bytes → 120 base64 chars + #)

| Bytes | Content |
|-------|---------|
| 0–15 | Axis 1: gear, stepRot, backlash, backlashRate, lowCurr, highCurr, micro, flags (reverse, silent) |
| 16–31 | Axis 2 (same layout) |
| 32–55 | Rates: guide, slow, medium, fast (float32), acceleration, maxRate, defaultRate, settleTime |
| 56–73 | Limits: meridian E/W, axis1/2 min/max, underPole, minAlt, maxAlt, minDistPole, refraction flags |
| 74–83 | Encoders: PPD1, PPD2, sync mode, flags |
| 84 | Mount index |
| 89 | XOR checksum (bytes 0–88) |

---

## TeenAstroMountStatus (C++)

Parses GXAS → **MountState** (tracking, sidereal, parkState, atHome, positions, time, rates, focuser, alignment, **gotoKind** from byte 100 bits 5–7, error, enableFlags, …). Parses GXCS → config (axes, rates, limits, encoders). Enums: TrackState, SiderealMode, ParkState, PierState, GuidingRate, Mount, Errors. **StepperDriver:** StepDir, TOS100, TMC2130, TMC5160, TMC2660.

**Focuser:** :Fa# → 12 base64 chars (9 bytes).

**Source:** `libraries/TeenAstroMountStatus/`

---

**See also:** [LX200 protocol](protocol.md) · [Orphan commands](orphan_commands_gxas_gxcs.md)
