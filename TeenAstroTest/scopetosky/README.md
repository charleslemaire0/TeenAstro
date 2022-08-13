# Scope to Sky and TeenAstro



Note: The ScopeToSky test is no longer maintained. It is replaced by mountSim, which uses the skyfield Python library. 

Mel Bartel’s [Scope to Sky calculator](https://www.bbastrodesigns.com/scopeToSky.html), written in Javascript with an HTML user interface, has several functions:

- Given sky coordinates (either Right Ascension or Hour Angle, and Declination), compute a telescope’s axis positions.
- Given a telescope’s axis positions, compute the corresponding sky coordinates.
- For a telescope with encoders, display encoder values for a given telescope position or compute telescope position according to the encoder values.

The configuration includes:

- Setup site, time and date, time zone
- equatorial and altazimuth mounts
- Correction for precession, nutation and annual aberration
- Correction for refraction
- For equatorial mounts, set pier side (mount flip)
- Conversion styles (trigonometry or matrix)
- Tracking rates algorithms (method for computing the rates)
- Encoder gears

## How to use it for testing TeenAstro Firmware

We generate a set of test cases (sky positions, site, time etc.), use ScopeToSky to compute axes positions for each, and use them as Goto targets for a TeenAstro. We read the stepper counts for both axes, normalize them back to degrees, and compare with the computed values.

See complete documentation at http://astro.roya.org/teenastro_linux/scopetosky/