# TeenAstroPad

Button input library for the TeenAstro Smart Hand Controller (SHC). Handles a 7-button directional pad with click, double-click, and long-press detection, plus WiFi control integration.

## Buttons

| Index | Name | Menu action |
|-------|------|-------------|
| 0 | Shift | Home |
| 1 | North | Up |
| 2 | South | Down |
| 3 | East | Previous |
| 4 | West | Next |
| 5 | F | Select |
| 6 | f | Home |

## Main class: `Pad`

### Setup

```cpp
Pad pad;
pad.setup(pins, activeLevels, eepromAddress, rotated);
pad.setMenuMode();       // Slow/Medium/Fast UI click & long-press timings
pad.setControlerMode();  // same debounce; N/S/E/W/F/f start motion quickly
```

PCB buttons are RC-filtered; software debounce is fixed at 15 ms for all speeds.
Button Speed only changes menu click/long-press:

| Speed  | Debounce | Click | Long-press |
|--------|----------|-------|------------|
| Slow   | 15 ms    | 280 ms | 550 ms    |
| Medium | 15 ms    | 180 ms | 380 ms    |
| Fast   | 15 ms    | 110 ms | 250 ms    |

### Input handling

```cpp
pad.tickButtons();               // poll all buttons (call in loop)
if (pad.buttonPressed()) {
    // check eventbuttons[B_NORTH], eventbuttons[B_F], etc.
}
```

### Button events

Events are stored in `eventbuttons[7]` (volatile, extern):

| Event | Description |
|-------|-------------|
| `E_NONE` | No event |
| `E_CLICK` | Single click |
| `E_DOUBLECLICK` | Double click |
| `E_LONGPRESSTART` | Long press started |
| `E_LONGPRESS` | Long press held |
| `E_LONGPRESSSTOP` | Long press released |

### Two modes

- **Menu mode** — longer click/long-press thresholds for UI navigation
- **Controller mode** — shorter thresholds for responsive slewing

### Button speed

Configurable via `getButtonSpeed()` / `setButtonSpeed(BS_SLOW | BS_MEDIUM | BS_FAST)`, stored in EEPROM.

### Rotation support

When `rotated == true`, North/South and East/West pins are swapped for upside-down mounting.

### WiFi control

| Method | Description |
|--------|-------------|
| `isWifiOn()` | WiFi enabled? |
| `turnWifiOn(on)` | Enable/disable |
| `getIP(buf)` | Get IP address |
| `getWifiMode()` / `setWifiMode(k)` | AP/station mode |

## Dependencies

- **OneButton** — click/double-click/long-press detection
- **TeenAstroWifi** — WiFi setup and mode control
