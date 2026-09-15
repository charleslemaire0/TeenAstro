# TeenAstro WiFi Server

WiFi bridge and embedded web server firmware for TeenAstro telescope controllers. It exposes the MainUnit's LX200 command protocol over TCP and provides a web-based configuration interface for telescope settings and network management.

**Full documentation:** [docs/firmware/server.md](../docs/firmware/server.md)

---

## Architecture & Features

The server runs on an **ESP8266** (Wemos D1 Mini / ESP-12) or **ESP32** module connected to the MainUnit via high-speed serial UART (115200 baud):

- **TCP Command Bridge (Port 9999)**:
  - Listens on TCP port `9999` for incoming connections from Planetarium software, ASCOM drivers, and the TeenAstro Mobile/Desktop App.
  - Transparently forwards LX200 text commands and high-speed binary bulk telemetry packets (`:GXAS#` / `:GXCS#`) to/from the MainUnit.
- **Dual WiFi Modes**:
  - **Access Point (AP) Mode**: Creates a standalone network (Default SSID: `TeenAstro`, IP: `192.168.0.1`).
  - **Station (STA) Mode**: Connects to an existing observatory or home WiFi network with automatic fallback.
- **Embedded Web Portal (HTTP Port 80)**:
  - Network configuration (SSID, WiFi password, AP channel, security).
  - Web authentication password protection.
  - Telescope observing site settings (Latitude, Longitude, Timezone, UTC offset).
- **Safety & Memory Protection**:
  - Bounded input parsing and buffer-overflow hardened string handling (`strncpy`, fixed-size buffers) to ensure stable long-running operation.

---

## Building Firmware

Build with [PlatformIO](https://platformio.org/):

```bash
# From repository root
pio run -d TeenAstroServer

# Upload to connected ESP8266 module
pio run -d TeenAstroServer -t upload
```

---

## Connection Reference

| Setting | Default Value | Description |
|---------|---------------|-------------|
| **Default AP SSID** | `TeenAstro` | Access Point network name |
| **Default AP IP** | `192.168.0.1` | Gateway IP address |
| **TCP Control Port** | `9999` | LX200 & Bulk Protocol socket |
| **HTTP Web UI Port** | `80` | Web configuration interface |
| **Baud Rate to MainUnit** | `115200` | Serial UART bridge baud rate |
