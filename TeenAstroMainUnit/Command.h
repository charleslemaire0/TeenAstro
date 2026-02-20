#pragma once
/**
 * TeenAstro serial command interface (LX200-style protocol).
 * - T_Serial: per-port command buffer and reply handling
 * - processCommands(): main dispatcher (called from loop)
 * - Command_*(): handlers for each command letter
 */
#include "CommandGlobals.h"
#include "CommandConstants.h"

// -----------------------------------------------------------------------------
// Serial channel identification
// -----------------------------------------------------------------------------
enum Command {
  COMMAND_NONE,
  COMMAND_SERIAL,
  COMMAND_SERIAL1
};

// -----------------------------------------------------------------------------
// Serial command buffer (one per port: USB, SHC)
// -----------------------------------------------------------------------------
class T_Serial {
public:
  Stream* m_serial = nullptr;

  void attach_Stream(Stream* serial, Command cmdSerial);
  Command getcmdSerial();
  void update();
  void getCmdPar(char* cmd, Command& pcmd);
  void reply(char* reply, const Command pcmd);

private:
  Command m_cmdSerial = Command::COMMAND_NONE;
  bool m_ready = false;
  char m_command[CMD_BUFFER_LEN] = "";
  byte m_bufferPtr = 0;

  void clearCommand();
};

// -----------------------------------------------------------------------------
// Command/serial state: current command/reply buffers and precision (no globals)
// -----------------------------------------------------------------------------
struct CommandState {
  CommandState();
  unsigned long baudRate_[10];
  T_Serial S_SHC_;
  T_Serial S_USB_;
  char reply[REPLY_BUFFER_LEN];
  char command[CMD_BUFFER_LEN];
  bool highPrecision;
};
extern CommandState commandState;

// -----------------------------------------------------------------------------
// Command dispatcher
// -----------------------------------------------------------------------------
void processCommands();

// -----------------------------------------------------------------------------
// Command handlers (by lead character)
// -----------------------------------------------------------------------------
void Command_dollar();   // $
void Command_ACK();      // <ACK>
void Command_A();       // A  Alignment
void Command_B();       // B  Reticule
void Command_C();       // C  Sync
void Command_D();       // D  Distance bars
void Command_E();       // E  Encoder / push-to
void Command_F();       // F  Focuser
void Command_G();       // G  Get (LX200)
void Command_GX();      // GX TeenAstro get
void Command_GNSS();    // g  GNSS
void Command_h();      // h  Home / park
void Command_M();      // M  Move / slew
void Command_Q();      // Q  Halt
void Command_R();      // R  Slew rate
void Command_S(Command& process_command);  // S  Set (LX200)
void Command_SX();     // SX TeenAstro set
void Command_T();      // T  Tracking
void Command_U();      // U  Precision
void Command_W();      // W  Site

bool iSGNSSValid();
