// -------------------------------------------------------------------------------
// Configuration

// at startup this firmware will attempt to switch OnStep's baud rate to a faster speed and AFTER success, start WiFi, etc.
// valid baud rates are 115200, 57600, 38400, 28800, 19200, 9600
#ifndef CONFIG_H
#define CONFIG_H
#define PEC_OFF
#define SERIAL_BAUD 57600         // Default=57600

// misc. options that are usually not changed
#define DEBUG_OFF                 // Turn _ON to allow WiFi startup without OnStep attached (Serial port for debug at 115200 baud)
#define Ser Serial                // Default=Serial, This is the hardware serial port where OnStep is attached

// On first startup an AP will appear with an SSID of "TeenAstro", after connecting:
// The web-site is at "192.168.0.1" and the cmd channel is at "192.168.0.1:9999".
//
// -------------------------------------------------------------------------------

#endif