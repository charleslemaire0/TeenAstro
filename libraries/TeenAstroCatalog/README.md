# TeenAstroCatalog

Astronomical object catalog library for the TeenAstro telescope controller. Provides star and deep-sky object databases stored in PROGMEM, with filtering, navigation, and coordinate access for goto and alignment workflows.

## Catalogs

| Catalog | Objects | Type | Source |
|---------|---------|------|--------|
| Bright Stars | 408 | General stars | KStars |
| STF (Struve) | 595 | Double stars | WDS |
| STT (Struve) | 114 | Double stars | WDS |
| GCVS | 621 | Variable stars | GCVS 5.1 |
| Messier | 109 | DSO | Open NGC |
| Caldwell | 109 | DSO | Open NGC |
| Herschel 400 | 398 | DSO | Open NGC |
| NGC (select) | 2367 | DSO | Open NGC |
| IC (select) | 473 | DSO | Open NGC |

## Main class: `CatMgr`

A singleton catalog manager (`cat_mgr`) that provides:

- **Catalog selection** — `select(index)` switches the active catalog
- **Filtering** — combine bit flags: above-horizon, constellation, object type, magnitude, nearby, named, double-star separation, variable-star period
- **Navigation** — `setIndex()`, `incIndex()`, `decIndex()` iterate through records respecting active filters
- **Coordinates** — `ra()`, `dec()`, `ha()`, `alt()`, `azm()` in degrees; HMS/DMS accessors for display
- **Properties** — `magnitude()`, `constellation()`, `objectType()`, `objectName()`, `primaryId()`, `bayerFlam()`, plus double-star (separation, PA) and variable-star (period, secondary magnitude) fields
- **Refraction** — `topocentricToObservedPlace()` corrects for atmospheric refraction

## Usage

```cpp
#include <TeenAstroCatalog.h>

cat_mgr.setLat(latitude);
cat_mgr.setLstT0(lst);
cat_mgr.select(0);                     // Bright Stars
cat_mgr.filterAdd(FM_ABOVE_HORIZON);
cat_mgr.setIndex(0);

double ra  = cat_mgr.ra();   // degrees
double dec = cat_mgr.dec();  // degrees
```

## Dependencies

- **TeenAstroLanguage** — compile-time language selection for Messier/Caldwell names
