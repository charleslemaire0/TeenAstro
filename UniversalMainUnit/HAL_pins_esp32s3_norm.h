#pragma once

#define Axis1DirPin      3
#define Axis1StepPin    17
#define Axis2DirPin     46
#define Axis2StepPin    48

#define Axis1EnablePin   4
#define Axis2EnablePin   5
#define Axis1CSPin      10
#define Axis2CSPin      45
#define FocusCSPin       2

#define AxisSDIPin      11
#define AxisSDOPin      13
#define AxisSCKPin      12

// Serial interfaces
// Serial (SHC1)
#define USBRx           19  //USB-
#define USBTx           20  //USB+
// Serial1
#define GNSSRx          15
#define GNSSTx          16
// Serial0 (SHC)
#define SHCRx           44
#define SHCTx           43
// Serial0 (debug)
#define UARTRx          44
#define UARTTx          43

// The ESP32S3Devkit-c and TeenAstroESP32s3 handle
//  ARDUINO_USB_MODE, ARDUINO_CDC_ON_BOOT
//  Serial0 (pins 43, 44) (board-label:UART) TA-SHC
//  Serial1 (pins 15, 16)                    TA-GNSS
//  Serial  (pins 19, 20) (board-label:USB)  TA-SHC1
//

// Need ARDUINO_USB_CDC_ON_BOOT for UART and USB serials
#ifdef ARDUINO_USB_CDC_ON_BOOT
#define UARTSerial  Serial0         // Defined in HardwareSerial.h
#define USBSerial   Serial          // Defined in HWCDC.h (rename it to USBSerial because it uses USB connector)
#endif

#ifdef TESTING
#define SHCSerial       Serial1         // Testing = GPIO Pins 16, 15
#else
#define SHCSerial       Serial0         // Running = GPIO Pins 44, 43
#endif // TESTING

#define SHC1Serial      USBSerial       // GPIO Pins 20, 19 (HWCDC, USBCDC)

#define debugSerial     UARTSerial      // UART


// Fixed serial interfaces
#define SHC1Serial      USBSerial       // GPIO Pins 20, 19 (HWCDC, USBCDC)
#define GNSSSerial      Serial1         // GPIO Pins 16, 15
#define debugSerial     UARTSerial      // UART

// ST4 interface
#undef HASST4
/*
#define ST4RAe          39              // ST4 East
#define ST4DEs          41              // ST4 South
#define ST4DEn          40              // ST4 North
#define ST4RAw          42              // ST4 West
*/

// Wire interfaces (defined by espressif)
//#define MOSI            11   
//#define MISO            13
//#define SCK             12

#define OWD              1          

#define SDA              8          
#define SCL              9          

#ifdef LED_BUILTIN
#undef LED_BUILTIN
#endif
#define LED_BUILTIN 	18
#define LEDPin 	        14
