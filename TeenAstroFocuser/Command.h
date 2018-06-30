// Command.h

#ifndef _COMMAND_h
#define _COMMAND_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// single key commands
#define AzCmd_Help 'h'
#define AzCmd_Version 'v'
#define FocCmd_Goto 'g'
#define FocCmd_Park 'p'
#define FocCmd_Sync 's'
#define FocCmd_Halt 'x'

#define FocCmd_Write 'w'

#define CmdDumpState '?'
#define CmdDumpConfig '~'

#define FocCmd_startPosition '0'
#define FocCmd_maxPosition '1'
#define FocCmd_minSpeed '2'
#define FocCmd_maxSpeed '3'
#define FocCmd_cmdAcc '4'
#define FocCmd_manualAcc '5'
#define FocCmd_manualDec '6' 
#define FocCmd_Inv '7'

#define Char_CR 13
#define Char_Spc 32
#define Char_254 254

void Command_Check();

#endif

