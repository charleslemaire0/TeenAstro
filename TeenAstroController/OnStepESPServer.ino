/*
 * Title       OnStepESPServer
 * by          Howard Dutton
 *
 * Copyright (C) 2017 Howard Dutton
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
 *
 *
 * Revision History, see GitHub
 *
 *
 * Author: Howard Dutton
 * http://www.stellarjourney.com
 * hjd1964@gmail.com
 *
 * Description
 *
 * ESP8266-01 OnStep control
 *
 */

#include <U8x8lib.h>
#include <U8g2lib.h>
#define Product "OnEsp"
#define Version "1.0a 09 28 17"


#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include "Config.h"
#include "Selection_catalog.h"

#include <ESP8266WebServer.h>
#include <ESP8266WiFiAP.h>
#include <EEPROM.h>
#include <OneButton.h>




U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0);

OneButton buttons[7] = { OneButton(D8, false),
                        OneButton(D7, false), OneButton(D6, false),OneButton(D0, false),OneButton(D5, false),
                        OneButton(D3, true), OneButton(D4, true) };

bool Move[4] = { false,false,false,false };

char* BreakRC[4] = { ":Qn#" ,":Qs#" ,":Qe#" ,":Qw#" };
char* RC[4] = { ":Mn#" , ":Ms#" ,":Me#" ,":Mw#" };

enum Button { B_SHIFT, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_F, B_f };
enum ButtonEvent { E_NONE, E_CLICK, E_DOUBLECLICK, E_LONGPRESSTART, E_LONGPRESS, E_LONGPRESSSTOP };
byte eventbuttons[7] = { E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE };

enum Menu { M_NONE, M_ALL, M_REF, M_GOTO, M_DISPlAY, M_SPIRAL, M_SPEED, M_MOT1, M_MOT2, M_DIVERSE };
enum Errors { ERR_NONE, ERR_MOTOR_FAULT, ERR_ALT, ERR_LIMIT_SENSE, ERR_DEC, ERR_AZM, ERR_UNDER_POLE, ERR_MERIDIAN, ERR_SYNC };
Errors lastError = ERR_NONE;
enum AlignMode { ALIM_ONE, ALIM_TWO, ALIM_THREE };
enum AlignState {
  ALI_OFF,
  ALI_SELECT_STAR_1, ALI_SLEW_STAR_1, ALI_RECENTER_1,
  ALI_SELECT_STAR_2, ALI_SLEW_STAR_2, ALI_RECENTER_2,
  ALI_SELECT_STAR_3, ALI_SLEW_STAR_3, ALI_RECENTER_3
};

enum TrackState { TRK_OFF, TRK_ON, TRK_SLEWING, TRK_UNKNOW };
enum ParkState { PRK_UNPARKED, PRK_PARKED, PRK_FAILED, PRK_PARKING, PRK_UNKNOW };
enum PierState { PIER_E, PIER_W, PIER_UNKNOW };
enum Mount { GEM, FEM };
Mount mountType = GEM;
uint8_t align = ALI_OFF;
uint8_t aliMode = 0;
unsigned short alignSelectedStar = 1;
bool buttonCommand = false;
class TelState
{
public:
  char TempRa[20];
  char TempDec[20];
  unsigned long lastStateRaDec;
  char TempAz[20];
  char TempAlt[20];
  unsigned long lastStateAzAlt;
  char TempUTC[20];
  char TempSideral[20];
  unsigned long lastStateTime;
  char TelStatus[20];
  char sideofpier[20];
  unsigned long lastStateTel;
  bool connected = true;
  bool hasInfoRa = false;
  bool hasInfoDec = false;
  bool hasInfoAz = false;
  bool hasInfoAlt = false;
  bool hasInfoUTC = false;
  bool hasInfoSideral = false;
  bool hasPierInfo = false;
  bool hasTelStatus = false;
  unsigned long lastState;
  void updateRaDec()
  {
    if (millis() - lastStateRaDec > 100 && connected)
    {
      hasInfoRa = GetLX200(":GR#", TempRa, true);
      hasInfoDec = GetLX200(":GD#", TempDec, true);
      lastStateRaDec = millis();
      if (!hasInfoRa && !hasInfoDec)
      {
        connected = true;
      }
    }
  };
  void updateAzAlt()
  {
    if (millis() - lastStateAzAlt > 100 && connected)
    {
      hasInfoAz = GetLX200(":GZ#", TempAz, true);
      hasInfoAlt = GetLX200(":GA#", TempAlt, true);
      lastStateAzAlt = millis();
      if (!hasInfoAz && !hasInfoAlt)
      {
        connected = true;
      }
    }
  }
  void updateTime()
  {
    if (millis() - lastStateTime > 100 && connected)
    {
      hasInfoUTC = GetLX200(":GL#", TempUTC, true);
      hasInfoSideral = GetLX200(":GS#", TempSideral, true);
      lastStateTime = millis();
      if (!hasInfoUTC && !hasInfoSideral)
      {
        connected = true;
      }
    }

  };
  void updateTel()
  {
    if (millis() - lastStateTel > 100 && connected)
    {
      hasPierInfo = GetLX200(":Gm#", sideofpier, true);
      hasTelStatus = GetLX200(":GU#", TelStatus, true);
      lastStateTel = millis();
    }
    if (!hasPierInfo && !hasTelStatus)
    {
      connected = true;
    }
  };
  ParkState getParkState()
  {
    if (strchr(&TelStatus[0], 'P') != NULL)
    {
      return PRK_PARKED;
    }
    else if (strchr(&TelStatus[0], 'p') != NULL)
    {
      return PRK_UNPARKED;
    }
    else if (strchr(&TelStatus[0], 'I') != NULL)
    {
      return PRK_PARKING;
    }
    else if (strchr(&TelStatus[0], 'F') != NULL)
    {
      return PRK_FAILED;
    }
    return PRK_UNKNOW;
  }
  TrackState getTrackingState()
  {
    if (strchr(&TelStatus[0], 'N') == NULL)
    {
      return TRK_SLEWING;
    }
    else if (strchr(&TelStatus[0], 'n') == NULL)
    {
      return TRK_ON;
    }
    if (strchr(&TelStatus[0], 'n') != NULL && strchr(&TelStatus[0], 'N') != NULL)
    {
      return TRK_OFF;
    }
    return TRK_UNKNOW;
  }
  bool atHome()
  {
    return strchr(&TelStatus[0], 'H') != NULL;
  }
  bool isGuiding()
  {
    return  strchr(&TelStatus[0], 'G') != NULL;
  }
  PierState getPierState()
  {
    if (strchr(&sideofpier[0], 'E') != NULL)
    {
      return PIER_E;
    }
    else if (strchr(&sideofpier[0], 'W') != NULL)
    {
      return PIER_W;
    }
    return PIER_UNKNOW;
  }
  Errors getError()
  {
    if (strlen(TelStatus) > 2)
    {
      int l = strlen(TelStatus) - 2;
      switch (TelStatus[l])
      {
      case '1':
        return ERR_MOTOR_FAULT;
      case '2':
        return ERR_ALT;
      case '4':
        return ERR_DEC;
      case '6':
        return ERR_UNDER_POLE;
      case '7':
        return ERR_MERIDIAN;
      default:
        return ERR_NONE;
      }
    }
  }
}telInfo;

#define PierSideNone     0
#define PierSideEast     1
#define PierSideWest     2
#define PierSideBest     3
#define PierSideFlipWE1  10
#define PierSideFlipWE2  11
#define PierSideFlipWE3  12
#define PierSideFlipEW1  20
#define PierSideFlipEW2  21
#define PierSideFlipEW3  22
byte pierSide = PierSideNone;
int AlignMaxNumStars = -1;

byte button = 0;
unsigned long lastpageupdate = millis();
unsigned long time_last_action = millis();
uint8_t maxContrast = 255;
bool wifiOn = true;
bool powerCylceRequired = false;
bool sleepDisplay = false;
bool lowContrast = false;
bool buttonPressed = false;
byte page = 0;


#define Default_Password "password"
char masterPassword[40] = Default_Password;

bool accessPointEnabled = true;
bool stationEnabled = false;
bool stationDhcpEnabled = true;

char wifi_sta_ssid[40] = "";
char wifi_sta_pwd[40] = "";

IPAddress wifi_sta_ip = IPAddress(192, 168, 0, 1);
IPAddress wifi_sta_gw = IPAddress(192, 168, 0, 1);
IPAddress wifi_sta_sn = IPAddress(255, 255, 255, 0);

char wifi_ap_ssid[40] = "";

char wifi_ap_pwd[40] = "password";
byte wifi_ap_ch = 7;

IPAddress wifi_ap_ip = IPAddress(192, 168, 0, 1);
IPAddress wifi_ap_gw = IPAddress(192, 168, 0, 1);
IPAddress wifi_ap_sn = IPAddress(255, 255, 255, 0);

// base timeouts
int WebTimeout = TIMEOUT_WEB;  // default 15
int CmdTimeout = TIMEOUT_CMD;  // default 30

ESP8266WebServer server(80);
WiFiServer cmdSvr(9999);
WiFiClient cmdSvrClient;
unsigned long clientTime = 0;

char writeBuffer[40] = "";
int writeBufferPos = 0;

const char* html_head1 = "<!DOCTYPE HTML>\r\n<html>\r\n<head>\r\n";
const char* html_headerPec = "<meta http-equiv=\"refresh\" content=\"5; URL=/pec.htm\">\r\n";
const char* html_headerIdx = "<meta http-equiv=\"refresh\" content=\"5; URL=/index.htm\">\r\n";
const char* html_head2 = "</head>\r\n<body bgcolor=\"#26262A\">\r\n";

const char* html_main_css1 = "<STYLE>";
const char* html_main_css2 = ".a { background-color: #111111; } .t { padding: 10px 10px 20px 10px; border: 5px solid #551111;";
const char* html_main_css3 = " margin: 25px 25px 0px 25px; color: #999999; background-color: #111111; } input { width:4em; font-weight: bold; background-color: #A01010; padding: 2px 2px; }";
const char* html_main_css4 = ".b { padding: 10px; border-left: 5px solid #551111; border-right: 5px solid #551111; border-bottom: 5px solid #551111; margin: 0px 25px 25px 25px; color: #999999;";
const char* html_main_css5 = "background-color: #111111; } select { width:4em; font-weight: bold; background-color: #A01010; padding: 2px 2px; } .c { color: #A01010; font-weight: bold; }";
const char* html_main_css6 = "h1 { text-align: right; } a:hover, a:active { background-color: red; } .g { color: #105010; font-weight: bold; }";
const char* html_main_css7 = "a:link, a:visited { background-color: #332222; color: #a07070; border:1px solid red; padding: 5px 10px;";
const char* html_main_css8 = " margin: none; text-align: center; text-decoration: none; display: inline-block; }";
const char* html_main_css9 = "button { background-color: #A01010; font-weight: bold; border-radius: 5px; font-size: 12px; margin: 2px; padding: 4px 8px; }</STYLE>";

const char* html_links1in = "<a href=\"/index.htm\" style=\"background-color: #552222;\">Status</a><a href=\"/control.htm\">Control</a>";
const char* html_links2in = "<a href=\"/guide.htm\">Guide</a><a href=\"/pec.htm\">PEC</a><a href=\"/settings.htm\">Settings</a>";
const char* html_links3in = "<a href=\"/wifi.htm\">WiFi</a><a href=\"/config.htm\">Config.h</a><br />";

const char* html_links1ct = "<a href=\"/index.htm\">Status</a><a href=\"/control.htm\" style=\"background-color: #552222;\">Control</a>";
const char* html_links2ct = "<a href=\"/guide.htm\">Guide</a><a href=\"/pec.htm\">PEC</a><a href=\"/settings.htm\">Settings</a>";
const char* html_links3ct = "<a href=\"/wifi.htm\">WiFi</a><a href=\"/config.htm\">Config.h</a><br />";

const char* html_links1gu = "<a href=\"/index.htm\">Status</a><a href=\"/control.htm\">Control</a>";
const char* html_links2gu = "<a href=\"/guide.htm\" style=\"background-color: #552222;\">Guide</a><a href=\"/pec.htm\">PEC</a><a href=\"/settings.htm\">Settings</a>";
const char* html_links3gu = "<a href=\"/wifi.htm\">WiFi</a><a href=\"/config.htm\">Config.h</a><br />";

const char* html_links1pe = "<a href=\"/index.htm\">Status</a><a href=\"/control.htm\">Control</a>";
const char* html_links2pe = "<a href=\"/guide.htm\">Guide</a><a href=\"/pec.htm\" style=\"background-color: #552222;\">PEC</a><a href=\"/settings.htm\">Settings</a>";
const char* html_links3pe = "<a href=\"/wifi.htm\">WiFi</a><a href=\"/config.htm\">Config.h</a><br />";

const char* html_links1se = "<a href=\"/index.htm\">Status</a><a href=\"/control.htm\">Control</a>";
const char* html_links2se = "<a href=\"/guide.htm\">Guide</a><a href=\"/pec.htm\">PEC</a><a href=\"/settings.htm\" style=\"background-color: #552222;\">Settings</a>";
const char* html_links3se = "<a href=\"/wifi.htm\">WiFi</a><a href=\"/config.htm\">Config.h</a><br />";

const char* html_links1es = "<a href=\"/index.htm\">Status</a><a href=\"/control.htm\">Control</a>";
const char* html_links2es = "<a href=\"/guide.htm\">Guide</a><a href=\"/pec.htm\">PEC</a><a href=\"/settings.htm\">Settings</a>";
const char* html_links3es = "<a href=\"/wifi.htm\" style=\"background-color: #552222;\">WiFi</a><a href=\"/config.htm\">Config.h</a><br />";

const char* html_links1co = "<a href=\"/index.htm\">Status</a><a href=\"/control.htm\">Control</a>";
const char* html_links2co = "<a href=\"/guide.htm\">Guide</a><a href=\"/pec.htm\">PEC</a><a href=\"/settings.htm\">Settings</a>";
const char* html_links3co = "<a href=\"/wifi.htm\">WiFi</a><a href=\"/config.htm\" style=\"background-color: #552222;\">Config.h</a><br />";

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}



void setup(void) {
  initMotor();
  telInfo.lastState = millis();
  setupButton();
  display.begin();
  drawIntro();
  tickButtons();
  delay(2000);
  tickButtons();
  display.firstPage();
  uint8_t x = 0;
  do {
    display.setFont(u8g2_font_helvR14_tr);
    x = (display.getDisplayWidth() - display.getStrWidth("Loading")) / 2;
    display.drawStr(x, 25, "Loading");
    x = (display.getDisplayWidth() - display.getStrWidth("Version 0.0")) / 2;
    display.drawStr(x, 55, "Version 0.0");
  } while (display.nextPage());


  setupWifi();

  display.firstPage();
  do {
    x = (display.getDisplayWidth() - display.getStrWidth("Ready!")) / 2;
    display.drawStr(x, 40, "Ready!");
  } while (display.nextPage());
  delay(500);

}


void setupWifi()

{
#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
#endif
  EEPROM.begin(1024);

  // EEPROM Init
  if ((EEPROM_readInt(0) != 8266) || (EEPROM_readInt(2) != 0)) {
    EEPROM_writeInt(0, 8266);
    EEPROM_writeInt(2, 0);

    EEPROM_writeInt(4, (int)accessPointEnabled);
    EEPROM_writeInt(6, (int)stationEnabled);
    EEPROM_writeInt(8, (int)stationDhcpEnabled);

    EEPROM_writeInt(10, (int)WebTimeout);
    EEPROM_writeInt(12, (int)CmdTimeout);
    EEPROM.write(14, maxContrast);
    EEPROM.write(15, wifiOn);
 
    EEPROM_writeString(100, wifi_sta_ssid);
    EEPROM_writeString(150, wifi_sta_pwd);
    EEPROM_writeString(200, masterPassword);
    EEPROM.write(20, wifi_sta_ip[0]); EEPROM.write(21, wifi_sta_ip[1]); EEPROM.write(22, wifi_sta_ip[2]); EEPROM.write(23, wifi_sta_ip[3]);
    EEPROM.write(24, wifi_sta_gw[0]); EEPROM.write(25, wifi_sta_gw[1]); EEPROM.write(26, wifi_sta_gw[2]); EEPROM.write(27, wifi_sta_gw[3]);
    EEPROM.write(28, wifi_sta_sn[0]); EEPROM.write(29, wifi_sta_sn[1]); EEPROM.write(30, wifi_sta_sn[2]); EEPROM.write(31, wifi_sta_sn[3]);

    sprintf(wifi_ap_ssid, "TeenAstro_%u", random(1000));
    EEPROM_writeString(500, wifi_ap_ssid);
    EEPROM_writeString(550, wifi_ap_pwd);
    EEPROM_writeInt(50, (int)wifi_ap_ch);
    EEPROM.write(60, wifi_ap_ip[0]); EEPROM.write(61, wifi_ap_ip[1]); EEPROM.write(62, wifi_ap_ip[2]); EEPROM.write(63, wifi_ap_ip[3]);
    EEPROM.write(70, wifi_ap_gw[0]); EEPROM.write(71, wifi_ap_gw[1]); EEPROM.write(72, wifi_ap_gw[2]); EEPROM.write(73, wifi_ap_gw[3]);
    EEPROM.write(80, wifi_ap_sn[0]); EEPROM.write(81, wifi_ap_sn[1]); EEPROM.write(82, wifi_ap_sn[2]); EEPROM.write(83, wifi_ap_sn[3]);
    EEPROM.commit();
  }
  else {
    accessPointEnabled = EEPROM_readInt(4);
    stationEnabled = EEPROM_readInt(6);
    stationDhcpEnabled = EEPROM_readInt(8);

    WebTimeout = EEPROM_readInt(10);
    CmdTimeout = EEPROM_readInt(12);
    maxContrast = EEPROM.read(14);
    display.setContrast(maxContrast);
    wifiOn = EEPROM.read(15);
    EEPROM_readString(100, wifi_sta_ssid);
    EEPROM_readString(150, wifi_sta_pwd);
    EEPROM_readString(200, masterPassword);
    wifi_sta_ip[0] = EEPROM.read(20); wifi_sta_ip[1] = EEPROM.read(21); wifi_sta_ip[2] = EEPROM.read(22); wifi_sta_ip[3] = EEPROM.read(23);
    wifi_sta_gw[0] = EEPROM.read(24); wifi_sta_gw[1] = EEPROM.read(25); wifi_sta_gw[2] = EEPROM.read(26); wifi_sta_gw[3] = EEPROM.read(27);
    wifi_sta_sn[0] = EEPROM.read(28); wifi_sta_sn[1] = EEPROM.read(29); wifi_sta_sn[2] = EEPROM.read(30); wifi_sta_sn[3] = EEPROM.read(31);

    EEPROM_readString(500, wifi_ap_ssid);
    EEPROM_readString(550, wifi_ap_pwd);
    wifi_ap_ch = EEPROM_readInt(50);
    wifi_ap_ip[0] = EEPROM.read(60); wifi_ap_ip[1] = EEPROM.read(61); wifi_ap_ip[2] = EEPROM.read(62); wifi_ap_ip[3] = EEPROM.read(63);
    wifi_ap_gw[0] = EEPROM.read(70); wifi_ap_gw[1] = EEPROM.read(71); wifi_ap_gw[2] = EEPROM.read(72); wifi_ap_gw[3] = EEPROM.read(73);
    wifi_ap_sn[0] = EEPROM.read(80); wifi_ap_sn[1] = EEPROM.read(81); wifi_ap_sn[2] = EEPROM.read(82); wifi_ap_sn[3] = EEPROM.read(83);
  }

#ifndef DEBUG_ON
  Serial.begin(SERIAL_BAUD_DEFAULT);
#ifdef SERIAL_SWAP_ON
  Serial.swap();
#endif

Again:
#ifdef LED_PIN
  digitalWrite(LED_PIN, LOW);
#endif
  char c = 0;

  // clear the buffers and any noise on the serial lines
  for (int i = 0; i < 3; i++) {
    Serial.print(":#");
#ifdef LED_PIN
    digitalWrite(LED_PIN, HIGH);
#endif
    delay(500);
    Serial.flush();
    serialRecvFlush();
#ifdef LED_PIN
    digitalWrite(LED_PIN, LOW);
#endif
    delay(500);
  }

  // safety net
  if ((c == 'R') || (!accessPointEnabled && !stationEnabled)) {
    // reset EEPROM values, triggers an init
    EEPROM_writeInt(0, 0); EEPROM_writeInt(2, 0);
    EEPROM.commit();
    Serial.println();
    Serial.println("Cycle power for reset to defaults.");
    Serial.println();
  }

  // switch OnStep Serial1 up to ? baud
  Serial.print(SERIAL_BAUD);
  delay(100);
  c = 0;
  if (Serial.available() > 0) { c = Serial.read(); }
  if (c == '1') {
    if (!strcmp(SERIAL_BAUD, ":SB0#")) Serial.begin(115200); else
      if (!strcmp(SERIAL_BAUD, ":SB1#")) Serial.begin(57600); else
        if (!strcmp(SERIAL_BAUD, ":SB2#")) Serial.begin(38400); else
          if (!strcmp(SERIAL_BAUD, ":SB3#")) Serial.begin(28800); else
            if (!strcmp(SERIAL_BAUD, ":SB4#")) Serial.begin(19200); else Serial.begin(9600);
#ifdef SERIAL_SWAP_ON
    Serial.swap();
#endif
  }
  else {
#ifdef LED_PIN
    digitalWrite(LED_PIN, HIGH);
#endif
    delay(1000);
    //goto Again;
  }
#else
  Serial.begin(115200);
  delay(10000);

  Serial.println(accessPointEnabled);
  Serial.println(stationEnabled);
  Serial.println(stationDhcpEnabled);

  Serial.println(WebTimeout);
  Serial.println(CmdTimeout);

  Serial.println(wifi_sta_ssid);
  Serial.println(wifi_sta_pwd);
  Serial.println(wifi_sta_ip.toString());
  Serial.println(wifi_sta_gw.toString());
  Serial.println(wifi_sta_sn.toString());

  Serial.println(wifi_ap_ssid);
  Serial.println(wifi_ap_pwd);
  Serial.println(wifi_ap_ch);
  Serial.println(wifi_ap_ip.toString());
  Serial.println(wifi_ap_gw.toString());
  Serial.println(wifi_ap_sn.toString());

#endif


  if (wifiOn)
  {
    if ((stationEnabled) && (!stationDhcpEnabled)) WiFi.config(wifi_sta_ip, wifi_sta_gw, wifi_sta_sn);

    if (accessPointEnabled) WiFi.softAPConfig(wifi_ap_ip, wifi_ap_gw, wifi_ap_sn);

    if (accessPointEnabled && !stationEnabled)
    {
      WiFi.softAP(wifi_ap_ssid, wifi_ap_pwd, wifi_ap_ch);
      WiFi.mode(WIFI_AP);
    }
    else if (!accessPointEnabled && stationEnabled)
    {
      WiFi.softAPdisconnect(true);
      WiFi.begin(wifi_sta_ssid, wifi_sta_pwd);
      WiFi.mode(WIFI_STA);
    }
    else if (accessPointEnabled && stationEnabled)
    {
      WiFi.softAP(wifi_ap_ssid, wifi_ap_pwd, wifi_ap_ch);
      WiFi.begin(wifi_sta_ssid, wifi_sta_pwd);
      WiFi.mode(WIFI_AP_STA);
    }

    // clear the buffers and any noise on the serial lines
    for (int i = 0; i < 3; i++) {
      Serial.print(":#");
      delay(50);
      serialRecvFlush();
    }

    // Wait for connection
    if (stationEnabled) {
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
      }
    }

    server.on("/", handleRoot);
    server.on("/index.htm", handleRoot);
    server.on("/settings.htm", handleSettings);
    server.on("/control.htm", handleControl);
    server.on("/guide.htm", handleGuide);
    server.on("/guide.txt", handleGuideAjax);
    server.on("/wifi.htm", handleWifi);
    server.on("/config.htm", handleConfig);

    server.onNotFound(handleNotFound);

    cmdSvr.begin();
    cmdSvr.setNoDelay(true);
    server.begin();
  }
  else
  {
    WiFi.mode(WIFI_OFF);
  }

#ifdef DEBUG_ON
  Serial.println("HTTP server started");
#endif
}

void updateWifi()
{
  if (!wifiOn)
    return;

  server.handleClient();

  // disconnect client
  if (cmdSvrClient && (!cmdSvrClient.connected())) cmdSvrClient.stop();
  if (cmdSvrClient && ((long)(clientTime - millis()) < 0)) cmdSvrClient.stop();

  // new client
  if (!cmdSvrClient && (cmdSvr.hasClient())) {
    // find free/disconnected spot
    cmdSvrClient = cmdSvr.available();
    clientTime = millis() + 2000UL;
  }

  // check clients for data, if found get the command, send cmd and pickup the response, then return the response
  while (cmdSvrClient && cmdSvrClient.connected() && (cmdSvrClient.available() > 0)) {
    // get the data
    byte b = cmdSvrClient.read();
    writeBuffer[writeBufferPos] = b; writeBufferPos++; if (writeBufferPos > 39) writeBufferPos = 39; writeBuffer[writeBufferPos] = 0;

    // send cmd and pickup the response
    if ((b == '#') || ((strlen(writeBuffer) == 1) && (b == (char)6))) {
      char readBuffer[40] = "";
      readLX200Bytes(writeBuffer, readBuffer, CmdTimeout); writeBuffer[0] = 0; writeBufferPos = 0;

      // return the response, if we have one
      if (strlen(readBuffer) > 0) {
        if (cmdSvrClient && cmdSvrClient.connected()) {
          cmdSvrClient.print(readBuffer);
          delay(2);
        }
      }
    }
    else server.handleClient();
  }
}


void setupButton()
{
  setticks(buttons[B_SHIFT]);
  buttons[B_SHIFT].attachClick(click_s);
  buttons[B_SHIFT].attachDoubleClick(doubleclick_s);
  buttons[B_SHIFT].attachLongPressStart(longPressStart_s);
  buttons[B_SHIFT].attachLongPressStop(longPressStop_s);
  buttons[B_SHIFT].attachDuringLongPress(longPress_s);

  setticks(buttons[B_NORTH]);
  buttons[B_NORTH].attachClick(click_N);
  //buttons[B_NORTH].attachDoubleClick(doubleclick_N);
  buttons[B_NORTH].attachLongPressStart(longPressStart_N);
  buttons[B_NORTH].attachLongPressStop(longPressStop_N);
  buttons[B_NORTH].attachDuringLongPress(longPress_N);

  setticks(buttons[B_SOUTH]);
  buttons[B_SOUTH].attachClick(click_S);
  //buttons[B_SOUTH].attachDoubleClick(doubleclick_S);
  buttons[B_SOUTH].attachLongPressStart(longPressStart_S);
  buttons[B_SOUTH].attachLongPressStop(longPressStop_S);
  buttons[B_SOUTH].attachDuringLongPress(longPress_S);

  setticks(buttons[B_EAST]);
  buttons[B_EAST].attachClick(click_E);
  //buttons[B_EAST].attachDoubleClick(doubleclick_E);
  buttons[B_EAST].attachLongPressStart(longPressStart_E);
  buttons[B_EAST].attachLongPressStop(longPressStop_E);
  buttons[B_EAST].attachDuringLongPress(longPress_E);

  setticks(buttons[B_WEST]);
  buttons[B_WEST].attachClick(click_W);
  //buttons[B_WEST].attachDoubleClick(doubleclick_W);
  buttons[B_WEST].attachLongPressStart(longPressStart_W);
  buttons[B_WEST].attachLongPressStop(longPressStop_W);
  buttons[B_WEST].attachDuringLongPress(longPress_W);
  buttons[B_WEST].attachClick(click_W);

  setticks(buttons[B_F]);
  buttons[B_F].attachClick(click_F);
  buttons[B_F].attachDoubleClick(doubleclick_F);
  buttons[B_F].attachLongPressStart(longPressStart_F);
  buttons[B_F].attachLongPressStop(longPressStop_F);
  buttons[B_F].attachDuringLongPress(longPress_F);

  setticks(buttons[B_f]);
  buttons[B_f].attachClick(click_f);
  buttons[B_f].attachDoubleClick(doubleclick_f);
  buttons[B_f].attachLongPressStart(longPressStart_f);
  buttons[B_f].attachLongPressStop(longPressStop_f);
  buttons[B_f].attachDuringLongPress(longPress_f);

}

void tickButtons()
{
  delay(10);
  buttonPressed = false;
  for (int k = 0; k < 7; k++)
  {
    delay(1);
    eventbuttons[k] = E_NONE;
    buttons[k].tick();
  }
  for (int k = 0; k < 7; k++)
  {
    if (eventbuttons[k] != 0)
    {
      buttonPressed = true;
    }
  }
  for (int k = 1; k < 6; k += 2)
  {
    if (eventbuttons[k] == eventbuttons[k + 1])
    {
      eventbuttons[k] = E_NONE;
      eventbuttons[k + 1] = E_NONE;
    }
  }
}


void loop() {
  updateWifi();
  tickButtons();
  //if (Serial1.available())
  //{//-- Affichage sur la console des données  
  //  Serial1.write(Serial1.read());
  unsigned long top = millis();
  if (buttonPressed)
  {
    if (sleepDisplay)
    {
      display.setContrast(maxContrast);
      display.sleepOff();
      sleepDisplay = false;
      lowContrast = false;
      time_last_action = millis();
    }
    if (lowContrast)
    {
      display.setContrast(maxContrast);
      lowContrast = false;
      time_last_action = top;
    }
  }
  else if (sleepDisplay)
  {
    return;
  }
  else if (top - time_last_action > 120000)
  {
    display.sleepOn();
    sleepDisplay = true;
    return;
  }
  else if (top - time_last_action > 30000 && !lowContrast)
  {
    display.setContrast(0);
    lowContrast = true;
    return;
  }

  if (powerCylceRequired)
  {
    display.setFont(u8g2_font_helvR12_tr);
    DisplayMessage("REBOOT", "DEVICE", 1000);
    return;
  }

  if (align == ALI_SELECT_STAR_1 || align == ALI_SELECT_STAR_2 || align == ALI_SELECT_STAR_3)
  {
    if (align == ALI_SELECT_STAR_1)
      DisplayLongMessage("Select a Star", "near the Meridian", "& the Celestial Equ.", "in the Western Sky", -1);
    else if (align == ALI_SELECT_STAR_2)
      DisplayLongMessage("Select a Star", "near the Meridian", "& the Celestial Equ.", "in the Eastern Sky", -1);
    else if (align == ALI_SELECT_STAR_3)
      DisplayLongMessage("Select a Star", "HA = -3 hour", "Dec = +- 45 degree", "in the Eastern Sky", -1);
    if (!SelectStarAlign())
    {
      DisplayMessage("Alignment", "Aborted", -1);
      align = ALI_OFF;
      return;
    }
    align += 1;
  }
  else if (top - lastpageupdate > 100)
  {
    update_main(display.getU8g2(), page);
  }
  if (telInfo.connected == false)
  {
    DisplayMessage("Hand controler", "not connected", -1);
  }
  if (telInfo.connected && (telInfo.getTrackingState() == TRK_SLEWING || telInfo.getParkState() == PRK_PARKING))
  {
    bool stop = (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART || eventbuttons[0] == E_DOUBLECLICK) ? true : false;
    int it = 1;
    while (!stop && it < 5)
    {
      stop = (eventbuttons[it] == E_LONGPRESS || eventbuttons[it] == E_CLICK || eventbuttons[it] == E_LONGPRESSTART);
      it++;
    }
    if (stop)
    {
      Serial.print(":Q#");
      Serial.flush();
      time_last_action = millis();
      display.sleepOff();
      if (align != ALI_OFF)
      {
        align -= 1;
      }
      return;
    }

  }
  else
  {
    buttonCommand = false;
    for (int k = 1; k < 5; k++)
    {
      if (Move[k - 1] && (eventbuttons[k] == E_LONGPRESSSTOP || eventbuttons[k] == E_NONE))
      {
        buttonCommand = true;
        Move[k - 1] = false;
        Serial.print(BreakRC[k - 1]);
        Serial.flush();
        continue;
      }
      else if (!Move[k - 1] && (eventbuttons[k] == E_LONGPRESS || eventbuttons[k] == E_CLICK || eventbuttons[k] == E_LONGPRESSTART))
      {
        buttonCommand = true;
        Move[k - 1] = true;
        Serial.print(RC[k - 1]);
        Serial.flush();
        continue;
      }
    }
    if (buttonCommand)
    {
      time_last_action = millis();
      return;
    }

  }

  if (eventbuttons[0] == E_DOUBLECLICK /*|| eventbuttons[0] == E_CLICK)  && eventbuttons[1] != E_NONE*/)
  {
    menuSpeedRate();
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_CLICK && align == ALI_OFF)
  {
    page++;
    if (page > 2) page = 0;
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_LONGPRESS && align == ALI_OFF)
  {
    menuMain();
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_CLICK && (align == ALI_RECENTER_1 || align == ALI_RECENTER_2 || align == ALI_RECENTER_3))
  {
    addStar();
  }


}






