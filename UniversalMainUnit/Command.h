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
void Command_dollar(void);
void Command_ACK(void);
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
void replyShortTrue();
void replyLongTrue();
void replyShortFalse();
void replyLongFalse();
void replyLongUnknown();
void replyNothing(void);
void clearReply(void);
void replyValueSetShort(bool set);


namespace Cmd {
  constexpr char RESET       = '$';   // :$x#  Reset / reboot / init
  constexpr char ACK         = 6;     // <ACK> Mount type (LX200)
  constexpr char ALIGNMENT   = 'A';   // :Ax#  Alignment
  constexpr char RETICULE    = 'B';   // :B+#  Reticule brightness
  constexpr char SYNC        = 'C';   // :Cx#  Sync
  constexpr char DISTANCE    = 'D';   // :D#   Distance bars
  constexpr char ENCODER     = 'E';   // :Ex#  Encoder / push-to
  constexpr char FOCUSER     = 'F';   // :Fx#  Focuser
  constexpr char GET         = 'G';   // :Gx#  Get (LX200 + :GXnn# TeenAstro)
  constexpr char GNSS        = 'g';   // :gx#  GNSS sync
  constexpr char HOME_PARK   = 'h';   // :hx#  Home / park
  constexpr char MOVE        = 'M';   // :Mx#  Move / slew / goto
  constexpr char HALT        = 'Q';   // :Qx#  Halt
  constexpr char RATE        = 'R';   // :Rx#  Slew rate
  constexpr char SET         = 'S';   // :Sx#  Set (LX200 + :SXnnn# TeenAstro)
  constexpr char TRACKING    = 'T';   // :Tx#  Tracking
  constexpr char PRECISION   = 'U';   // :U#   Precision toggle
  constexpr char SITE        = 'W';   // :Wn#  Site select
}

/// Pad CMDR_LONG reply to expected length (leading blanks). Call after command handler runs.
void padReplyToExpectedLength();

inline CMDREPLY getReplyType(const char* command)
{
  // ACK (0x06) is a special single-byte command
  if ((command[0] == Cmd::ACK) && (command[1] == 0))
    return CMDR_SHORT;

  if (command[0] != ':')
    return CMDR_INVALID;

  switch (command[1])
  {
  // ---- A  Alignment ---------------------------------------------------
  case Cmd::ALIGNMENT:
    if (strchr("*0123456789CWA", command[2])) return CMDR_SHORT_BOOL;
    if (strchr("E", command[2]))              return CMDR_LONG;
    return CMDR_INVALID;

  // ---- B  Reticule brightness ----------------------------------------
  case Cmd::RETICULE:
    if (strchr("+-", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- C  Sync -------------------------------------------------------
  case Cmd::SYNC:
    if (strchr("AMU", command[2])) return CMDR_LONG;
    if (strchr("S", command[2]))   return CMDR_NO;
    return CMDR_INVALID;

  // ---- D  Distance bars ----------------------------------------------
  case Cmd::DISTANCE:
    if (strchr("#", command[2])) return CMDR_LONG;
    return CMDR_INVALID;

  // ---- E  Encoder / Push-to ------------------------------------------
  case Cmd::ENCODER:
    if (strchr("ACD", command[2])) return CMDR_LONG;
    if (command[2] == 'M' && strchr("ASUQ", command[3])) return CMDR_SHORT;
    if (command[2] == 'W' && (command[3] == '1' || command[3] == '0')) return CMDR_SHORT_BOOL;  // EW1#/EW0# emu WiFi
    return CMDR_INVALID;

  // ---- F  Focuser -----------------------------------------------------
  case Cmd::FOCUSER:
    if (strchr("+-gGPQsS$!", command[2]))                    return CMDR_NO;
    if (strchr("OoIi:012345678cCmrW", command[2]))           return CMDR_SHORT_BOOL;
    if (strchr("x?~MVAa", command[2]))                       return CMDR_LONG;
    return CMDR_INVALID;

  // ---- g  GNSS --------------------------------------------------------
  case Cmd::GNSS:
    if (strchr("ts", command[2])) return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- G  Get ---------------------------------------------------------
  case Cmd::GET:
    if (strchr("AaCcDdefgGhLMNOPmnoRrSTtVXWZ", command[2]))
      return CMDR_LONG;
    return CMDR_INVALID;

  // ---- h  Home / Park -------------------------------------------------
  case Cmd::HOME_PARK:
    if (strchr("BbCFOPQRS", command[2])) return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- M  Move / Slew -------------------------------------------------
  case Cmd::MOVE:
    if (strchr("ewnsg", command[2]))    return CMDR_NO;
    if (strchr("SAUF?", command[2]))    return CMDR_SHORT;
    if (strchr("12@", command[2]))      return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- Q  Halt --------------------------------------------------------
  case Cmd::HALT:
    if (strchr("#ewns", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- R  Rate --------------------------------------------------------
  case Cmd::RATE:
    if (strchr("GCMS01234", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- S  Set ---------------------------------------------------------
  case Cmd::SET:
    if (strchr("!aBCedgGhLmMnNoOrtTUXz", command[2])) return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- T  Tracking ----------------------------------------------------
  case Cmd::TRACKING:
    if (strchr("R+-TSLQ", command[2]))   return CMDR_NO;
    if (strchr("ed012", command[2]))     return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  // ---- U  Precision ---------------------------------------------------
  case Cmd::PRECISION:
    if (strchr("#", command[2])) return CMDR_NO;
    return CMDR_INVALID;

  // ---- W  Site --------------------------------------------------------
  case Cmd::SITE:
    if (strchr("0123", command[2])) return CMDR_NO;
    if (strchr("?", command[2]))    return CMDR_LONG;
    return CMDR_INVALID;

  // ---- $  Reset -------------------------------------------------------
  case Cmd::RESET:
    if (strchr("$!", command[2]))  return CMDR_NO;
    if (strchr("X", command[2]))   return CMDR_SHORT_BOOL;
    return CMDR_INVALID;

  default:
    return CMDR_INVALID;
  }
}

/// Expected reply payload length (chars before '#'). Returns -1 for variable length, 0 for no reply, 1 for short.
inline int getExpectedReplyLength(const char* command)
{
  if (!command || command[0] != ':') return -1;
  CMDREPLY r = getReplyType(command);
  if (r == CMDR_NO) return 0;
  if (r == CMDR_SHORT || r == CMDR_SHORT_BOOL) return 1;
  if (r != CMDR_LONG) return -1;
  // Fixed-length CMDR_LONG commands:

  if (strcmp(command, ":AE#") == 0) return 16;
  if (strcmp(command, ":CA#") == 0) return 4;
  if (strcmp(command, ":CM#") == 0) return 4;
  if (strcmp(command, ":CS#") == 0) return 4;
  if (strcmp(command, ":CU#") == 0) return 4;
  if (strcmp(command, ":D#") == 0) return 4;
  if (strcmp(command, ":ED#") == 0) return 16;
  if (strcmp(command, ":F?#") == 0) return 32;
  if (strcmp(command, ":FA#") == 0) return 256;
  if (strcmp(command, ":FV#") == 0) return 32;
  if (strcmp(command, ":Fa#") == 0) return 16;
  if (strcmp(command, ":F~#") == 0) return 128;
  if (strcmp(command, ":G(#") == 0) return 16;
  if (strcmp(command, ":G)#") == 0) return 16;
  if (strcmp(command, ":GA#") == 0) return 16;
  if (strcmp(command, ":GC#") == 0) return 8;
  if (strcmp(command, ":GD#") == 0) return 16;
  if (strcmp(command, ":GDL#") == 0) return 16;
  if (strcmp(command, ":GG#") == 0) return 8;
  if (strcmp(command, ":GL#") == 0) return 8;
  if (strcmp(command, ":GM#") == 0) return 16;
  if (strcmp(command, ":GN#") == 0) return 16;
  if (strcmp(command, ":GO#") == 0) return 16;
  if (strcmp(command, ":GR#") == 0) return 8;
  if (strcmp(command, ":GRL#") == 0) return 16;
  if (strcmp(command, ":GS#") == 0) return 8;
  if (strcmp(command, ":GSL#") == 0) return 16;
  if (strcmp(command, ":GT#") == 0) return 16;
  if (strcmp(command, ":GVB#") == 0) return 4;
  if (strcmp(command, ":GVD#") == 0) return 16;
  if (strcmp(command, ":GVN#") == 0) return 16;
  if (strcmp(command, ":GVP#") == 0) return 32;
  if (strcmp(command, ":GVT#") == 0) return 16;
  if (strcmp(command, ":GVb#") == 0) return 4;
  if (strcmp(command, ":GW#") == 0) return 4;
  if (strcmp(command, ":GXA0#") == 0) return 16;
  if (strcmp(command, ":GXA1#") == 0) return 16;
  if (strcmp(command, ":GXA2#") == 0) return 16;
  if (strcmp(command, ":GXA3#") == 0) return 16;
  if (strcmp(command, ":GXA4#") == 0) return 16;
  if (strcmp(command, ":GXA5#") == 0) return 16;
  if (strcmp(command, ":GXA6#") == 0) return 16;
  if (strcmp(command, ":GXA7#") == 0) return 16;
  if (strcmp(command, ":GXA8#") == 0) return 16;
  if (strcmp(command, ":GXAS#") == 0) return 136;
  if (strcmp(command, ":GXAa#") == 0) return 16;
  if (strcmp(command, ":GXAw#") == 0) return 16;
  if (strcmp(command, ":GXAz#") == 0) return 16;
  if (strcmp(command, ":GXCS#") == 0) return 128;
  if (strcmp(command, ":GXDP0#") == 0) return 16;
  if (strcmp(command, ":GXDP1#") == 0) return 16;
  if (strcmp(command, ":GXDP2#") == 0) return 16;
  if (strcmp(command, ":GXDP3#") == 0) return 16;
  if (strcmp(command, ":GXDP4#") == 0) return 16;
  if (strcmp(command, ":GXDP5#") == 0) return 16;
  if (strcmp(command, ":GXDP6#") == 0) return 16;
  if (strcmp(command, ":GXDP7#") == 0) return 16;
  if (strcmp(command, ":GXDR1#") == 0) return 16;
  if (strcmp(command, ":GXDR2#") == 0) return 16;
  if (strcmp(command, ":GXDR3#") == 0) return 16;
  if (strcmp(command, ":GXDR4#") == 0) return 16;
  if (strcmp(command, ":GXDR5#") == 0) return 16;
  if (strcmp(command, ":GXDR6#") == 0) return 16;
  if (strcmp(command, ":GXDW#") == 0) return 8;
  if (strcmp(command, ":GXDW1#") == 0) return 16;
  if (strcmp(command, ":GXE0#") == 0) return 16;
  if (strcmp(command, ":GXE1#") == 0) return 16;
  if (strcmp(command, ":GXE2#") == 0) return 16;
  if (strcmp(command, ":GXEA#") == 0) return 16;
  if (strcmp(command, ":GXED#") == 0) return 16;
  if (strcmp(command, ":GXEO#") == 0) return 4;
  if (strcmp(command, ":GXEPD#") == 0) return 16;
  if (strcmp(command, ":GXEPR#") == 0) return 16;
  if (strcmp(command, ":GXER#") == 0) return 8;
  if (strcmp(command, ":GXEZ#") == 0) return 16;
  if (strcmp(command, ":GXErD#") == 0) return 16;
  if (strcmp(command, ":GXErR#") == 0) return 16;
  if (strcmp(command, ":GXLE#") == 0) return 16;
  if (strcmp(command, ":GXLH#") == 0) return 8;
  if (strcmp(command, ":GXLO#") == 0) return 8;
  if (strcmp(command, ":GXLS#") == 0) return 8;
  if (strcmp(command, ":GXLU#") == 0) return 16;
  if (strcmp(command, ":GXLW#") == 0) return 16;
  if (strcmp(command, ":GXMBD#") == 0) return 8;
  if (strcmp(command, ":GXMBR#") == 0) return 8;
  if (strcmp(command, ":GXMCD#") == 0) return 8;
  if (strcmp(command, ":GXMCR#") == 0) return 8;
  if (strcmp(command, ":GXMGD#") == 0) return 16;
  if (strcmp(command, ":GXMGR#") == 0) return 16;
  if (strcmp(command, ":GXMID#") == 0) return 8;
  if (strcmp(command, ":GXMIR#") == 0) return 8;
  if (strcmp(command, ":GXMLD#") == 0) return 8;
  if (strcmp(command, ":GXMLR#") == 0) return 8;
  if (strcmp(command, ":GXMMD#") == 0) return 8;
  if (strcmp(command, ":GXMMR#") == 0) return 8;
  if (strcmp(command, ":GXMRD#") == 0) return 4;
  if (strcmp(command, ":GXMRR#") == 0) return 4;
  if (strcmp(command, ":GXMSD#") == 0) return 8;
  if (strcmp(command, ":GXMSR#") == 0) return 8;
  if (strcmp(command, ":GXMcD#") == 0) return 8;
  if (strcmp(command, ":GXMcR#") == 0) return 8;
  if (strcmp(command, ":GXMmD#") == 0) return 8;
  if (strcmp(command, ":GXMmR#") == 0) return 8;
  if (strcmp(command, ":GXOA#") == 0) return 16;
  if (strcmp(command, ":GXOB#") == 0) return 16;
  if (strcmp(command, ":GXOC#") == 0) return 16;
  if (strcmp(command, ":GXOI#") == 0) return 8;
  if (strcmp(command, ":GXOS#") == 0) return 8;
  if (strcmp(command, ":GXP1#") == 0) return 16;
  if (strcmp(command, ":GXP2#") == 0) return 16;
  if (strcmp(command, ":GXP3#") == 0) return 16;
  if (strcmp(command, ":GXP4#") == 0) return 16;
  if (strcmp(command, ":GXRA#") == 0) return 16;
  if (strcmp(command, ":GXRB#") == 0) return 16;
  if (strcmp(command, ":GXRD#") == 0) return 4;
  if (strcmp(command, ":GXRX#") == 0) return 8;
  if (strcmp(command, ":GXT0#") == 0) return 8;
  if (strcmp(command, ":GXT1#") == 0) return 8;
  if (strcmp(command, ":GXT2#") == 0) return 16;
  if (strcmp(command, ":GXT3#") == 0) return 8;
  if (strcmp(command, ":GXlX#") == 0) return 8;
  if (strcmp(command, ":GXrg#") == 0) return 4;
  if (strcmp(command, ":GXrp#") == 0) return 4;
  if (strcmp(command, ":GXrt#") == 0) return 4;
  if (strcmp(command, ":GZ#") == 0) return 16;
  if (strcmp(command, ":Ga#") == 0) return 8;
  if (strcmp(command, ":Gc#") == 0) return 4;
  if (strcmp(command, ":Gd#") == 0) return 16;
  if (strcmp(command, ":GdL#") == 0) return 16;
  if (strcmp(command, ":Ge#") == 0) return 8;
  if (strcmp(command, ":Gf#") == 0) return 16;
  if (strcmp(command, ":Gg#") == 0) return 16;
  if (strcmp(command, ":Gh#") == 0) return 8;
  if (strcmp(command, ":Gm#") == 0) return 4;
  if (strcmp(command, ":Gn#") == 0) return 16;
  if (strcmp(command, ":Go#") == 0) return 8;
  if (strcmp(command, ":Gr#") == 0) return 8;
  if (strcmp(command, ":GrL#") == 0) return 16;
  if (strcmp(command, ":Gt#") == 0) return 16;
  if (strcmp(command, ":W?#") == 0) return 4;
  return -1;
}


