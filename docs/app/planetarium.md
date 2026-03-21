# Planetarium engine

Real-time sky chart in the Flutter app: **stereographic zenithal projection**, stars, DSOs, planets, constellation lines/names, grids. Implemented with CustomPainter.

**Source:** `teenastro_app/lib/screens/planetarium_screen.dart`

---

## Projection (_Proj)

**Fields:** lat, lst, size, zoom, rotation, panY; _latRad, _cx, _cy, _radius = (diag/2)·zoom.

**project(raH, decDeg):**
1. HA = (LST − RA)·15·π/180
2. sin(Alt) = sin(Dec)·sin(φ) + cos(Dec)·cos(φ)·cos(HA); Alt = asin(sin(Alt))
3. cos(Az) = (sin(Dec)−sin(Alt)·sin(φ))/(cos(Alt)·cos(φ)); if sin(HA)>0 then Az = 2π−Az
4. r = radius·cos(Alt)/(1+sin(Alt))
5. x = cx − r·sin(Az+rotation), y = cy − r·cos(Az+rotation) + panY

**projectAltAz(alt, az)** — same stereographic formula from Alt/Az directly.

---

## Rendering layers (_SkyRenderer)

1. Sky background, 2. Ground (clip to sky circle), 3. Horizon, 4. Milky Way (J2000→JNow, clip), 5. Alt-Az grid (10°–80° alt, 30° az), 6. Equatorial grid (RA 2h, Dec ±75°), 7. Constellation lines, 8. Constellation names, 9. Stars (mag limit, B-V color, radial gradient, clamp 0.15–22 px), 10. DSO symbols, 11. Planets, 12. Crosshair (mount), 13. Target marker, 14. Cardinal labels (N/E/S/W).

---

## Hit testing

_handleTap: project each object (planets, stars, DSOs), distance to tap; pick closest within 30 px. _SkyObject: name, type, RA, Dec, mag, color, catalog, isJNow.

---

## Gestures

Scroll: zoom ×0.9/×1.1; scale panY. Pinch/pan: damped Jacobian for rotation and panY (denom = fDy²+30²). Tap: show UI or run _handleTap.

---

## Coordinates

Catalog (J2000) → JNow via equatorialEquinoxToJNow. Mount/planets (JNow) used as-is when _useJNow.

---

**See also:** [Astronomy algorithms](astro.md) · [App overview](overview.md)
