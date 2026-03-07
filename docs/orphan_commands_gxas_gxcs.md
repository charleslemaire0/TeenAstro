# Orphan Get Commands — Redundant with GXAS / GXCS

Commands below return data that is **already present** in the bulk packets **:GXAS#** (state) or **:GXCS#** (config). They can be considered unnecessary if all clients use GXAS/GXCS for polling; removing them would simplify the firmware and reduce code paths.

**Note:** Standard LX200 commands (`:GR#`, `:GD#`, `:GA#`, `:GZ#`, `:GS#`, `:Gr#`, `:Gd#`) are **not** listed here; they should be kept for protocol compatibility with planetarium software, simulators, and scripts that expect classic LX200.

---

## 1. Redundant with :GXAS# (state packet, 102 bytes)

**Removed (use GXAS only):** `:GXJB#`, `:GXJC#`, `:GXJm#`, `:GXJM1#`, `:GXJM2#`, `:GXJP#`, `:GXJS#`, `:GXJT#`, `:GXRr#`, `:GXRd#`, `:GXRh#`, `:GXRe#`, `:GXRf#`.

| Command    | Description              | GXAS location / derivation                    |
|-----------|--------------------------|-----------------------------------------------|
| `:GXT0#`  | UTC time                 | Bytes 6–8 (hour, min, sec)                    |
| `:GXT1#`  | UTC date                 | Bytes 9–11 (month, day, year)                 |
| `:GXT3#`  | LHA (sidereal)           | Derivable: LST − RA (bytes 44–51, 12–19)      |

**Current use:**  
- **TeenAstroASCOM_V7** and **TeenAstroAscomNative** use GXAS for HasMotors and connection check.  
- **LX200Client** no longer exposes rate getters; **TeenAstroMountStatus** gets rates from GXAS.  
- **teenastro_app** uses GXAS for state.

---

## 2. Redundant with :GXCS# (config packet, 90 bytes)

| Command     | Description           | GXCS location                    |
|------------|-----------------------|----------------------------------|
| `:GXMBR#` `:GXMBD#` | Backlash amount    | Bytes 6–7, 22–23                 |
| `:GXMbR#` `:GXMbD#` | Backlash rate      | Bytes 8–9, 24–25                 |
| `:GXMGR#` `:GXMGD#` | Gear              | Bytes 0–3, 16–19                 |
| `:GXMSR#` `:GXMSD#` | Steps per rotation| Bytes 4–5, 20–21                 |
| `:GXMMR#` `:GXMMD#` | Microstep         | Byte 14, 30                      |
| `:GXMmR#` `:GXMmD#` | Silent mode       | Byte 15, 31 (flags)              |
| `:GXMRR#` `:GXMRD#` | Reverse           | Byte 15, 31 (flags)              |
| `:GXMCR#` `:GXMCD#` | High current      | Bytes 12–13, 28–29               |
| `:GXMcR#` `:GXMcD#` | Low current       | Bytes 10–11, 26–27               |
| `:GXR0#` … `:GXR3#` | User rates 0–3    | Bytes 32–47 (float32)            |
| `:GXRA#`   | Acceleration (deg)     | Bytes 48–51 (float32)            |
| `:GXRD#`   | Default rate index    | Byte 54                          |
| `:GXRX#`   | Max slew rate         | Bytes 52–53 (uint16)             |
| `:GXLA#` … `:GXLD#` | User axis limits  | Bytes 60–67 (int16)              |
| `:GXLE#` `:GXLW#`   | Meridian E/W      | Bytes 56–59 (int16)              |
| `:GXLU#`   | Under-pole limit      | Bytes 68–69 (uint16)             |
| `:GXLO#` `:GXLH#`   | Overhead/horizon     | Bytes 70–71 (int8)               |
| `:GXLS#`   | Distance from pole    | Byte 72                          |
| `:GXrp#` `:GXrg#` `:GXrt#` | Refraction flags | Byte 73 (bits 0–2)   |
| `:GXOI#`   | Mount index           | Byte 84                          |
| `:GXOS#`   | Slew settle (s)       | Byte 55                          |

**Current use:**  
- **TAConfig** and other config tools often use the individual `:GXMx#` / `:GXRx#` / `:GXLx#` getters when reading or validating one field.  
- **MountStatus** (C++) and **teenastro_app** use **:GXCS#** for full config; individual getters are still used where only one value is needed (e.g. config UI).

---

## 3. Commands to keep (not fully redundant or required for compatibility)

- **`:GR#`, `:GD#`, `:GA#`, `:GZ#`, `:GS#`, `:Gr#`, `:Gd#`** — Standard LX200; required for compatibility.
- **`:GXP1#`, `:GXP2#`, `:GXP3#`, `:GXP4#`** — Axis position in degrees (raw axis or EQ pipeline); not the same as RA/Dec/Alt/Az in GXAS.
- **`:GXT2#`** — Unix timestamp; not in GXAS.
- **`:GXOA#`, `:GXOB#`, `:GXOC#`** — Mount names (strings); not in GXCS packet.
- **`:GXE1#`, `:GXE2#`, `:GXEA#`, `:GXEZ#`, `:GXED#`, `:GXER#`, etc.** — Encoder positions/options; not in GXAS.
- **`:GXA0#` … `:GXA8#`** — Alignment matrix; not in GXAS/GXCS.
- **`:GXlA#` … `:GXlD#`** — Mount-type limits (computed); may overlap GXCS axis limits but are a different concept.
- **`:GXD*#`** — Debug only; keep for development.

---

## 4. Recommendation

- **State:** Any client that already polls **:GXAS#** does not need the **:GXJx#**, **:GXRr#**, **:GXRd#**, **:GXRe#**, **:GXRf#**, **:GXT0#**, **:GXT1#**, **:GXT3#** getters for that purpose. They remain useful for:
  - Minimal “single property” queries (e.g. “is motor on?” without parsing 102 bytes).
  - Legacy or VB ASCOM driver that has not been switched to GXAS.
- **Config:** Similarly, **:GXCS#** makes the listed **:GXMx#**, **:GXRx#**, **:GXLx#**, **:GXr*#**, **:GXOI#**, **:GXOS#** getters redundant for “full config” use cases; individual getters are still convenient for config UIs that read/write one field at a time.

To **remove** a command as orphan:

1. Ensure every current caller (ASCOM V7, LX200Client, app, TAConfig, tests) uses GXAS/GXCS (or another non-orphan command) for that data.
2. Remove the command handler in **TeenAstroMainUnit/Command_GX.cpp** (and **UniversalMainUnit/Command_G.cpp** if present).
3. Update **CommandReplyLength**, **Commands.md**, and any client that still referenced the command.

This document is a reference for planning; no commands have been removed automatically.
