# Audits & safety

Summary and index of audit documents.

---

## Memory overflow audit

**Document:** [MEMORY_OVERFLOW_AUDIT.md](MEMORY_OVERFLOW_AUDIT.md)

- **SHC:** Buffer overflows in catalog titles, constellation list, catalogSubMenu — fixed (larger buffers, strncpy).
- **Webserver:** All form inputs bounded (strncpy + length checks); no unchecked strcpy.
- **TeenAstroCatalog:** catalogSubMenu overflow fixed with strncpy.
- **Recommendations:** Prefer strncpy/strncat/snprintf; centralize max lengths; keep SHC_text.h static_asserts; re-check list sizes if catalogs grow.

---

## Build & app audit

**Document:** [AUDIT_BUILD_AND_APP.md](AUDIT_BUILD_AND_APP.md)

- **Build portability:** All required files in repo; build_app.ps1 no hardcoded paths; Flutter from PATH or -FlutterPath.
- **App fixes:** TCP disconnect (close socket before cancelling subscription); RA/Dec formatting (seconds=60 carry); center-on-object (panY reset); star size clamp (0.15 px min); constellation_names.json fallback ([] not {}).
- **Planetarium:** Stereographic projection and 12 layers verified; precession, nutation, aberration, planet positions checked.

---

## Orphan commands (GXAS/GXCS)

**Document:** [orphan_commands_gxas_gxcs.md](orphan_commands_gxas_gxcs.md)

- **Removed (use :GXAS#):** GXJB, GXJC, GXJm, GXJM1/2, GXJP, GXJS, GXJT, GXRr, GXRd, GXRh, GXRe, GXRf.
- **Kept:** Standard LX200 (GR, GD, GA, GZ, GS, Gr, Gd); GXP1–GXP4 (axis positions); GXT2 (Unix time); GXOA/OB/OC (mount names); GXE* (encoders); GXA0–GXA8 (alignment matrix); GXD* (debug).
- Prefer :GXAS# and :GXCS# for polling.

---

## Emulator TCP & connection audit

**Date:** 2026-03-17

- **Black screen root cause:** `TcpClientStream` did not detect `WSAECONNRESET` (treated all `recv() < 0` as `EWOULDBLOCK`). Socket stayed zombie; all commands timed out; `notResponding()` triggered `ESP.reset()` → `exit(0)`.
- **Connection loss root cause:** Same `recv()` bug on `TcpServerStream` side could lose Windows app connection on port 9997.
- **Fix 1 (TcpStream.h):** Both `TcpServerStream` and `TcpClientStream` now check `WSAGetLastError() == WSAEWOULDBLOCK` (Windows) / `errno == EAGAIN` (POSIX) before ignoring `recv() < 0`. Non-WOULDBLOCK errors close the socket.
- **Fix 2 (TcpStream.h):** `TcpClientStream::write()` closes socket on non-WOULDBLOCK send errors. Added `reconnect()` method with saved host/port.
- **Fix 3 (esp_shim.h + shc_emu.cpp):** `ESP.reset()`/`restart()` no longer call `exit(0)`. Instead they invoke `emu_attempt_reconnect()` which retries TCP connect up to 20 times, resets `m_connectionFailure`, and only exits on total failure.
- **Fix 4 (SmartController.cpp):** Emergency stop `Ser.print(":Q#")` was dead code in emulator (`Ser` is unconnected stub). Now routes through `m_client->stopSlew()` under `#ifdef EMU_SHC`.
- **Interface check:** All 17 SHC and 39 MainUnit .cpp files correctly included. Shared libraries (CommandDef, LX200io, MountStatus, Math) at same paths. Protocol (LX200 framing, GXAS/GXCS binary) is shared code — no divergence.

---

## MainUnit cockpit UI overhaul

**Date:** 2026-03-17

- **Replaced** the basic `cockpit_sdl.h` (single flat list, 480×520) with a **tabbed UI** (720×700) in 7 tabs.
- **Tabs:** Overview | Park/Home | Tracking | Guiding | Axes | Alignment | Site/Limits.
- **Overview tab:** Mount config (type, meridian flip, peripherals), quick status (tracking, park, goto, guiding, error, pole side), current coordinates (target RA/DEC, alt/az, LST), control buttons.
- **Park/Home tab:** Park status/saved/position, backlash phase, settle info, home saved/at-home/mount flags, user-defined target coords, target pole side.
- **Tracking tab:** Sidereal enable/mode/clock/compensation, requested/stored HA/DEC rates, effective axis rates, goto/spiral state.
- **Guiding tab:** Guide state/last-state, all 5 guide rates, pulse/recenter/active rate, per-axis busy/direction/braking/abs-rate/duration.
- **Axes tab:** Per-axis position/target/delta/interval/acceleration/fstep/enable/fault/direction/backlash, geometry (steps/rot, steps/deg, limits, reference points), motor config (gear, micro, currents, backlash).
- **Alignment tab:** Valid flag, max stars, auto-by-sync, phase/star#/name, CoordConv ready/refs/error, T and Tinv matrices, refraction flags, temperature/pressure.
- **Site/Limits tab:** Site name/index/lat/long/elevation/UTC-offset/hemisphere, UTC time, LST, sidereal timer, missed ticks, alt/meridian/pole limits, encoder config, worst loop time.
- **Scrolling:** Mouse wheel scrolls content within each tab.
- **Build:** Compiles successfully under `emu_mainunit` environment.

---

**See also:** [Overview](overview.md) · [Build](build.md)
