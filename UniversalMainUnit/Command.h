#pragma once
enum Command { COMMAND_NONE, COMMAND_SERIAL, COMMAND_SERIAL1 };
class T_Serial
{
public:
  Stream* m_serial = NULL;
private:
  Command m_cmdSerial = COMMAND_NONE;
  bool m_ready = false;
  char m_command[28] = "";
  byte m_bufferPtr = 0;
  bool clearCommand()
  {
    m_bufferPtr = 0;
    m_command[m_bufferPtr] = (char)0;
    return true;
  }
public:
  void attach_Stream(Stream* serial, Command cmdSerial)
  {
    m_serial = serial;
    m_cmdSerial = cmdSerial;
    return;
  }
  Command getcmdSerial()
  {
    return m_cmdSerial;
  }
  void update()
  {
    if (m_serial->available() > 0 && !m_ready)
    {
      // (chr)6 is a special status command for the LX200 protocol
      char c = m_serial->read();
      //!!!!TODO!!!!!
      //if ((c == (char)6) && (m_bufferPtr == 0))
      //{
      //  mountType == isAltAZ() ? m_serial->print("A") : m_serial->print("P");
      //}

      // ignore spaces/lf/cr, dropping spaces is another tweek to allow compatibility with LX200 protocol
      if ((c != (char)32) && (c != (char)10) && (c != (char)13) && (c != (char)6))
      {
        m_command[m_bufferPtr] = c;
        m_bufferPtr++;
        m_command[m_bufferPtr] = (char)0;
        if (m_bufferPtr > 22)
        {
          m_bufferPtr = 22;
        }   // limit maximum command length to avoid overflow, c2+p16+cc2+eol2+eos1=23 bytes max ranging from 0..22
      }

      if (c == '#')
      {
        // validate the command frame, normal command
        if ((m_bufferPtr > 1) && (m_command[0] == ':') &&
          (m_command[m_bufferPtr - 1] == '#'))
        {
          m_command[m_bufferPtr - 1] = 0;
        }
        else
        {
          clearCommand();
          m_ready = false;
        }
        memmove(m_command, (char *)&m_command[1], strlen(&m_command[1]));
        m_command[strlen(m_command) - 1] = 0;
        m_ready = true;
      }
      else
      {
        m_ready = false;
      }
    }
  }
  void getCmdPar(char* cmd, Command& pcmd)
  {
    if (!m_ready || pcmd != COMMAND_NONE)
      return;
    strcpy(cmd, m_command);
    m_ready = false;
    clearCommand();
    pcmd = m_cmdSerial;
  }
  void reply(char* reply, const Command pcmd)
  {
    if (m_cmdSerial == pcmd)
    {
      m_serial->print(reply);
    }
  }
};


void processCommandsTask(void *);
void processCommands(void);
void Command_GNSS(void);
void Command_dollar(void);
void Command_A(void);
void Command_B(void);
void Command_C(void);
void Command_D(void);
void Command_F(void);
void Command_h(void);
void Command_M(void);
void Command_Q(void);
void Command_R(void);
void Command_T(void);
void Command_U(void);
void Command_W(void);
void Command_G(void);
void Command_GX(void);
void Command_S(Command& process_command);
bool iSGNSSValid(void);
