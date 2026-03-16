# Flutter app overview

Android and Windows app: dashboard, planetarium, catalog browser, goto, multi-star alignment. Connects to MainUnit via WiFi Server (TCP, LX200).

**Source:** `teenastro_app/lib/`

---

## Structure

- **main.dart**, **app.dart**, **theme.dart** — entry, root widget, theme.
- **screens/** — Connection, Dashboard, Control, Goto, Catalog Browser, Planetarium, Alignment, Tracking, Settings, Sync Panel.
- **models/** — MountState, MountConfig, CatalogEntry, equinox_precession, planet_positions, constellation_lines, milky_way_data, lx200_commands, lx200_reply_lengths.
- **providers/** — Riverpod: mountStateProvider, planetariumSettingsProvider, nightViewProvider, catalog filter.
- **services/** — lx200_tcp_client, catalog_filter_provider.
- **widgets/** — shared UI.
- **assets/** — catalogs (JSON), star field, constellation lines, milky way.

---

## State

**MountStateNotifier** polls `:GXAS#`; parses 102-byte binary packet into MountState (tracking, park, positions, time, rates, focuser, alignment, errors, etc.). Reconnects with backoff. **mount_state_provider** exposes state to screens.

---

## TCP client (LX200TcpClient)

**connect(ip, port)**, **disconnect()** — close socket **before** cancelling stream subscription so the connection always closes. **sendCommand(cmd, timeout)** — mutex, send, read reply (length from getReplyType). Heartbeat ~1.5 s when idle (`:GXAS#`).

---

## Screens (summary)

| Screen | Purpose |
|--------|---------|
| Connection | IP/port, connect to mount |
| Dashboard | Live RA/Dec, Alt/Az, tracking, park, timers |
| Control | N/S/E/W slew, speed, stop |
| Goto | Catalog object, goto |
| Catalog Browser | Filter/search Messier, NGC, stars |
| Planetarium | Sky chart, stars, DSOs, planets, constellation lines |
| Alignment | Multi-star alignment wizard |
| Tracking | Sidereal/lunar/solar, rate control |
| Settings | Mount config, limits, site |
| Sync Panel | Manual sync |

---

**See also:** [Planetarium](planetarium.md) · [Astronomy](astro.md) · [Bulk packets](../protocol-bulk.md)
