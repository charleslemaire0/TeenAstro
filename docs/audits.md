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

**See also:** [Overview](overview.md) · [Build](build.md)
