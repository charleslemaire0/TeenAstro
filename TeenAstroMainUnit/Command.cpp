/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2024 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Revision History, see GitHub
 *
 * Description: Command dispatcher and reply helpers; commandState and rtk. See Command.h.
 */
#include "Command.h"

// -----------------------------------------------------------------------------
// Command/serial state and real-time clock (single definitions)
// -----------------------------------------------------------------------------
CommandState::CommandState() : highPrecision(true)
{
  reply[0] = '\0';
  command[0] = '\0';
  baudRate_[0] = 115200;
  baudRate_[1] = 56700;
  baudRate_[2] = 38400;
  baudRate_[3] = 28800;
  baudRate_[4] = 19200;
  baudRate_[5] = 14400;
  baudRate_[6] = 9600;
  baudRate_[7] = 4800;
  baudRate_[8] = 2400;
  baudRate_[9] = 1200;
}

CommandState commandState;

DateTimeTimers rtk;

// -----------------------------------------------------------------------------
// Command dispatcher
// -----------------------------------------------------------------------------
void processCommands() {
  Command process_command = COMMAND_NONE;

  commandState.S_USB_.update();
  commandState.S_SHC_.update();

  commandState.S_USB_.getCmdPar(commandState.command, process_command);
  commandState.S_SHC_.getCmdPar(commandState.command, process_command);

  if (process_command == COMMAND_NONE)
    return;

  clearReply();

  switch (commandState.command[0]) {
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

  if (strlen(commandState.reply) > 0) {
    commandState.S_USB_.reply(commandState.reply, process_command);
    commandState.S_SHC_.reply(commandState.reply, process_command);
  }
  if (mount.motorsEncoders.reboot_unit)
    reboot();
}

// -----------------------------------------------------------------------------
// Reply helpers (write into commandState.reply; dispatcher sends to active port)
// -----------------------------------------------------------------------------
void replyShortTrue() {
  strcpy(commandState.reply, "1");
}

void replyLongTrue() {
  strcpy(commandState.reply, "1#");
}

void replyShortFalse() {
  strcpy(commandState.reply, "0");
}

void replyLongFalse() {
  strcpy(commandState.reply, "0#");
}

void replyLongUnknow() {
  strcpy(commandState.reply, "#");
}

void replyValueSetShort(bool set) {
  set ? replyShortTrue() : replyShortFalse();
}

void replyNothing() {
  commandState.reply[0] = '\0';
}

void clearReply() {
  for (int i = 0; i < REPLY_BUFFER_LEN; i++)
    commandState.reply[i] = '\0';
}
