/*
 * AlpacaFocuser.h - ASCOM Alpaca Focuser (IFocuserV3) for TeenAstro
 *
 * Copyright (C) 2026 TeenAstro by Charles Lemaire
 * GNU General Public License v3
 *
 * Served at `/api/v1/focuser/0/...` on the same port as the telescope.
 * Position and motion come from `:F?#`; Move is `:FG,nnnnn#`; Halt is `:FQ#`.
 */
#pragma once

#include <Arduino.h>
#include "AlpacaResponse.h"

class LX200Client;
class TeenAstroAlpaca;

class AlpacaFocuser
{
public:
  AlpacaFocuser() = default;

  void begin(LX200Client& client, TeenAstroAlpaca& parent);

  void dispatch(AlpacaWebServer& server, const AlpacaRequest& req);

  /// Refresh `:F?#` outside the HTTP handler. Handlers only read this cache.
  void prepareForRequest();

  /// True after a `:F?#` reply that is a real focuser status, not "0".
  bool present() const { return m_present; }

  static const char* deviceName() { return "TeenAstro Focuser"; }

private:
  bool requireConnected(AlpacaWebServer& s, const AlpacaRequest& r);
  bool moving() const;
  int  maxStep() const;

  void getConnected(AlpacaWebServer& s, const AlpacaRequest& r);
  void putConnected(AlpacaWebServer& s, const AlpacaRequest& r);
  void getName(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDescription(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDriverInfo(AlpacaWebServer& s, const AlpacaRequest& r);
  void getDriverVersion(AlpacaWebServer& s, const AlpacaRequest& r);
  void getInterfaceVersion(AlpacaWebServer& s, const AlpacaRequest& r);
  void getSupportedActions(AlpacaWebServer& s, const AlpacaRequest& r);
  void putAction(AlpacaWebServer& s, const AlpacaRequest& r);
  void putCommand(AlpacaWebServer& s, const AlpacaRequest& r);

  void getAbsolute(AlpacaWebServer& s, const AlpacaRequest& r);
  void getIsMoving(AlpacaWebServer& s, const AlpacaRequest& r);
  void getMaxIncrement(AlpacaWebServer& s, const AlpacaRequest& r);
  void getMaxStep(AlpacaWebServer& s, const AlpacaRequest& r);
  void getPosition(AlpacaWebServer& s, const AlpacaRequest& r);
  void getStepSize(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTempComp(AlpacaWebServer& s, const AlpacaRequest& r);
  void putTempComp(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTempCompAvailable(AlpacaWebServer& s, const AlpacaRequest& r);
  void getTemperature(AlpacaWebServer& s, const AlpacaRequest& r);
  void putHalt(AlpacaWebServer& s, const AlpacaRequest& r);
  void putMove(AlpacaWebServer& s, const AlpacaRequest& r);

  LX200Client*    m_client = nullptr;
  TeenAstroAlpaca* m_parent = nullptr;
  bool            m_connected = false;

  bool            m_present = false;
  bool            m_statusValid = false;
  int             m_position = 0;
  int             m_speed = 0;
  double          m_temperature = 0;
  unsigned long   m_statusMs = 0;

  bool            m_maxValid = false;
  int             m_maxStep = 65535;
  unsigned long   m_configMs = 0;

  bool            m_targetSet = false;
  int             m_target = 0;
};
