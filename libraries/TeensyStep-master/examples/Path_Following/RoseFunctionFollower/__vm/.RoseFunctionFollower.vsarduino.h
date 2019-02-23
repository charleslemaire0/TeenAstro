/* 
	Editor: https://www.visualmicro.com/
			visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
			all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Teensy 3.2 / 3.1, Platform=teensy3, Package=teensy
*/

#if defined(_VMICRO_INTELLISENSE)

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __HARDWARE_MK20dx256__
#define __HARDWARE_MK20DX256__
#define __MK20DX256__
#define TEENSYDUINO 141
#define ARDUINO 10805
#define F_CPU 72000000
#define USB_SERIAL
#define LAYOUT_US_ENGLISH
#define __cplusplus 201103L
#define __arm__
#define __ARM__
#define  __attribute__(x)
typedef void *__builtin_va_list;
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__
#define NEW_H

#define __STDC__ 

#include <arduino.h>
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))



#define __disable_irq() __asm__ volatile("");
#define __enable_irq()	__asm__ volatile("");


#include "RoseFunctionFollower.ino"
#endif
#endif
