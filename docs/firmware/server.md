# WiFi Server

ESP8266 firmware: TCP bridge on port **9999**, exposing the MainUnit LX200 serial protocol over WiFi. Includes a small web UI for configuration.

**Source:** `TeenAstroServer/`, `libraries/TeenAstroWifi/`

---

## Role

- Listen on TCP 9999; forward bytes to/from MainUnit serial (57600).
- Web server for WiFi credentials (STA/AP), web password, site config.

---

## Safety

All web form inputs (SSID, password, AP settings, web password) use **bounded** copies: `strncpy(..., sizeof(...)-1)` and null termination. No unchecked `strcpy` of `server.arg()` into fixed buffers. readBuffer/writeBuffer sizes and usage are bounded.

---

**See also:** [Memory overflow audit](../MEMORY_OVERFLOW_AUDIT.md) · [MainUnit](mainunit.md)
