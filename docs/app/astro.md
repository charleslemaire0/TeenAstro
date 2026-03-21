# Astronomy algorithms (app)

Precession, nutation, aberration (J2000 ↔ JNow) and planet/Sun/Moon positions in the Flutter app.

**Source:** `teenastro_app/lib/models/equinox_precession.dart`, `planet_positions.dart`

---

## Equinox precession (equinox_precession.dart)

**Helpers:** _limitDeg360, _limitHours24, _sinD, _cosD, _tanD, _tFromJd = (jd−2451545)/36525.

**Mean obliquity:** ε₀ = 23°26′21.448″ − 46.8150·T − 0.00059·T² + 0.001813·T³.

**Nutation:** 17-term series for Δψ, Δε (arcsec); true obliquity ε = ε₀ + Δε.

**Precession (annual):** m = 3.07496+0.00186·Teq, nRA = 1.33621−0.00057·Teq, nDec = 20.0431−0.0085·Teq; ΔRA = m + nRA·sin(RA)·tan(Dec), ΔDec = nDec·cos(RA); apply yearDiff in arcsec.

**Nutation in RA/Dec:** ΔRA_nut = (cos(ε)+sin(ε)·sin(RA)·tan(Dec))·Δψ − cos(RA)·tan(Dec)·Δε; ΔDec_nut = sin(ε)·cos(RA)·Δψ + sin(RA)·Δε.

**Aberration:** K = 20.49552″; formulas for ΔRA_ab, ΔDec_ab using Sun longitude, eccentricity, etc.

**Functions:** equatorialEquinoxToJNow(raHours, decDeg, equinox, jd) → (raJNow, decJNow); equatorialJNowToEquinox (Newton iteration); j2000ToJNowLx200; formatRaDecLx200.

---

## Planet positions (planet_positions.dart)

**Julian date:** julianDate(DateTime utc) — Meeus formula.

**Ecliptic → equatorial:** sin(Dec)=sin(lat)·cos(ε)+cos(lat)·sin(ε)·sin(lon); RA = atan2(sin(lon)·cos(ε)−tan(lat)·sin(ε), cos(lon)).

**Sun:** Meeus Ch. 25 — L₀, M, C, sunLon, apparentLon, ε; sunPosition(jd) → CelestialBody.

**Moon:** Meeus Ch. 47 — lp, d, m, mp, f; sumL, sumB (20+ terms); moonPosition(jd) → CelestialBody.

**Planets:** Meeus Ch. 31–33. _OrbitalElements (l0, l1, a0, a1, e0, e1, i, Ω, ω, …). _solveKepler(mDeg, e): M = E − e·sin(E). Heliocentric: xp = a(cos(E)−e), yp = a√(1−e²)sin(E); rotate to ecliptic; geocentric (subtract Earth). planetPosition(planet, jd) → CelestialBody. _planetMag for magnitude.

**CelestialBody:** name, ra, dec, mag, color, radius.

---

**See also:** [Planetarium](planetarium.md) · [LA3 refraction](../math/la3.md)
