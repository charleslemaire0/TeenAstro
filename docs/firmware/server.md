# WiFi Server

ESP8266 firmware: TCP bridge on port **9999**, exposing the MainUnit LX200 serial protocol over WiFi. Includes a small web UI for configuration.

**Source:** `TeenAstroServer/`, `libraries/TeenAstroWifi/`

---

## Role

- Listen on TCP 9999; forward bytes to/from MainUnit serial (57600).
- Web server for WiFi credentials (STA/AP), web password, site and mount config. On the mount page, **Mount error** is above refraction: an Off/On choice (“Hold CH and NP”), then one row for CH and one for NP. Each row is the value, Upload, then the name and the unit (degrees, ±5). The signs are Wallace's. On an equatorial mount the Reticule page draws one circle at the true polar distance of Polaris, or Sigma Octantis in the south, with 12 at the top. The bottom-right shows local civil time and LMT (UT shifted by the site longitude). A finder whose longitude correction stays at 0 is set to LMT. Brighter and Less Bright sit with those clocks and drive the illuminator.

---

## Safety

All web form inputs (SSID, password, AP settings, web password) use **bounded** copies: `strncpy(..., sizeof(...)-1)` and null termination. No unchecked `strcpy` of `server.arg()` into fixed buffers. readBuffer/writeBuffer sizes and usage are bounded.

---

**See also:** [Memory overflow audit](../MEMORY_OVERFLOW_AUDIT.md) · [MainUnit](mainunit.md)
