// Command.h

#ifndef _COMMAND_h
#define _COMMAND_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// single key commands
#define AzCmd_Help 'H'
#define AzCmd_Version 'V'
#define FocCmd_out_wor '+'
#define FocCmd_in_wor '-'
#define FocCmd_out 'O'
#define FocCmd_out_stop 'o'
#define FocCmd_in 'I'
#define FocCmd_in_stop 'i'
#define FocCmd_Goto 'G'
#define FocCmd_Park 'P'
#define FocCmd_Sync 'S'
#define FocCmd_Halt 'Q'

#define FocCmd_Write 'W'

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
#define FocCmd_inpulse '8'

#define Char_CR 13
#define Char_Spc 32
#define Char_254 254



class SerCom
{
public:
  SerCom(Stream& s) :ser(s) {}

private:
  Stream& ser;
  char m_command;
  unsigned int m_value = 0;
  bool m_valuedefined = false;
  bool m_hasReceivedCommand = false;
public:
  void updateGoto();
  void Command_Check();
  void Get_Command();
  void MoveRequest(void);
private:
  void dumpState();
  void dumpConfig();

  void sayHello();
  void setbool(bool valuedefined, unsigned int value, bool  &adress);
  void setvalue(bool valuedefined, unsigned int value, unsigned int min, unsigned int max, unsigned int &adress);
  void setvalue(bool valuedefined, unsigned long value, unsigned long min, unsigned long max, unsigned long &adress);
  void setvalue(bool valuedefined, unsigned int value, uint8_t min, uint8_t max, uint8_t &adress);
  void HaltRequest();
};


#endif

