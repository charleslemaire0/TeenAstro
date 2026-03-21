# SHC and Webserver – Memory Overflow Audit

Audit date: 2025-03. Focus: fixed-size buffers, `strcpy`/`strcat`/`sprintf` usage, and web input handling.

---

## Executive summary

- **SHC**: Several buffer overflows found (catalog titles, constellation list, `catalogSubMenu`). Fixes applied where possible; remaining items need size increases or `strncpy`/`strncat`/`snprintf`.
- **Webserver (TeenAstroWifi)**: HTTP form input is copied with `strncpy`/length checks (SSID, password, AP settings). Web password and SSID/password handling are bounded. No unchecked `strcpy` of `server.arg()` into fixed buffers found.
- **TeenAstroCatalog**: `catalogSubMenu()` had a definite overflow (`Title[32]` copied into `thisSubMenu[16]`); fixed with `strncpy`.

---

## 1. SHC – Critical overflows (fixed or documented)

### 1.1 TeenAstroCatalog – `catalogSubMenu()` (FIXED)

- **File**: `libraries/TeenAstroCatalog/src/TeenAstroCatalog.cpp`
- **Issue**: `static char thisSubMenu[16]` and `strcpy(thisSubMenu, catalog[_selected].Title)`. `catalog_t.Title` is `char[32]`. Copying up to 32 bytes into 16 bytes causes buffer overflow.
- **Fix**: Use `strncpy(thisSubMenu, catalog[_selected].Title, sizeof(thisSubMenu)-1)` and null-terminate.

### 1.2 Actions_Tel.cpp – `title[20]` + `strcat(title, catalogTitle())` (FIXED)

- **Issue**: `char title[20]` then `strcat(title, cat_mgr.catalogTitle())`. `catalogTitle()` can return up to 31 chars. Overflow.
- **Fix**: Increase buffer (e.g. `title[40]`) or use `strncat` with remaining size.

### 1.3 Actions_SyncGoto.cpp – multiple overflows (FIXED)

- **title[16] / lastSubmenu[16] / thisSubmenu[16]**: `strcpy(title, cat_mgr.catalogTitle())` – catalog title can be 31 chars. Same for submenu usage.
- **Fix**: Use buffers of at least 32 (e.g. `title[32]`, `lastSubmenu[32]`, `thisSubmenu[32]`).
- **string_list_gotoL1[60 + NUM_CAT*10] = 150**: List is built with `strcat(string_list_gotoL1, title)` and `"\n"` for up to `NUM_CAT` (9) entries. Each `title` can be ~31 chars → 9*(31+1) = 288 bytes. 150 is too small.
- **Fix**: Increase to at least 400 (e.g. `60 + NUM_CAT * 40` or a constant 400).
- **menuCatalog() / menuCatalogAlign()**: `title[20]` and `strcat(title, catalogTitle())` (and optional "!" or "PushTo "/"Goto "). Same as 1.2.
- **Fix**: Use `title[40]` (or equivalent) and ensure all concatenations respect buffer size.

### 1.4 SmartController_CatFilter.cpp – `string_list_fCon[1000]` (FIXED)

- **Issue**: `menuFilterCon()` builds a list of 89 constellation names + newlines. Longest names ~20 chars → 89*(20+1) ≈ 1869 bytes. Buffer is 1000.
- **Fix**: Increase to 2000 (or 89*22 + margin).

---

## 2. SHC – Safe or already bounded

- **SmartController_Message.cpp**: `text1[32]`, `text2[32]` – already enlarged; content is fixed `T_*` strings (validated by `SHC_text.h` static_asserts).
- **SmartController.cpp**: `line[32]` = getVB() (CachedStr<10>) + " - " + drivername (stepperDriverName max 7). Fits. `getDriverName(drivername)` – `drivername[10]`, stepper names ≤ 7. Safe.
- **SmartController.cpp**: `date_time[40]`, `date_time2[40]` – `T_TIME`/`T_DATE` + mount time/date strings (CachedStr<15>). Fits.
- **SmartController_Display.cpp**: `starNum[16]` – `sprintf(starNum, T_STAR " #%d", ...)`. T_STAR short; number is one digit in practice. Safe.
- **Settings_Mount.cpp**, **Settings_Motor.cpp**, **Settings_Focuser.cpp**, **Settings_Limits.cpp**: `sprintf` into `line[32]`, `menu[32]`, `text[20]` with fixed format strings and numeric values – sizes are tight but within current T_* limits (see SHC_text.h).
- **Settings_Wifi.cpp**: `txt[150]` then `strcat(txt, out)` (up to 3× getStationName), then `menustxt[200] = txt + T_ACCESPOINT`. 3*40+3+13 < 200. Safe.
- **Settings_TimeSite.cpp**: `txt[70]` = 3× sitename (15) + 2 newlines = 47. Safe. `getSiteName(..., sitename, sizeof(sitename))` bounds the copy.
- **TeenAstroWifi writeBuffer[50]**: TCP command buffer; `writeBufferPos > 49` is checked and buffer is reset. Safe.
- **TeenAstroWifi readBuffer[130]**: Used for LX200 reply; `sendReceiveAuto(..., readBuffer, sizeof(readBuffer), ...)`. Bounded.

---

## 3. Webserver (TeenAstroWifi) – Input handling

- **Configuration_wifi.cpp**: `server.arg("webpwd")` → `strncpy(masterPassword, v.c_str(), sizeof(masterPassword)-1)` and null termination. Safe.
- **SSID/password**: `strncpy(wifi_sta_ssid[k], v.c_str(), sizeof(wifi_sta_ssid[k])-1)`, same for `wifi_sta_pwd`, `wifi_ap_ssid`, `wifi_ap_pwd`. Safe.
- **Index.cpp / Helper.cpp**: `sprintf_P(temp, ...)` with `temp[128]` or `temp1[64]` and format strings that embed mount status (short strings). Safe for current formats.
- **Configuration_site.cpp**: `getSiteName(..., siteName, sizeof(siteName))` with `siteName[50]`. Safe.
- **Other server.arg() uses**: Values are used as integers/floats via `toInt()`/`atof2()`/`atoi2()` or compared; no unchecked copy into small fixed buffers.

---

## 4. Recommendations

1. **Prefer bounded functions**: Use `strncpy`/`strncat`/`snprintf` with explicit size for any buffer that might receive variable-length or external data.
2. **Centralize max lengths**: For catalog titles/submenus, consider `#define` (e.g. `CATALOG_TITLE_MAX 32`) and use it for both `catalog_t.Title` and all local buffers that hold catalog title/submenu.
3. **Keep SHC_text.h checks**: The existing `static_assert` checks in `SHC_text.h` prevent T_* strings from exceeding display and buffer sizes; keep them when adding or changing translations.
4. **Constellation/object lists**: If more catalogs or longer names are added, re-check `string_list_fCon` and `string_list_fType` sizes.

---

## 5. Files changed in this audit (fixes)

- `libraries/TeenAstroCatalog/src/TeenAstroCatalog.cpp` – `catalogSubMenu()`: strncpy + null terminate.
- `TeenAstroSHC/Actions_Tel.cpp` – `title[20]` → `title[40]`.
- `TeenAstroSHC/Actions_SyncGoto.cpp` – title/lastSubmenu/thisSubmenu 16→32; string_list_gotoL1 150→400; menuCatalog/menuCatalogAlign title 20→40.
- `TeenAstroSHC/SmartController_CatFilter.cpp` – `string_list_fCon[1000]` → `string_list_fCon[2000]`.
