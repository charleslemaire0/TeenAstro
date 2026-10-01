# SHC — Smart Hand Controller

ESP8266-based hand controller with OLED display and button pad. Talks to MainUnit over serial; uses LX200Client for commands.

**Source:** `TeenAstroSHC/`

---

## SmartHandController class

**Key methods:** `setup(version, pin[], active[], SerialBaud, model, nSubmodel)`, `update()` (main loop: buttons, display, actions), `setClient(LX200Client&)`, `updateMainDisplay(PAGES page)`, `updateAlign()`, `updatePushing()`, `manualMove()`, `tickButtons()`, `DisplayMessage()`, `DisplayMessageLX200()`.

---

## Display pages (PAGES)

`P_RADEC`, `P_HADEC`, `P_ALTAZ`, `P_PUSH`, `P_TIME`, `P_AXIS_STEP`, `P_AXIS_DEG`, `P_FOCUSER`, `P_ALIGN`.

---

## Menu system

U8g2_ext: selection lists, input value (integer, DMS, float), messages, catalog UI. Long-press center button → menu; buttons 1–6 for selection. Top-level: Tel settings, Display actions, Focuser, Speed/rate; sub-menus: Sync/Goto, Catalogs, Mount, Motors, Limits, Encoders, WiFi, Time/Site, etc.

**Mount → Mount error** sits directly above Refraction. The same item is the last line of **Align**. Off leaves a 2-star alignment on the classic Taki path. On, with a non-zero cone or perpendicularity (±5°), a 2-star alignment holds both values and still measures the pole. **4 Stars** and **3+3 Stars** estimate those terms from the stars and do not read the stored values. The field manual, appendix A, is the menu tree in each firmware language.

---

## Actions files

- **Actions_SyncGoto:** menuSyncGoto, menuCatalogs, menuCoordinates, menuPier, menuSpiral, menuCatalog, menuCatalogAlign, menuSolarSys, menuRADecNow, menuAltAz.
- **Actions_Tel:** menuTelActionGoto, menuTelActionPushTo, menuSpeedRate, menuTrack, menuReticule.
- **Actions_Focuser:** focuser actions.

**Display:** SmartController_Display.cpp — updateMainDisplay, drawIntro, icons for tracking/park/guiding/pier/errors.

---

**See also:** [MainUnit](mainunit.md) · [LX200 protocol](../protocol.md)
