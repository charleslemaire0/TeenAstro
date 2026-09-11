// -------------------------------------------------------------------------------
// Configuration

#pragma once

// Enable debugging messages on DebugSer
#define DEBUG_OFF                 // default=_OFF, use "DEBUG_ON" to activate
#define DebugSer Serial           // default=Serial, or Serial1 for example (always 9600 baud)
#define DEBUGBUTTON_OFF           // defualt=_OFF, use "DEBUGBUTTON" to activate

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#define SERIAL_BAUD 57600
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
#define SERIAL_BAUD 57600
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
#define SERIAL_BAUD 57600
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
#define SERIAL_BAUD 57600
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
// Use the D0–D8 / A0 silkscreen positions (not the inner IO35/IO36 I2C pads).
#ifdef ARDUINO_LOLIN_S3_MINI
#define SERIAL_BAUD 57600
#define DEBUG_OFF
#define SHC_SERIAL_RX 44            // RX
#define SHC_SERIAL_TX 43            // TX
#define SHC_I2C_SCL   1             // D1 socket
#define SHC_I2C_SDA   2             // D2 socket
#define B_PIN0  8                   // D8 Shift
#define B_PIN1  7                   // D7 N
#define B_PIN2  6                   // D6 S
#define B_PIN3  0                   // D0 E
#define B_PIN4  5                   // D5 W
#define B_PIN5  3                   // D3 F
#define B_PIN6  4                   // D4 f

#define B_PIN_UP_0 false            // true for active LOW, false if active HIGH
#define B_PIN_UP_1 false
#define B_PIN_UP_2 false
#define B_PIN_UP_3 false
#define B_PIN_UP_4 false
#define B_PIN_UP_5 true
#define B_PIN_UP_6 true
#define A_SCREEN 1                  // A0 (same GPIO as D1; read before I2C init)
#endif


