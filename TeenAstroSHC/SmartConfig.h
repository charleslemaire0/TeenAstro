// -------------------------------------------------------------------------------
// Configuration

#pragma once

// Enable debugging messages on DebugSer
#define DEBUG_OFF                 // default=_OFF, use "DEBUG_ON" to activate
#define DebugSer Serial           // default=Serial, or Serial1 for example (always 9600 baud)
#define DEBUGBUTTON_OFF           // defualt=_OFF, use "DEBUGBUTTON" to activate
#define SHC_STARTUP_BUTTON_TEST_OFF // default=_OFF; set SHC_STARTUP_BUTTON_TEST to show pad diag at boot

// Link to the MainUnit. Not board-specific. This is the rate every MainUnit
// boots at, so first contact always works; the SHC then asks for the faster
// rate itself (SHC_BAUD_FAST in SmartController.h, which the .cpp can see too).
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 57600
#endif

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#define DEBUG_OFF
// the hand controller buttons
#define B_PIN0 D8               // Shift
#define B_PIN1 D7               // N
#define B_PIN2 D6               // S
#define B_PIN3 D0               // E
#define B_PIN4 D5               // W
#define B_PIN5 D3               // F
#define B_PIN6 D4               // f

#define B_PIN_UP_0 false        // true for active LOW, false if active HIGH
#define B_PIN_UP_1 false
#define B_PIN_UP_2 false
#define B_PIN_UP_3 false
#define B_PIN_UP_4 false
#define B_PIN_UP_5 true
#define B_PIN_UP_6 true
#define A_SCREEN A0
#endif

#ifdef ARDUINO_LOLIN_C3_MINI
#define DEBUG_OFF
// the hand controller buttons
#define B_PIN0 5               // Shift
#define B_PIN1 4               // N
#define B_PIN2 3               // S
#define B_PIN3 1               // E
#define B_PIN4 2               // W
#define B_PIN5 7               // F
#define B_PIN6 6               // f

#define B_PIN_UP_0 false        // true for active LOW, false if active HIGH
#define B_PIN_UP_1 false
#define B_PIN_UP_2 false
#define B_PIN_UP_3 false
#define B_PIN_UP_4 false
#define B_PIN_UP_5 true
#define B_PIN_UP_6 true
#define A_SCREEN 0

#endif

#ifdef ARDUINO_TTGO_LoRa32_V1
#define DEBUG_OFF
// the Keypad controller buttons
#define B_PIN0 21               // Row1
#define B_PIN1 22               // Row2
#define B_PIN2 17               // Row3
#define B_PIN3  2               // Row4
#define B_PIN4 15               // Col1
#define B_PIN5 13               // Col2
#define B_PIN6 12               // Col3

#define B_PIN_UP_0 false        // true for active LOW, false if active HIGH
#define B_PIN_UP_1 false
#define B_PIN_UP_2 false
#define B_PIN_UP_3 false
#define B_PIN_UP_4 false
#define B_PIN_UP_5 false
#define B_PIN_UP_6 false
#endif

#ifdef ARDUINO_ESP32_DEV
#define DEBUG_OFF
// the hand controller buttons
#define B_PIN0 32               // Shift
#define B_PIN1 33               // N
#define B_PIN2 34               // S
#define B_PIN3 35               // E
#define B_PIN4 36               // W
#define B_PIN5 39               // F
#define B_PIN6  2               // f

#define B_PIN_UP_0 false        // true for active LOW, false if active HIGH
#define B_PIN_UP_1 false
#define B_PIN_UP_2 false
#define B_PIN_UP_3 false
#define B_PIN_UP_4 false
#define B_PIN_UP_5 true
#define B_PIN_UP_6 true
#endif

// LOLIN S3 Mini — same SHC PCB as the Wemos D1 Mini (ESP8266).
// Dual-row headers: only the OUTER row mates with the D1 Mini socket.
// Outer map (USB at bottom, antenna at top) from board silk / schematic:
//   Left:  EN, 2, 4, 12, 13, 11, 10, 3V3  = RST, A0, D0, D5, D6, D7, D8, 3V3
//   Right: 43, 44, 36, 35, 18, 16, G, 5V = TX, RX, D1, D2, D3, D4, G, 5V
// Do NOT use D#==GPIO# (inner row / CircuitPython alias, not the shield row).
#ifdef ARDUINO_LOLIN_S3_MINI
#define DEBUG_OFF
#define SHC_SERIAL_RX 44            // RX
#define SHC_SERIAL_TX 43            // TX
// Display already verified on these pins with this PCB + module.
#define SHC_I2C_SCL   1
#define SHC_I2C_SDA   2
#define B_PIN0  10                  // D8 Shift
#define B_PIN1  11                  // D7 N
#define B_PIN2  13                  // D6 S
#define B_PIN3  4                   // D0 E
#define B_PIN4  12                  // D5 W
#define B_PIN5  18                  // D3 F
#define B_PIN6  16                  // D4 f

// Same resistor PCB as Wemos D1 Mini:
//   Shift/N/S/E/W  and  F/f  use opposite polarities.
// Pin map OK on S3; sense was inverted vs the pad (same flags as Wemos).
#define B_PIN_UP_0 false            // Shift  active HIGH
#define B_PIN_UP_1 false            // N
#define B_PIN_UP_2 false            // S
#define B_PIN_UP_3 false            // E
#define B_PIN_UP_4 false            // W
#define B_PIN_UP_5 true             // F   inverse of NEWSShift
#define B_PIN_UP_6 true             // f   inverse of NEWSShift
#define A_SCREEN 2                  // A0 outer
#endif


