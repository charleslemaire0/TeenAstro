# LX200 protocol

All communication with the MainUnit uses the **LX200 serial protocol** (Meade Autostar/LX200 compatible). Format: **:CMD#** (leading colon, command string, hash). Case-sensitive. Over serial (USB) or TCP (WiFi Server port 9999).

**Full reference:** `TeenAstroMainUnit/Commands.md`

---

## Command groups

| Group | Prefix | Purpose |
|-------|--------|---------|
| Reset | $ | Reboot, EEPROM erase, reinit encoders/motors |
| ACK | \<ACK\> | Alignment type (A/P/G/L) |
| Alignment | A | Polar home, sync, add reference, save |
| Reticule | B | Brightness +/− |
| Sync | C | CM, CS, CA, CU |
| Distance | D | Distance-to-target bars |
| Encoder | E | Encoder align, sync, push-to |
| Focuser | F | Focus in/out, query, binary packets |
| GNSS | g | gs (full sync), gt (time only) |
| Get | G | Position, time, site, status (GR, GD, GA, GZ, GS, …) |
| Get extensions | GX | GXAS, GXCS, alignment, encoders, debug |
| Move | M | MS (slew), MA (Alt/Az), pulse guide (Mge, Mgw, Mgn, Mgs) |
| Quit | Q | Halt all, halt E/W, halt N/S |
| Set | S | Time, site, limits, target (Sr, Sd, St, Sh, So, …) |
| Set extensions | SX | Alignment, encoders, rates, limits, motors |
| Tracking | T | Te/Td (enable/disable), TQ/TL/TS (sidereal/lunar/solar), T+/T− |
| User rates | U | Precision toggle |

---

## Key commands (examples)

| Command | Description | Returns |
|---------|-------------|---------|
| :$$# | Factory reset | 1 |
| :$!# | Reboot | 1 |
| :A0# | Alignment from polar home | 1 |
| :A*# | Sync + add reference | 1 |
| :CM# / :CS# | Sync to target (EQ) / + start tracking | N/A / (nothing) |
| :GR# / :GD# | Get RA / Dec | HH:MM.T# / sDD*MM# |
| :GXAS# | Bulk state (102 bytes, base64) | 136 chars + # |
| :GXCS# | Bulk config (90 bytes, base64) | 120 chars + # |
| :MS# | Slew to target | 0–6 (ERRGOTO) |
| :Q# | Halt all | (nothing) |
| :Sr HH:MM.T# / :Sd sDD*MM# | Set target RA / Dec | 1/0 |
| :Te# / :Td# | Enable / disable tracking | 1/0 |

---

## LX200Client (C++)

**IX200Transport** — abstract: flush, write, available, read, setTimeout. **StreamTransport** — over Arduino Stream.

**LX200Client:** sendReceive, sendReceiveAuto (uses CommandReplyLength); get/set; getters for RA, Dec, Alt, Az, time, version; syncGoto, moveToTarget, tracking, movement, home/park, alignment, focuser, motor config, encoder.

**LX200Navigation** — free functions: SyncGotoLX200, SyncGotoCatLX200, SyncGotoPlanetLX200, SyncSelectedStarLX200 (with equinox precession).

**Source:** `libraries/TeenAstroLX200io/`

---

**See also:** [Bulk packets (GXAS/GXCS)](protocol-bulk.md) · [MainUnit](firmware/mainunit.md)
