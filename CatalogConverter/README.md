# TeenAstro Catalog Converter

Tooling and source data for converting astronomical deep-sky and star catalogs (e.g. from OnStep / OpenNGC formats) into optimized, flash-resident (PROGMEM) C/C++ data tables for TeenAstro and its Smart Hand Controller (SHC).

---

## Structure

- **`CatConv/`**: VB.NET command-line utility for parsing catalog header files and generating indexed TeenAstro catalog arrays (`ta_*.h`).
- **`ONSTEPcat/`**: Upstream catalog tables and definitions (Messier, Caldwell, NGC, IC, Herschel 400, Struve STF/STT, GCVS variable stars, and Bright Stars).

---

## Generated Catalogs

The output header files are consumed by the [`TeenAstroCatalog`](../libraries/TeenAstroCatalog/README.md) library located in `libraries/TeenAstroCatalog/`:

| Catalog File | Content | Records |
|--------------|---------|---------|
| `messier.h` / `messier_c.h` | Messier Deep Sky Objects | 109 |
| `caldwell.h` / `caldwell_c.h` | Caldwell Catalog | 109 |
| `herschel.h` / `herschel_c.h` | Herschel 400 Survey | 398 |
| `ngc_select_c.h` | Selected New General Catalogue objects | 2,367 |
| `ic_select_c.h` | Selected Index Catalogue objects | 473 |
| `stars.h` / `stars_vc.h` | Bright Stars Catalog | 408 |
| `stf.h` / `stf_select_c.h` | WDS Struve (STF) Double Stars | 595 |
| `stt.h` / `stt_select_c.h` | WDS Otto Struve (STT) Double Stars | 114 |
| `gcvs.h` / `gcvs_select_c.h` | General Catalogue of Variable Stars | 621 |

---

## Building CatConv

The tool is a .NET / VB.NET console application:

```bash
# Build with MSBuild / Visual Studio
msbuild CatConv/CatConv.sln /p:Configuration=Release
```

See [libraries/TeenAstroCatalog/README.md](../libraries/TeenAstroCatalog/README.md) for details on how catalogs are queried and indexed at runtime.
