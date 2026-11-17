# Webserver

> Groups.io: https://groups.io/g/TeenAstro/wiki/8105

The web server is where you set the site, the mount, the motors, the limits and the Wi‑Fi. Connect a computer or a phone to the TeenAstro network, then open a browser at **192.168.0.1** (the default Access Point address).

If that address does not answer, read the one in use on the hand controller: **Telescope Settings → Wifi → Show IP**. The two Wi‑Fi modes (Access Point and Station) are described on the [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592) page.

Screenshots below are from firmware **1.6** (dark theme). The navigation bar lists Status, Control, Speed, Tracking, Site, Mount, Motors, Limits and WiFi. Encoders and Focuser appear when those options are enabled. Reticule appears on an equatorial mount.

## Status

Live mount state: time, coordinates, pier side, alignment, tracking and parking. **Last Error** shows **RTC battery empty** when the coin cell on the main unit is empty or missing. After a full power-off the clock shows the firmware build time, or 1 January 2019 on a Teensy 4 main unit. Set the time on the Site page, or from GNSS. Sky gotos and sync stay refused until then.

## Control

Manual moves and common night actions from a browser.

## Site

Latitude, longitude, elevation and time / time zone.

## Mount

Mount type, motors and encoders. **Mount error** sits above Refraction: **Off** / **On** (“Hold CH and NP”), then CH and NP in degrees (±5, Wallace’s signs). Same meaning as on the hand controller. After **Save** on a **4 Stars** or **3+3 Stars** alignment, these CH and NP fields are filled from the fit (they are not left at 0.000°).

## Reticule

Equatorial mounts only. The circle is the pole star's true distance from the pole: Polaris in the north, Sigma Octantis in the south. Put 12 at the top. The clock in the bottom-right corner is the observatory local time from the mount. That time already includes the site longitude, so the longitude correction on the polar finder stays at 0. Set the finder to that clock and place the pole star on the circle. **Brighter** and **Less Bright** sit with the clock, clear of the circle.

On the hand controller: **Telescope Settings → Mount → Reticule → Finder**. The same circle is on the left and the local time is beside it. East makes the illuminator brighter, West makes it dimmer.

## Motors

Gear ratios, steps, microsteps, currents and backlash for each axis.

## Speed / Tracking

Guiding and slew rates, and tracking rates / corrections.

## Limits

Horizon, under-pole, meridian and axis limits. Each limit has a **Default** button that restores the factory value for that limit only.

## WiFi

Enter the password (**password** by default) to change networks, Access Point settings and firmware update. Step-by-step Station mode setup is on the [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592) page.

## Related

- [WiFi Interface](https://groups.io/g/TeenAstro/wiki/14592)
- Field manuals: [EN](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_en.pdf) · [FR](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_fr.pdf) · [DE](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/docs/manuel/Manuel_utilisateur_de.pdf)
