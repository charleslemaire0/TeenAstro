#pragma once

#include <Arduino.h>

#ifdef ARDUINO_D1_MINI32
#include <Wifi.h>
#include <WebServer.h>
#include <HTTPUpdate.h>
#endif


#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266HTTPUpdateServer.h>
#endif


#include <EEPROM.h>
#include <WiFiClient.h>
#include <TeenAstroMountStatus.h>

#define Product "TeenAstro Server"
#define ServerFirmwareDate          __DATE__
#define ServerFirmwareTime          __TIME__
#define ServerFirmwareVersionMajor  "1"
#define ServerFirmwareVersionMinor  "2"
#define ServerFirmwareVersionPatch  "4"


// -----------------------------------------------------------------------------------
// Macros
#ifndef LEGACY_TRANSMIT_ON
// macros to help with sending webpage data, chunked
#define sendHtmlStart() server.setContentLength(CONTENT_LENGTH_UNKNOWN); server.sendHeader("Cache-Control","no-cache"); server.send(200, "text/html", String());
#define sendHtml(x) server.sendContent(x); x=""
#define sendHtmlDone(x) server.sendContent("");
#else
// macros to help with sending webpage data, normal method
#define sendHtmlStart()
#define sendHtml(x)
#define sendHtmlDone(x) server.send(200, "text/html", x)
#endif


// -----------------------------------------------------------------------------------
// Constants

// The settings below are for initialization only, afterward they are stored and recalled from EEPROM and must
// be changed in the web interface OR with a reset (for initialization again) as described in the Config.h comments


#define EEPROM_start 0
#define EEPROM_WifiOn EEPROM_start + 4
#define EEPROM_WifiMode EEPROM_WifiOn + 1
#define EEPROM_WebTimeout EEPROM_WifiMode + 1
#define EEPROM_CmdTimeout EEPROM_WebTimeout + 1
#define EEPROM_WifiConnectMode EEPROM_CmdTimeout + 1
#define EEPROM_Contrast 20
#define EEPROM_T1 21
#define EEPROM_T2 22
#define EEPROM_BSPEED 23
#define EEPROM_DISPLAYSUBMODEL 24
#define EEPROM_start_wifi_sta 100
#define EEPROM_start_wifi_ap 400
#define EPPROM_password 50


class TeenAstroWifi
{
  enum Responding
  {
    R_NONE, R_ONE, R_BOOL, R_STRING
  };
  enum WifiMode
  {
    M_Station1, M_Station2, M_Station3, M_AcessPoint, OFF
  };
  enum WifiConnectMode
  {
    AutoClose, KeepOpened
  };
  static bool wifiOn;
  static int WebTimeout;
  static int CmdTimeout;
  static WifiConnectMode activeWifiConnectMode;

#define Default_Password "password"
  static char masterPassword[40];
  static WifiMode activeWifiMode;

  static bool stationDhcpEnabled[3];
  static char wifi_sta_ssid[3][40];
  static char wifi_sta_pwd[3][40];
  static IPAddress wifi_sta_ip[3];
  static IPAddress wifi_sta_gw[3];
  static IPAddress wifi_sta_sn[3];

  static char wifi_ap_ssid[40];
  static char wifi_ap_pwd[40];
  static byte wifi_ap_ch;
  static IPAddress wifi_ap_ip;
  static IPAddress wifi_ap_gw;
  static IPAddress wifi_ap_sn;

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
  static ESP8266WebServer server;
#endif

#ifdef ARDUINO_D1_MINI32
  static WebServer server;
#endif

  static WiFiServer cmdSvr;
  static WiFiClient cmdSvrClient;
  static const char* HighSpeedCommsStr(long baud);
  static void preparePage(String &data, int page);
  static void processConfigurationSiteGet();
  static void handleConfigurationSite();
  static void processConfigurationTelescopeGet();
  static void handleConfigurationTelescope();
  static void processConfigurationFocuserGet();
  static void handleConfigurationFocuser();
  static void handleRoot();
  static void handleSettings();

  static void settingsAjax();
  static void processSettingsGet();
  static void controlAjax();
  static void processControlGet();
  static void guideAjax();
  static void handleControl();
  static void handlePec();
  static void pecAjax();
  static void processPecGet();
  static void handleWifi();
  static void handleNotFound();
  static void initFromEEPROM();
  static void writeStation2EEPROM(const int& k);
  static void writeAccess2EEPROM();
  static void processWifiGet();

  static bool atoi2(char *a, int *i);
  static bool atof2(char *a, float *f);
  static int hexToInt(String s);

  // write int numbers into EEPROM at position i (2 bytes)
  static void EEPROM_writeInt(int i, int j);
  // read int numbers from EEPROM at position i (2 bytes)
  static int EEPROM_readInt(int i);
  // write 4 byte variable into EEPROM at position i (4 bytes)
  static void EEPROM_writeQuad(int i, byte *v);
  // read 4 byte variable from EEPROM at position i (4 bytes)
  static void EEPROM_readQuad(int i, byte *v);
  // write String into EEPROM at position i (40 bytes)
  static void EEPROM_writeString(int i, char l[]);
  // read String from EEPROM at position i (40 bytes)
  static void EEPROM_readString(int i, char l[]);
  // write 4 byte float into EEPROM at position i (4 bytes)
  static void EEPROM_writeFloat(int i, float f);
  // read 4 byte float from EEPROM at position i (4 bytes)
  static float EEPROM_readFloat(int i);
  // write 4 byte long into EEPROM at position i (4 bytes)
  static void EEPROM_writeLong(int i, long l);
  // read 4 byte long from EEPROM at position i (4 bytes)
  static long EEPROM_readLong(int i);
public:
  static bool isWifiOn();
  static bool isWifiRunning();
  static void turnWifiOn(bool turnOn);
  static void setup();
  static void update();
  static void getIP(uint8_t* ip);
  static int  getWifiMode();
  static const char* getPassword();
  static bool setWifiMode(int k);
  static void getStationName(int k, char* SSID);
};
