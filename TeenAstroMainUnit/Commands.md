# TeenAstro serial command reference

Commands use the format `:CMD#`: leading colon, command string, terminating hash. Case-sensitive. **LX200 standard** = command (or equivalent) in the Meade Autostar/LX200 command set; **TeenAstro extension** = not in that set.

---

## $ — Reset / reboot / reinit

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:$$#` | Erase entire EEPROM (factory reset). | `1` | TeenAstro extension |
| `:$!#` | Reboot the main unit. | `1` | TeenAstro extension |
| `:$X#` | Reinitialize encoders and motors (no EEPROM wipe). | `1` | TeenAstro extension |

---

## &lt;ACK&gt; (ASCII 6) — Alignment query

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| &lt;ACK&gt; (no `:#`) | Report mount alignment type. | `A` AltAz, `P` Polar (Fork), `G` GEM, `L` Land/undefined | LX200 standard |

---

## A — Alignment

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:A0#` | Set alignment from polar home; enable axes and start tracking. | `1` | LX200 standard |
| `:A*#` | Telescope at target: sync to current Alt/Az, add reference to alignment model. | `1` | LX200 standard |
| `:A2#` | Add reference from current target; if model ready, compute and apply alignment. | `1` | LX200 standard |
| `:AE#` | Get current alignment error (degrees). | `sDD*MM'SS#` | LX200 standard |
| `:AC#` | Sync at home; disable auto alignment-by-sync. | `1` | LX200 standard |
| `:AA#` | Sync at home; enable auto alignment-by-sync. | `1` | LX200 standard |
| `:AW#` | Save alignment model to EEPROM. | `1` | LX200 standard |

---

## B — Reticule

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:B+#` | Increase reticle brightness. | (nothing) | LX200 standard |
| `:B-#` | Decrease reticle brightness. | (nothing) | LX200 standard |

---

## C — Sync

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:CM#` | Sync mount to current object coordinates (EQ); optional multi-star alignment. | `N/A#` or nothing | LX200 standard |
| `:CS#` | Sync mount to current object coordinates (EQ); optional multi-star alignment; start tracking. | (nothing) | LX200 standard |
| `:CA#` | Sync mount to current target Alt/Az. | `N/A#` | LX200 standard |
| `:CU#` | Sync to user-defined RA/Dec (from EEPROM). | `N/A#` | TeenAstro extension |

---

## D — Distance bars

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:D#` | Get distance-to-target bars (moving indicator). | `0x7f#` if slewing, else `#` | LX200 standard |

---

## E — Encoder / push-to

All `:E…#` commands are encoder and push-to related. Not in Meade LX200.

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:EAS#` | Align encoder start: sync encoders to telescope, set reference. | `1#` | TeenAstro extension |
| `:EAE#` | Align encoder end: calibrate encoders from current position. | `1#` / `0#` | TeenAstro extension |
| `:EAQ#` | Align encoder quit: clear encoder reference. | `1#` | TeenAstro extension |
| `:ECT#` | Sync telescope to encoder positions. | `1#` | TeenAstro extension |
| `:ECE#` | Sync encoders to telescope position. | `1#` | TeenAstro extension |
| `:ECS#` | Sync telescope and encoders to current target (RA/Dec or Alt/Az). | `1#` | TeenAstro extension |
| `:ED#` | Get push-to delta (status and arcmin offsets). | `e,arcmin1,arcmin2#` | TeenAstro extension |
| `:EMS#` | Set push-to mode to RA/Dec using current target. | `e#` | TeenAstro extension |
| `:EMA#` | Set push-to mode to Alt/Az using current target. | `e#` | TeenAstro extension |
| `:EMU#` | Set push-to mode to RA/Dec using user coordinates from EEPROM. | `e#` | TeenAstro extension |
| `:EMQ#` | Turn off push-to mode. | `1` | TeenAstro extension |

---

## F — Focuser

Focuser commands are passed through to the focuser serial line. Standard LX200 focus commands.

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:F+#` | Focus in. | (nothing) | LX200 standard |
| `:F-#` | Focus out. | (nothing) | LX200 standard |
| `:FQ#` | Focus quit / stop. | (nothing) | LX200 standard |
| `:FF#` / `:FS#` | Focus fast / slow. | (nothing) | LX200 standard |
| `:F?#` | Query focuser (no focuser = `0`). | (varies) | LX200 standard |
| `:Fg#` `:FG#` `:FP#` `:Fs#` `:FS#` `:F!#` `:F$#` `:Fx#` `:F~#` `:FM#` `:FV#` | Forwarded to focuser (no reply). | (nothing) | LX200 standard |
| `:FO#` `:Fo#` `:FI#` `:Fi#` `:FW#` `:F0#` … `:F8#` `:Fc#` `:FC#` `:Fm#` `:Fr#` | Forwarded to focuser (short reply). | Depends on focuser | LX200 standard |

---

## g — GNSS

All `:g…#` commands are TeenAstro GNSS extensions.

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:gs#` | Full GNSS sync: set site lat/long/elev from GNSS and set time; reinit pole and limits. | `1` / `0` | TeenAstro extension |
| `:gt#` | Time sync from GNSS only. | `1` / `0` | TeenAstro extension |

---

## G — Get (telescope and site information)

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:GA#` | Get telescope altitude. | `sDD*MM#` or `sDD*MM'SS#` | LX200 standard |
| `:GZ#` | Get telescope azimuth. | `DDD*MM#` or `DDD*MM'SS#` | LX200 standard |
| `:G)#` | Get field rotation (horizontal). | `DDD*MM#` or similar | TeenAstro extension |
| `:Ga#` | Get local time (12 h). | `HH:MM:SS#` | LX200 standard |
| `:GC#` | Get calendar date. | `MM/DD/YY#` | LX200 standard |
| `:Gc#` | Get time format (12/24). | `24#` | LX200 standard |
| `:GD#` | Get telescope declination. | `sDD*MM#` or `sDD*MM'SS#` | LX200 standard |
| `:GR#` | Get telescope RA. | `HH:MM.T#` or `HH:MM:SS#` | LX200 standard |
| `:G(#` | Get field rotation (EQ). | DDD*MM# or similar | TeenAstro extension |
| `:GDL#` | Get declination (decimal). | `sDD.VVVVV#` | TeenAstro extension |
| `:GRL#` | Get RA (decimal). | `DDD.VVVVV#` | TeenAstro extension |
| `:Gd#` | Get target declination. | `sDD*MM#` or similar | LX200 standard |
| `:GdL#` | Get target declination (decimal). | `sDD.VVVVV#` | TeenAstro extension |
| `:Ge#` | Get site elevation (m). | `sDDDD#` | TeenAstro extension |
| `:Gf#` | Get sidereal clock speed. | `dd#` | TeenAstro extension |
| `:GG#` | Get UTC offset (hours). | `sHH.H#` | LX200 standard |
| `:Gg#` | Get site longitude. | `DDD*MM#` or `DDD*MM'SS#` | LX200 standard |
| `:Gh#` | Get horizon (higher) limit. | `DD*#` | LX200 standard |
| `:GL#` | Get local time (24 h). | `HH:MM:SS#` | LX200 standard |
| `:GM#` `:GN#` `:GO#` | Get site 1/2/3 name. | `<name>#` | LX200 standard |
| `:Gm#` | Get meridian pier side. | `E#` / `W#` / `N#` / `?#` | TeenAstro extension |
| `:Gn#` | Get current site name. | `<name>#` | TeenAstro extension |
| `:Go#` | Get overhead (lower) limit. | `DD*#` | TeenAstro extension |
| `:Gr#` | Get target RA. | `HH:MM.T#` or similar | LX200 standard |
| `:GrL#` | Get target RA (decimal). | `DDD.VVVVV#` | TeenAstro extension |
| `:GS#` | Get sidereal time. | `HH:MM:SS#` | LX200 standard |
| `:GSL#` | Get sidereal time (decimal). | `HH.VVVVVV#` | TeenAstro extension |
| `:GT#` | Get tracking rate. | `dd.ddddd#` or `0.0#` | LX200 standard |
| `:Gt#` | Get site latitude. | `sDD*MM#` or `sDD*MM'SS#` | LX200 standard |
| `:GVD#` `:GVN#` `:GVP#` `:GVT#` | Version date, number, product, time. | Various | LX200 / Autostar |
| `:GVB#` `:GVb#` | Board / driver version. | `d#` | TeenAstro extension |
| `:GW#` | Get mount status (alignment type, tracking, home/park/align state). | e.g. `GT1#`, `GNH#`, `GP2#` | LX200 standard |

---

## GX — Get (TeenAstro extensions)

All `:GXnn#` commands are TeenAstro extensions. **Standard:** TeenAstro extension.

### Alignment
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXA0#` … `:GXA8#` | Get alignment matrix coefficient (0–8 = t11…t33). | `float#` |

### Encoders
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXE1#` `:GXE2#` | Get encoder position in degrees (axis 1/2). | DMS |
| `:GXEA#` `:GXEZ#` | Get encoder altitude / azimuth. | DMS |
| `:GXED#` `:GXER#` | Get encoder declination / RA. | DMS / HMS |
| `:GXEO#` | Get encoder sync option. | `0`–`n#` |
| `:GXEPD#` `:GXEPR#` | Get encoder pulse per 100° (dec/RA axis). | `ul#` |
| `:GXErD#` `:GXErR#` | Get encoder reverse (0/1). | `0#` / `1#` |

### Debug
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXDB0#` `:GXDB1#` | Debug backlash correcting (axis 1/2). | `0`/`1#` |
| `:GXDB2#` `:GXDB3#` | Debug backlash moved steps (axis 1/2). | steps |
| `:GXDR1#` `:GXDR2#` | Debug requested tracking rate axis 1/2. | `float#` |
| `:GXDR3#` … `:GXDR6#` | Debug current rate, fstep. | `float#` |
| `:GXDP0#` … `:GXDP7#` | Debug position/target/delta/interval. | `long#` / `double#` |
| `:GXDW#` | Get workload (%). | `n%#` |

### Instrument position
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXP1#` `:GXP2#` | Get axis 1/2 position (degrees). | DMS |
| `:GXP3#` `:GXP4#` | Get axis 1/2 from EQ pipeline. | DMS |

### Refraction
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXrp#` `:GXrg#` `:GXrt#` | Get refraction for pole / goto / tracking. | `y#` / `n#` |

### Rates
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXR0#` … `:GXR3#` | Get user-defined rate 0–3. | `dd.dd#` |
| `:GXRA#` | Get degrees for acceleration. | `float#` |
| `:GXRB#` | Get backlash take-up rate. | `long#` |
| `:GXRD#` | Get default rate index. | `0`–`4#` |
| `:GXRX#` | Get max slew rate. | `int#` |
| `:GXRr#` `:GXRh#` `:GXRd#` | Get requested RA/HA/DEC tracking rate. | `long#` |
| `:GXRe#` `:GXRf#` | Get stored RA/DEC rate. | `long#` |

### Limits (user)
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXLA#` `:GXLB#` | Get user min/max axis 1. | `int#` |
| `:GXLC#` `:GXLD#` | Get user min/max axis 2. | `int#` |
| `:GXLE#` `:GXLW#` | Get meridian east/west limit. | `long#` |
| `:GXLU#` | Get under-pole limit (×10). | `long#` |
| `:GXLO#` `:GXLH#` | Get overhead / horizon limit. | `DD*#` |
| `:GXLS#` | Get distance from pole to keep tracking (deg). | `DD*#` |

### Limits (mount type)
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXlA#` `:GXlB#` | Get mount min/max axis 1. | `int#` |
| `:GXlC#` `:GXlD#` | Get mount min/max axis 2. | `int#` |

### Time
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXT0#` | Get UTC time. | `HH:MM:SS#` |
| `:GXT1#` | Get UTC date. | `MM/DD/YY#` |
| `:GXT2#` | Get seconds since 1970-01-01. | `ul#` |
| `:GXT3#` | Get LHA time. | `HH:MM:SS#` |

### Status
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXI#` | Get telescope status string (tracking, park, guide, pier, etc.). | 18 chars + `#` |
| `:GXJB#` | Both rate axes enabled. | `1#` / `0#` |
| `:GXJC#` | Connected. | `1#` |
| `:GXJm#` | Motors enabled. | `1#` / `0#` |
| `:GXJM1#` `:GXJM2#` | Axis 1/2 moving. | `1#` / `0#` |
| `:GXJP#` | Pulse guiding active. | `1#` / `0#` |
| `:GXJS#` | Slewing. | `1#` / `0#` |
| `:GXJT#` | Tracking. | `1#` / `0#` |

### Motors (axis: R = axis1, D = axis2)
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXMBR#` `:GXMBD#` | Backlash amount. | `int#` |
| `:GXMbR#` `:GXMbD#` | Backlash rate. | `int#` |
| `:GXMGR#` `:GXMGD#` | Gear. | `ul#` |
| `:GXMSR#` `:GXMSD#` | Steps per rotation. | `u#` |
| `:GXMMR#` `:GXMMD#` | Microstep. | `u#` |
| `:GXMmR#` `:GXMmD#` | Silent mode. | `0`/`1#` |
| `:GXMRR#` `:GXMRD#` | Reverse. | `0`/`1#` |
| `:GXMCR#` `:GXMCD#` | High current (mA). | `u#` |
| `:GXMcR#` `:GXMcD#` | Low current (mA). | `u#` |
| `:GXMLR#` `:GXMLD#` | StallGuard. | `int#` |
| `:GXMIR#` `:GXMID#` | Driver current. | `u#` |

### Options
| Syntax | Description | Returns |
|--------|-------------|---------|
| `:GXOI#` | Mount index. | `int#` |
| `:GXOA#` | Current mount name. | `string#` |
| `:GXOB#` `:GXOC#` | First/second mount name. | `string#` |
| `:GXOS#` | Slew settle duration (s). | `int#` |

---

## h — Home / park

Meade LX200 uses `h` only on 16" for home search. TeenAstro uses `h` for home and park.

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:hF#` | Reset/sync at home position (cold start). | (nothing) | TeenAstro extension |
| `:hC#` | Go to home position. | `1` / `0` | TeenAstro extension |
| `:hB#` | Set current position as home. | `1` / `0` | TeenAstro extension |
| `:hb#` | Reset home (clear saved home). | `1` | TeenAstro extension |
| `:hO#` | Sync at park position. | `1` / `0` | TeenAstro extension |
| `:hP#` | Go to park position. | `1` / `0` | TeenAstro extension |
| `:hQ#` | Set current position as park. | `1` / `0` | TeenAstro extension |
| `:hS#` | Query if park position is saved. | `1` / `0` | TeenAstro extension |
| `:hR#` | Unpark (restore from park). | `1` | TeenAstro extension |

---

## M — Move / slew

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:Me#` `:Mw#` | Move east / west at current rate. | (nothing) | LX200 standard |
| `:Mn#` `:Ms#` | Move north / south at current rate. | (nothing) | LX200 standard |
| `:MS#` | Slew to current target (object). | `0`–`6` (ERRGOTO) | LX200 standard |
| `:MA#` | Slew to target Alt/Az. | `0`–`6` (ERRGOTO) | LX200 standard |
| `:MU#` | Slew to user-defined position (RA/Dec from EEPROM). | `0`–`6` (ERRGOTO) | TeenAstro extension |
| `:MF#` | Meridian flip (GEM). | ERRGOTO code | TeenAstro extension |
| `:Mge&lt;ms&gt;#` `:Mgw&lt;ms&gt;#` | Pulse guide east/west (duration ms, 1–30000). | (nothing) | TeenAstro extension |
| `:Mgn&lt;ms&gt;#` `:Mgs&lt;ms&gt;#` | Pulse guide north/south (duration ms). | (nothing) | TeenAstro extension |
| `:M1nnnn#` `:M2nnnn#` | Move axis 1/2 at given rate (arcsec/s). | `1` or `e`/`h`/`s`/`g`/`i` | TeenAstro extension |

---

## Q — Halt

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:Q#` | Halt all motion and goto. | (nothing) | LX200 standard |
| `:Qe#` `:Qw#` | Halt east/west motion. | (nothing) | LX200 standard |
| `:Qn#` `:Qs#` | Halt north/south motion. | (nothing) | LX200 standard |

---

## R — Slew rate

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:RG#` | Set rate to guide. | (nothing) | LX200 standard |
| `:RC#` | Set rate to center. | (nothing) | LX200 standard |
| `:RM#` | Set rate to find. | (nothing) | LX200 standard |
| `:RS#` | Set rate to slew. | (nothing) | LX200 standard |
| `:R0#` … `:R4#` | Set custom rate index 0–4. | (nothing) | LX200 standard |

---

## S — Set (telescope and site parameters)

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:S!n#` | Set mount type (reboot). | `1` / `0` | TeenAstro extension |
| `:Sa…#` | Set target altitude. | `1` / `0` | LX200 standard |
| `:SBn#` | Set baud rate. | `1` / `0` | LX200 standard |
| `:SC MM/DD/YY#` | Set date. | `1` / `0` | LX200 standard |
| `:Sd sDD*MM#` / `:SdL,sVV.VVVVV#` | Set target declination (DMS or decimal). | `1` / `0` | LX200 / TeenAstro |
| `:Sg…#` | Set longitude. | `1` / `0` | LX200 standard |
| `:SG sHH#` | Set UTC offset. | `1` / `0` | LX200 standard |
| `:Sh DD#` | Set horizon limit. | `1` / `0` | LX200 standard |
| `:SL HH:MM:SS#` | Set local time. | `1` / `0` | LX200 standard |
| `:Se sDDDDD#` | Set site elevation (m). | `1` / `0` | TeenAstro extension |
| `:Sm E#` `:Sm W#` `:Sm N#` | Set pier side for next target (E/W/N; N = any). | `1` / `0` | TeenAstro extension |
| `:Sn &lt;name&gt;#` | Set current site name. | `1` / `0` | TeenAstro extension |
| `:So DD#` | Set overhead limit (60–91°). | `1` / `0` | LX200 standard |
| `:Sr HH:MM.T#` `:Sr HH:MM:SS#` | Set target RA. | `1` / `0` | LX200 standard |
| `:SrL,VVV.VVVVV#` | Set target RA (decimal degrees). | `1` / `0` | TeenAstro extension |
| `:SS HH:MM:SS#` | Set sidereal time. | `1` / `0` | LX200 standard |
| `:ST dd.ddddd#` | Set tracking rate (30–90 or &lt;0.1 to disable). | `1` / `0` | LX200 standard |
| `:St sDD*MM#` | Set latitude (at home/park only). | `1` / `0` | LX200 standard |
| `:SU#` | Store current position as user RA/Dec in EEPROM. | `1` | TeenAstro extension |
| `:Sz…#` | Set target azimuth. | `1` / `0` | LX200 standard |
| `:SM#` `:SN#` `:SO#` | Set site 1/2/3 name. | `1` / `0` | LX200 standard |

---

## SX — Set (TeenAstro extensions)

All `:SXnnn,V#` commands are TeenAstro extensions. **Standard:** TeenAstro extension. Returns `1`/`0` unless noted.

### Alignment
| Syntax | Description |
|--------|-------------|
| `:SXAn,VVVVVV#` | Set alignment model value (n = 0–5, x). |

### Encoders
| Syntax | Description |
|--------|-------------|
| `:SXEE,y#` | Enable/disable encoders (y/n). |
| `:SXEO,n#` | Set encoder sync option. |
| `:SXEPD,nnnnn#` `:SXEPR,nnnnn#` | Set pulse per 100° (dec/RA axis). |
| `:SXErD,V#` `:SXErR,V#` | Set encoder reverse (0/1). |

### Refraction
| Syntax | Description |
|--------|-------------|
| `:SXrp,y#` `:SXrg,y#` `:SXrt,y#` | Set refraction for pole / goto / tracking (y/n). |

### Rates
| Syntax | Description |
|--------|-------------|
| `:SXR0,VVV#` … `:SXR3,VVV#` | Set user-defined rate 0–3. |
| `:SXRA,VVV#` | Set degrees for acceleration. |
| `:SXRD,V#` | Set default rate index (0–4). |
| `:SXRX,VVVV#` | Set max slew rate. |
| `:SXRr,VVVVVVVVVV#` `:SXRh,…#` `:SXRd,…#` | Set RA/HA/DEC tracking rate. |
| `:SXRe,VVVVVVVVVV#` `:SXRf,…#` | Store RA/DEC rate in EEPROM. |

### Limits
| Syntax | Description |
|--------|-------------|
| `:SXLR#` | Reset user axis limits. |
| `:SXLA,VVVV#` `:SXLB,VVVV#` | Set user min/max axis 1. |
| `:SXLC,VVVV#` `:SXLD,VVVV#` | Set user min/max axis 2. |
| `:SXLE,sVV.V#` `:SXLW,sVV.V#` | Set meridian east/west limit. |
| `:SXLU,VV#` | Set under-pole limit. |
| `:SXLH,sVV#` `:SXLO,VV#` | Set horizon / overhead limit. |
| `:SXLS,sVV#` | Set distance from pole to keep tracking (deg). |

### Time
| Syntax | Description |
|--------|-------------|
| `:SXT0HH:MM:SS#` | Set UTC time. |
| `:SXT1MM/DD/YY#` | Set UTC date. |
| `:SXT2nnnnn#` | Set time from Unix timestamp. |

### Motors (axis: R = axis1, D = axis2)
| Syntax | Description |
|--------|-------------|
| `:SXME,y#` | Enable/disable motors (y/n). |
| `:SXMBR,VVVV#` `:SXMBD,VVVV#` | Set backlash amount. |
| `:SXMbR,VVVV#` `:SXMbD,VVVV#` | Set backlash rate. |
| `:SXMGR,VVVV#` `:SXMGD,VVVV#` | Set gear. |
| `:SXMSR,VVVV#` `:SXMSD,VVVV#` | Set steps per rotation. |
| `:SXMMR,V#` `:SXMMD,V#` | Set microstep (1–8). |
| `:SXMmR,V#` `:SXMmD,V#` | Set silent/coolstep mode (0/1). |
| `:SXMRR,V#` `:SXMRD,V#` | Set reverse (0/1). |
| `:SXMCR,VVVV#` `:SXMCD,VVVV#` | Set high current (mA). |
| `:SXMcR,VVVV#` `:SXMcD,VVVV#` | Set low current (mA). |
| `:SXMFn,VVV#` | Set StallGuard (n = R/D). |

### Options
| Syntax | Description |
|--------|-------------|
| `:SXOI,V#` | Set mount index (reboot). |
| `:SXOA,&lt;name&gt;#` `:SXOB,…#` `:SXOC,…#` | Set mount name (current / first / second). |
| `:SXOS,NNN#` | Set slew settle duration (s). |

---

## T — Tracking

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:T+#` `:T-#` | Increment / decrement sidereal rate. | (nothing) | LX200 standard |
| `:TQ#` `:TM#` | Switch to quartz / manual tracking. | (nothing) | LX200 standard |
| `:TR#` | Reset sidereal rate to default. | (nothing) | TeenAstro extension |
| `:TS#` `:TL#` | Solar / lunar tracking rate. | (nothing) | TeenAstro extension |
| `:Te#` `:Td#` | Enable / disable tracking. | `1` / `0` | TeenAstro extension |
| `:T1#` `:T2#` | Tracking compensation RA only / both. | `1` / `0` | TeenAstro extension |
| `:TT#` | Set tracking to user-defined target rate (stored RA/DEC drift). | (nothing) | TeenAstro extension |
| `:TXA0#` … `:TXT1#` | Test/alignment commands (build-dependent). | (varies) | TeenAstro extension |

---

## U — Precision

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:U#` | Toggle high/low precision (coordinates in DMS/HMS). | (nothing) | LX200 standard |

---

## W — Site (waypoint)

| Syntax | Description | Returns | Standard |
|--------|-------------|---------|----------|
| `:W0#` `:W1#` `:W2#` | Select site 0, 1, or 2. | (nothing) | LX200 standard |
| `:W?#` | Get current site index. | `n#` | LX200 standard |
