# TeenAstro Documentation

Use **this repo as the navigation basis**: open folders and files on GitHub to browse. All `.md` files render natively; no GitHub Pages required.

---

## Navigation

| Area | Content |
|------|--------|
| **Audits & notes** | [Memory overflow audit](MEMORY_OVERFLOW_AUDIT.md) · [Build & app audit](AUDIT_BUILD_AND_APP.md) · [Orphan commands (GXAS/GXCS)](orphan_commands_gxas_gxcs.md) |
| **Full technical reference** | [docs/html/](html/) — multi-page HTML site (architecture, math, firmware, app, protocol). Open ** [html/index.html](html/index.html)** in a browser after cloning (or run `python -m http.server 8080` inside `html/`). |

---

## Quick overview

- **MainUnit** (Teensy): mount control, goto, tracking, alignment.  
- **SHC** (ESP8266): hand controller with OLED and menus.  
- **Server** (ESP8266): WiFi bridge, TCP port 9999, LX200 protocol.  
- **App** (Flutter): Android/Windows — dashboard, planetarium, goto, alignment.  
- **Protocol:** LX200 (`:CMD#`). Bulk state/config: `:GXAS#` (102 bytes), `:GXCS#` (90 bytes).

Math: **TeenAstroLA3** (vectors, matrices, SVD, refraction), **TeenAstroCoord** (EQ/HO/IN/LO), **TeenAstroCoordConv** (Taki alignment + SVD).

---

*Repo = navigation: open `docs/` → this README and the links above.*
