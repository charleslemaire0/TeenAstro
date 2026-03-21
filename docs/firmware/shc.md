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

---

## Actions files

- **Actions_SyncGoto:** menuSyncGoto, menuCatalogs, menuCoordinates, menuPier, menuSpiral, menuCatalog, menuCatalogAlign, menuSolarSys, menuRADecNow, menuAltAz.
- **Actions_Tel:** menuTelActionGoto, menuTelActionPushTo, menuSpeedRate, menuTrack, menuReticule.
- **Actions_Focuser:** focuser actions.

**Display:** SmartController_Display.cpp — updateMainDisplay, drawIntro, icons for tracking/park/guiding/pier/errors.

---

**See also:** [MainUnit](mainunit.md) · [LX200 protocol](../protocol.md)
