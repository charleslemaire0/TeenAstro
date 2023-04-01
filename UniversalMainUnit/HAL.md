# TeenAstro Hardware-Abstraction Layer  

### Introduction

The Arduino framework and libraries already provide most of the abstraction layer (UART, SPI etc.), but some functionality is still different between builds. This includes:

- Chip-specific code

  - chip reset

  - programming hardware timers

  - installing and running interrupt service routines (ISR)

  - non-volatile memory

- Board-specific code
  - Pin definitions and multiplexing
  - other peripherals on the PCB (real-time clock etc.)

At this time, the HAL is implemented only for the Universal Main Unit.

### HAL definitions

```
unsigned long 	HAL_getRealTimeClock(void);
void       		HAL_setRealTimeClock(unsigned long t);
void 			HAL_reboot(void);
void  			HAL_beginTimer(void f(void), unsigned long);
void            HAL_EEPROM_begin(void);
```

### Non-Volatile Memory 

EEPROM is used to store the parameters for up to 4 sites (geographical position and time zone) and up to 2 mounts (type of mount, motor parameters etc.). A small header contains a magic number that triggers automatic initialization of sites and mounts.

Both Teensy 4 and ESP32 implement the Arduino EEPROM library on top of the Flash and provide 1kB of storage, but the interface is slightly different: ESP32 requires an explicit call to EEPROM.commit(), whereas Teensy4 provides an EEPROM.update() function that only commits if the value has changed. The library is implemented as follows: 


#### EEPROM Memory Map

| Section | Index | Address | Size (bytes) |
| ------- | ----- | ------- | ------------ |
| Header  | 0     | 0       | 128          |
| Sites   | 0     | 128     | 32           |
|         | 1     | 160     | 32           |
|         | 2     | 192     | 32           |
|         | 3     | 224     | 32           |
| Mounts  | 0     | 256     | 256          |
|         | 1     | 512     | 256          |


``` 
// Access functions
int XEEPROM.read(address);
int XEEPROM,write(address, value);
.. and other functions for reading/writing different data types

// To retrieve the address of a variable in a given section
int getMountAddress(int adress, int ndx);
int getSiteAddress(int adress, int ndx);

// Example 
XEEPROM.read(getMountAddress(EE_mountType), 0);
```



 #### Site and Mount Definitions

 See EEPROM_Address.h

