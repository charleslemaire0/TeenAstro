/**
 * Command dispatcher and reply helpers.
 * processCommands() reads from S_USB/S_SHC, dispatches by lead character, sends reply.
 */
#include "Command.h"

// -----------------------------------------------------------------------------
// Command dispatcher
// -----------------------------------------------------------------------------
void processCommands() {
  Command process_command = COMMAND_NONE;

  S_USB.update();
  S_SHC.update();

  S_USB.getCmdPar(command, process_command);
  S_SHC.getCmdPar(command, process_command);

  if (process_command == COMMAND_NONE)
    return;

  clearReply();

  switch (command[0]) {
    case Cmd::RESET:     Command_dollar(); break;
    case Cmd::ACK:       Command_ACK(); break;
    case Cmd::ALIGNMENT: Command_A(); break;
    case Cmd::RETICULE:  Command_B(); break;
    case Cmd::SYNC:      Command_C(); break;
    case Cmd::DISTANCE:  Command_D(); break;
    case Cmd::ENCODER:   Command_E(); break;
    case Cmd::FOCUSER:   Command_F(); break;
    case Cmd::GET:       Command_G(); break;
    case Cmd::GNSS:      Command_GNSS(); break;
    case Cmd::HOME_PARK: Command_h(); break;
    case Cmd::MOVE:      Command_M(); break;
    case Cmd::HALT:      Command_Q(); break;
    case Cmd::RATE:      Command_R(); break;
    case Cmd::SET:      Command_S(process_command); break;
    case Cmd::TRACKING:  Command_T(); break;
    case Cmd::PRECISION: Command_U(); break;
    case Cmd::SITE:      Command_W(); break;
    default:             replyNothing(); break;
  }

  if (strlen(reply) > 0) {
    S_USB.reply(reply, process_command);
    S_SHC.reply(reply, process_command);
  }
  if (reboot_unit)
    reboot();
}

// -----------------------------------------------------------------------------
// Reply helpers (write into global reply[]; dispatcher sends to active port)
// -----------------------------------------------------------------------------
void replyShortTrue() {
  strcpy(reply, "1");
}

void replyLongTrue() {
  strcpy(reply, "1#");
}

void replyShortFalse() {
  strcpy(reply, "0");
}

void replyLongFalse() {
  strcpy(reply, "0#");
}

void replyLongUnknow() {
  strcpy(reply, "#");
}

void replyValueSetShort(bool set) {
  set ? replyShortTrue() : replyShortFalse();
}

void replyNothing() {
  reply[0] = '\0';
}

void clearReply() {
  for (int i = 0; i < REPLY_BUFFER_LEN; i++)
    reply[i] = '\0';
}
