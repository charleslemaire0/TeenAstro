# TeenAstro Documentation

Full documentation in **Markdown**. Use the repo as the navigation basis: open any file on GitHub and it renders natively. No GitHub Pages required.

---

## Table of contents

### Overview

| Document | Description |
|----------|-------------|
| [Overview & architecture](overview.md) | System overview, components, data flow, coordinate pipeline |

### Math & coordinates

| Document | Description |
|----------|-------------|
| [math/README.md](math/README.md) | Math libraries index |
| [Linear algebra (LA3)](math/la3.md) | Vectors, matrices, rotations, Euler angles, refraction, SVD |
| [Coordinate systems](math/coord.md) | EQ, HO, IN, LO and conversion formulas |
| [Alignment (CoordConv)](math/alignment.md) | Taki method, SVD correction, firmware integration |

### Firmware

| Document | Description |
|----------|-------------|
| [firmware/README.md](firmware/README.md) | Firmware index |
| [MainUnit](firmware/mainunit.md) | Application, Axis classes, sidereal clock, tracking, goto, EEPROM |
| [SHC](firmware/shc.md) | Smart Hand Controller, menus, display |
| [WiFi Server](firmware/server.md) | TCP bridge, web config |
| [Motor & stepper](firmware/stepper.md) | StepDir, TMC, motion control |

### Application

| Document | Description |
|----------|-------------|
| [app/README.md](app/README.md) | App index |
| [Flutter app](app/overview.md) | Structure, state, screens, TCP client |
| [Planetarium](app/planetarium.md) | Stereographic projection, rendering layers |
| [Astronomy algorithms](app/astro.md) | Precession, nutation, aberration, planet positions |

### Protocol

| Document | Description |
|----------|-------------|
| [LX200 protocol](protocol.md) | Command format, groups, LX200Client |
| [Bulk packets (GXAS/GXCS)](protocol-bulk.md) | Binary packet layouts, MountStatus |

### Build & operations

| Document | Description |
|----------|-------------|
| [Build system](build.md) | Prerequisites, scripts, versioning, tests |
| [Audits](audits.md) | Memory safety, build/app audit, orphan commands |

### Audits & notes (standalone)

| Document | Description |
|----------|-------------|
| [Memory overflow audit](MEMORY_OVERFLOW_AUDIT.md) | SHC/webserver buffer safety |
| [Build & app audit](AUDIT_BUILD_AND_APP.md) | Portability, planetarium, fixes |
| [Orphan commands](orphan_commands_gxas_gxcs.md) | GXAS/GXCS redundant getters |

---

*Browse the repo: open `docs/` → this README → follow links to any section.*
