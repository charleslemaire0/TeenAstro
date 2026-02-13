/**
 * T_Serial implementation: command buffer and LX200-style frame parsing.
 */
#include "Command.h"

namespace {
  constexpr char FRAME_START = ':';
  constexpr char FRAME_END   = '#';
  constexpr char CHAR_ACK   = 6;
  constexpr char CHAR_SPACE = 32;
  constexpr char CHAR_LF    = 10;
  constexpr char CHAR_CR    = 13;
}

void T_Serial::clearCommand() {
  m_bufferPtr = 0;
  m_command[m_bufferPtr] = '\0';
}

void T_Serial::attach_Stream(Stream* serial, Command cmdSerial) {
  m_serial = serial;
  m_cmdSerial = cmdSerial;
}

Command T_Serial::getcmdSerial() {
  return m_cmdSerial;
}

void T_Serial::update() {
  if (m_serial->available() <= 0 || m_ready)
    return;

  char c = m_serial->read();

  if (c == CHAR_ACK && m_bufferPtr == 0) {
    m_command[0] = c;
    m_command[1] = '\0';
    m_ready = true;
    return;
  }

  if (c != CHAR_SPACE && c != CHAR_LF && c != CHAR_CR && c != CHAR_ACK) {
    m_command[m_bufferPtr] = c;
    m_bufferPtr++;
    m_command[m_bufferPtr] = '\0';
    if (m_bufferPtr > CMD_MAX_PAYLOAD)
      m_bufferPtr = CMD_MAX_PAYLOAD;
  }

  if (c == FRAME_END) {
    if (m_bufferPtr > 1 && m_command[0] == FRAME_START && m_command[m_bufferPtr - 1] == FRAME_END) {
      m_command[m_bufferPtr - 1] = '\0';
      memmove(m_command, &m_command[1], strlen(&m_command[1]) + 1);
      m_ready = true;
    } else {
      clearCommand();
      m_ready = false;
    }
  } else {
    m_ready = false;
  }
}

void T_Serial::getCmdPar(char* cmd, Command& pcmd) {
  if (!m_ready || pcmd != COMMAND_NONE)
    return;
  strcpy(cmd, m_command);
  m_ready = false;
  clearCommand();
  pcmd = m_cmdSerial;
}

void T_Serial::reply(char* reply, const Command pcmd) {
  if (m_cmdSerial == pcmd)
    m_serial->print(reply);
}
