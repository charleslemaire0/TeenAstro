/*
 * TeenAstroWifi_stub.cpp -- No-op implementation of TeenAstroWifi for the SHC emulator.
 *
 * All WiFi/web-server functionality is disabled; communication goes
 * directly over TCP serial (TcpClientStream).
 */
#include <TeenAstroWifi.h>

bool TeenAstroWifi::wifiOn = false;
bool TeenAstroWifi::s_handlerBusy = false;
unsigned long TeenAstroWifi::s_lastPageMs = 0;
int TeenAstroWifi::WebTimeout = 4;
int TeenAstroWifi::CmdTimeout = 8;
TeenAstroWifi::WifiConnectMode TeenAstroWifi::activeWifiConnectMode = AutoClose;
LX200Client* TeenAstroWifi::s_client = nullptr;
char TeenAstroWifi::masterPassword[40] = "password";
TeenAstroWifi::WifiMode TeenAstroWifi::activeWifiMode = OFF;
bool TeenAstroWifi::stationDhcpEnabled[3] = {false, false, false};
char TeenAstroWifi::wifi_sta_ssid[3][40] = {{0}, {0}, {0}};
char TeenAstroWifi::wifi_sta_pwd[3][40] = {{0}, {0}, {0}};
IPAddress TeenAstroWifi::wifi_sta_ip[3] = {};
IPAddress TeenAstroWifi::wifi_sta_gw[3] = {};
IPAddress TeenAstroWifi::wifi_sta_sn[3] = {};
char TeenAstroWifi::wifi_ap_ssid[40] = "TeenAstro";
char TeenAstroWifi::wifi_ap_pwd[40] = "password";
byte TeenAstroWifi::wifi_ap_ch = 1;
IPAddress TeenAstroWifi::wifi_ap_ip;
IPAddress TeenAstroWifi::wifi_ap_gw;
IPAddress TeenAstroWifi::wifi_ap_sn;

#ifdef ARDUINO_ARCH_ESP8266
ESP8266WebServer TeenAstroWifi::server(80);
#endif
WiFiServer TeenAstroWifi::cmdSvr(9999);
WiFiClient TeenAstroWifi::cmdSvrClient;

bool TeenAstroWifi::busyGuard() { return false; }
bool TeenAstroWifi::isWifiOn() { return false; }
bool TeenAstroWifi::isWifiRunning() { return false; }
void TeenAstroWifi::turnWifiOn(bool) {}
void TeenAstroWifi::setup() {}
void TeenAstroWifi::update() {}
void TeenAstroWifi::getIP(uint8_t* ip) { memset(ip, 0, 4); }
int  TeenAstroWifi::getWifiMode() { return 4; /* OFF */ }
const char* TeenAstroWifi::getPassword() { return masterPassword; }
bool TeenAstroWifi::setWifiMode(int) { return false; }
void TeenAstroWifi::getStationName(int, char* SSID) { SSID[0] = 0; }
void TeenAstroWifi::initFromEEPROM() {}
void TeenAstroWifi::writeStation2EEPROM(const int&) {}
void TeenAstroWifi::writeAccess2EEPROM() {}
