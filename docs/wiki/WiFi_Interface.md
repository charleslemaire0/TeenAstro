# WiFi Interface

> Groups.io: https://groups.io/g/TeenAstro/wiki/14592

The Wi‑Fi offers two operation modes: **Access Point** and **Station**. You switch between them on the hand controller (**Telescope Settings → Wifi → Select Mode**). Network names and passwords are set in the [Webserver](https://groups.io/g/TeenAstro/wiki/8105) WiFi tab (password default **password**).

## Access Point

TeenAstro works as a wireless access point. Connect a notebook, phone or tablet to the TeenAstro WLAN, then use SkySafari, ASCOM, or open the [Webserver](https://groups.io/g/TeenAstro/wiki/8105) (default **192.168.0.1**).

While you are connected to TeenAstro you usually have no Internet on that device.

## Station mode

TeenAstro joins your home or observatory router. A notebook stays on the usual WLAN and can reach both TeenAstro and the Internet. Configure the router SSID first in the Webserver, then select that profile on the hand controller.

### Example configuration

1. Connect to the TeenAstro WLAN with a notebook (the password is **password**).
2. Open the [Webserver](https://groups.io/g/TeenAstro/wiki/8105) and go to the **WiFi** tab. Enter **password** to unlock the configuration.
3. Configure **Station mode 0**:
   - **SSID** — name of your router network.
   - **Password** — password of your router network.
   - Leave **Enable DHCP** checked unless you need a fixed address.
4. Click **Upload**.
5. Leave the Webserver and disconnect from the TeenAstro WLAN.
6. Power TeenAstro off and on.
7. On the hand controller: **Telescope Settings** (Shift + West).
8. **Wifi → Select Mode**. The configured SSID should appear.
9. Select that SSID and confirm. The hand controller reboots.
10. When the Wi‑Fi icon shows a solid link, TeenAstro is connected to the router. If not, the signal is too weak or out of range.
11. Read the address: **Telescope Settings → Wifi → Show IP**.
12. Connect your notebook to the router as usual and open the Webserver at that IP.

## Related

- [Webserver](https://groups.io/g/TeenAstro/wiki/8105)
