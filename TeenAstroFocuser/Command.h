// Command.h

#ifndef _COMMAND_h
#define _COMMAND_h


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
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
#define FocCmd_goto 'g'
#define FocCmd_Park 'P'
#define FocCmd_Sync 'S'
#define FocCmd_set 's'
#define FocCmd_Halt 'Q'

#define FocCmd_Reset '$'
#define FocCmd_Reboot '!'

#define FocCmd_Write 'W'

#define CmdDumpState '?'
#define CmdDumpConfig '~'
#define CmdDumpConfigPos 'x'
#define CmdDumpConfigMotor 'M'

#define FocCmd_startPosition '0'
#define FocCmd_maxPosition '1'
#define FocCmd_lowSpeed '2'
#define FocCmd_highSpeed '3'
#define FocCmd_cmdAcc '4'
#define FocCmd_manualAcc '5'
#define FocCmd_manualDec '6' 
#define FocCmd_Inv '7'
#define FocCmd_inpulse '8'
#define FocCmd_current 'c'
#define FocCmd_micro 'm'
#define FocCmd_steprot 'r'


#define Char_CR 13
#define Char_Spc 32
#define Char_254 254



class SerCom
{
public:
  SerCom(Stream& s) :ser(s)
  {}

private:
  Stream& ser;
  char m_command;
  char m_parameter;
  char m_input[30];
  unsigned int m_value = 0;
  bool m_valuedefined = false;
  bool m_hasReceivedCommand = false;
  bool m_hasreceivedstart = false;
  unsigned int message_pos = 0;
  uint32_t m_lastupdate = millis();
public:
  bool Do();
  bool Get_Command();
  bool SetRequest(void);
  bool MoveRequest(void);
  bool GetRequest(void);
  bool HaltRequest(void);
private:
  void dumpState();
  void dumpParameterPosition(ParameterPosition* Pos);
  void printvalue(double val, int n, int d, bool plus);
  void dumpConfig();
  void dumpConfigMotor();
  void sayHello();

  bool setvalue(bool valuedefined, unsigned int value, Parameteruint *adress);
  bool setvalue(bool valuedefined, unsigned long value, unsigned long min, unsigned long max, unsigned long &adress);
  bool setvalue(bool valuedefined, unsigned long value, Parameterulong *adress);
  bool setvalue(bool valuedefined, unsigned int value, Parameteruint8_t *adress);
};


#endif

