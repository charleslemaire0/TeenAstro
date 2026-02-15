# TeenAstroLanguage

Header-only library that defines compile-time language constants for internationalization (i18n) in the TeenAstro project.

## Supported languages

| Constant | Value |
|----------|-------|
| `ENGLISH` | 0 |
| `FRENCH` | 1 |
| `GERMAN` | 2 |

## How it works

The `LANGUAGE` macro selects the active language at **compile time**. It defaults to `FRENCH` if not set by the build system.

### Setting the language

In PlatformIO:

```ini
build_flags = -DLANGUAGE=ENGLISH
```

### Using in code

Consumer libraries use `#if LANGUAGE == ...` to select language-specific strings:

```cpp
#include <TeenAstroLanguage.h>

#if LANGUAGE == ENGLISH
  #include "SHC_text_English.h"
#elif LANGUAGE == FRENCH
  #include "SHC_text_French.h"
#elif LANGUAGE == GERMAN
  #include "SHC_text_German.h"
#endif
```

Only one language's strings are compiled into the firmware — there is no runtime switching.

## Consumers

- **TeenAstroSHC** — `SHC_text.h` selects UI string tables
- **TeenAstroCatalog** — Messier, Caldwell, and star name localization

## Dependencies

None.
