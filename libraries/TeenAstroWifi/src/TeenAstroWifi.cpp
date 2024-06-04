#include <ArduinoOTA.h>
#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"


const char html_headB[] PROGMEM = "<!DOCTYPE HTML>\r\n<html>\r\n<head>\r\n";
const char html_headerIdx[] PROGMEM = "<meta http-equiv=\"refresh\" content=\"5; URL=/index.htm\">\r\n";
const char html_headE[] PROGMEM = "</head>\r\n";
const char html_bodyB[] PROGMEM = "<body bgcolor='#26262A'>\r\n";

const char html_main_cssB[] PROGMEM = "<STYLE>\n";
const char html_main_css1[] PROGMEM = ".clear { clear: both; } .a {  font-family: Helvetica; background-color: #111111; } .t { font-family: Helvetica; padding: 10px 10px 20px 10px; border: 5px solid #551111;\n";
const char html_main_css2[] PROGMEM = " margin: 25px 25px 0px 25px; color: #999999; background-color: #111111; min-width: 10em; } input {font-family: Helvetica; font-weight: bold; width:6em; background-color: #A01010; padding: 2px 2px; }\n";
const char html_main_css3[] PROGMEM = " .b { font-family: Helvetica; font-size: 75%; padding: 10px; border-left: 5px solid #551111; border-right: 5px solid #551111; border-bottom: 5px solid #551111; margin: 0px 25px 25px 25px; color: #999999; background-color: #111111; min-width: 10em; }\n";
const char html_main_css4[] PROGMEM = " .bt { font-size: 125%; }\n";
const char html_main_css5[] PROGMEM = " select { width:7em; font-weight: bold; font-family: Helvetica; background-color: #A01010; padding: 2px 2px; } .c { color: #A01010; font-weight: bold}\n";
const char html_main_css6[] PROGMEM = "h1 { text-align: right; } a:hover, a:active { background-color: red; } .y { color: #FFFF00; font-weight: bold; font-family: Helvetica;}\n";
const char html_main_css7[] PROGMEM = "a:link, a:visited { background-color: #332222; color: #a07070;  font-family: Helvetica; border:1px solid red; padding: 5px 10px;\n";
const char html_main_css8[] PROGMEM = " margin: none; text-align: center; text-decoration: none; display: inline-block;}\n";
const char html_main_css9[] PROGMEM = "button { background-color: #A01010; font-weight: bold; border-radius: 5px; margin: 2px; padding: 4px 8px; }\n";
const char html_main_css_control1[] PROGMEM = ".b1 { font-family: Helvetica; float: left; border: 2px solid #551111; background-color: #181818; text-align: center; margin: 5px; padding: 15px; padding-top: 3px; }\n";
const char html_main_css_control2[] PROGMEM = ".gb { width: 60px; height: 50px; padding: 0px; }\n";
const char html_main_css_control3[] PROGMEM = ".bb { height: 2.5em; width: 8em;} .bbh {   height: 2.5em; width: 6em;}\n";
const char html_main_css_control4[] PROGMEM = ".bct { font-family: Helvetica; font-size: 125% }\n";
const char html_main_cssE[] PROGMEM = "</STYLE>\n";

const char html_header1[] PROGMEM = "<div class='t'><table width='100%%'><tr><td><b><font size='5'>\n";
const char html_header2[] PROGMEM = "</font></b></td><td align='right'><b>" Product " " ServerFirmwareVersionMajor "." ServerFirmwareVersionMinor "." ServerFirmwareVersionPatch ", Main Unit \n";
const char html_header3[] PROGMEM = "</b></td></tr></table>\n";
const char html_header4[] PROGMEM = "</div><div class='b'>\r\n";

const char html_links1S[] PROGMEM = "<a href='/index.htm' style='background-color: #552222;'>Status</a>\n";
const char html_links1N[] PROGMEM = "<a href='/index.htm'>Status</a>\n";
const char html_links2S[] PROGMEM = "<a href='/control.htm' style='background-color: #552222;'>Control</a>\n";
const char html_links2N[] PROGMEM = "<a href='/control.htm'>Control</a>\n";
const char html_links3S[] PROGMEM = "<a href='/configuration_speed.htm' style='background-color: #552222;'>Speed</a>\n";
const char html_links3N[] PROGMEM = "<a href='/configuration_speed.htm'>Speed</a>\n";
const char html_links4S[] PROGMEM = "<a href='/configuration_tracking.htm' style='background-color: #552222;'>Tracking</a>\n";
const char html_links4N[] PROGMEM = "<a href='/configuration_tracking.htm'>Tracking</a>\n";
const char html_links5S[] PROGMEM = "<a href='/configuration_site.htm' style='background-color: #552222;'>Site</a>\n";
const char html_links5N[] PROGMEM = "<a href='/configuration_site.htm'>Site</a>\n";
const char html_links6S[] PROGMEM = "<a href='/configuration_mount.htm' style='background-color: #552222;'>Mount</a>\n";
const char html_links6N[] PROGMEM = "<a href='/configuration_mount.htm'>Mount</a>\n";
const char html_links7S[] PROGMEM = "<a href='/configuration_motors.htm' style='background-color: #552222;'>Motors</a>\n";
const char html_links7N[] PROGMEM = "<a href='/configuration_motors.htm'>Motors</a>\n";
const char html_links8S[] PROGMEM = "<a href='/configuration_limits.htm' style='background-color: #552222;'>Limits</a>\n";
const char html_links8N[] PROGMEM = "<a href='/configuration_limits.htm'>Limits</a>\n";
const char html_links9S[] PROGMEM = "<a href='/configuration_encoders.htm' style='background-color: #552222;'>Encoders</a>\n";
const char html_links9N[] PROGMEM = "<a href='/configuration_encoders.htm'>Encoders</a>\n";
const char html_links10S[] PROGMEM = "<a href='/configuration_focuser.htm' style='background-color: #552222;'>Focuser</a>\n";
const char html_links10N[] PROGMEM = "<a href='/configuration_focuser.htm'>Focuser</a>\n";
const char html_links11S[] PROGMEM = "<a href='/wifi.htm' style='background-color: #552222;'>WiFi</a><br />\n";
const char html_links11N[] PROGMEM = "<a href='/wifi.htm'>WiFi</a><br />\n";

bool TeenAstroWifi::wifiOn = true;

int TeenAstroWifi::WebTimeout = TIMEOUT_WEB;
int TeenAstroWifi::CmdTimeout = TIMEOUT_CMD;
TeenAstroWifi::WifiConnectMode TeenAstroWifi::activeWifiConnectMode = WifiConnectMode::AutoClose;

char TeenAstroWifi::masterPassword[40] = Default_Password;

TeenAstroWifi::WifiMode TeenAstroWifi::activeWifiMode = WifiMode::M_AcessPoint;
bool TeenAstroWifi::stationDhcpEnabled[3] = { true, true, true };

char TeenAstroWifi::wifi_sta_ssid[3][40] = { "", "", "" };
char TeenAstroWifi::wifi_sta_pwd[3][40] = { "", "", "" };

IPAddress TeenAstroWifi::wifi_sta_ip[3] = { IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1) };
IPAddress TeenAstroWifi::wifi_sta_gw[3] = { IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1) };
IPAddress TeenAstroWifi::wifi_sta_sn[3] = { IPAddress(255, 255, 255, 0), IPAddress(255, 255, 255, 0), IPAddress(255, 255, 255, 0) };

char TeenAstroWifi::wifi_ap_ssid[40] = "TeenAstro";
char TeenAstroWifi::wifi_ap_pwd[40] = "password";
byte TeenAstroWifi::wifi_ap_ch = 7;

IPAddress TeenAstroWifi::wifi_ap_ip = IPAddress(192, 168, 0, 1);
IPAddress TeenAstroWifi::wifi_ap_gw = IPAddress(192, 168, 0, 1);
IPAddress TeenAstroWifi::wifi_ap_sn = IPAddress(255, 255, 255, 0);



WiFiServer TeenAstroWifi::cmdSvr = WiFiServer(9999);
WiFiClient TeenAstroWifi::cmdSvrClient;

#ifdef ARDUINO_ARCH_ESP8266
ESP8266WebServer TeenAstroWifi::server;
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#endif 

#ifdef ARDUINO_ARCH_ESP32
WebServer TeenAstroWifi::server;
WebServer server(80);
HTTPUpdateServer httpUpdater;
#endif

// -----------------------------------------------------------------------------------
// EEPROM related functions

// write int numbers into EEPROM at position i (2 bytes)
void TeenAstroWifi::EEPROM_writeInt(int i, int j) {
  uint8_t *k = (uint8_t*)&j;
  EEPROM.write(i + 0, *k); k++;
  EEPROM.write(i + 1, *k);
}

// read int numbers from EEPROM at position i (2 bytes)
int TeenAstroWifi::EEPROM_readInt(int i) {
  uint16_t j;
  uint8_t *k = (uint8_t*)&j;
  *k = EEPROM.read(i + 0); k++;
  *k = EEPROM.read(i + 1);
  return j;
}

// write 4 byte variable into EEPROM at position i (4 bytes)
void TeenAstroWifi::EEPROM_writeQuad(int i, byte *v) {
  EEPROM.write(i + 0, *v); v++;
  EEPROM.write(i + 1, *v); v++;
  EEPROM.write(i + 2, *v); v++;
  EEPROM.write(i + 3, *v);
}

// read 4 byte variable from EEPROM at position i (4 bytes)
void TeenAstroWifi::EEPROM_readQuad(int i, byte *v) {
  *v = EEPROM.read(i + 0); v++;
  *v = EEPROM.read(i + 1); v++;
  *v = EEPROM.read(i + 2); v++;
  *v = EEPROM.read(i + 3);
}

// write String into EEPROM at position i (40 bytes)
void TeenAstroWifi::EEPROM_writeString(int i, char l[]) {
  for (int l1 = 0; l1<40; l1++) {
    EEPROM.write(i + l1, *l); l++;
  }
}

// read String from EEPROM at position i (40 bytes)
void TeenAstroWifi::EEPROM_readString(int i, char l[]) {
  for (int l1 = 0; l1<40; l1++) {
    *l = EEPROM.read(i + l1); l++;
  }
}

// write 4 byte float into EEPROM at position i (4 bytes)
void TeenAstroWifi::EEPROM_writeFloat(int i, float f) {
  EEPROM_writeQuad(i, (byte*)&f);
}

// read 4 byte float from EEPROM at position i (4 bytes)
float TeenAstroWifi::EEPROM_readFloat(int i) {
  float f;
  EEPROM_readQuad(i, (byte*)&f);
  return f;
}

// write 4 byte long into EEPROM at position i (4 bytes)
void TeenAstroWifi::EEPROM_writeLong(int i, long l) {
  EEPROM_writeQuad(i, (byte*)&l);
}

// read 4 byte long from EEPROM at position i (4 bytes)
long TeenAstroWifi::EEPROM_readLong(int i) {
  long l;
  EEPROM_readQuad(i, (byte*)&l);
  return l;
}

bool TeenAstroWifi::atoi2(char *a, int *i) {
  char *conv_end;
  long l = strtol(a, &conv_end, 10);

  if ((l<-32767) || (l>32768) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
};

bool TeenAstroWifi::atof2(char *a, float *f) {
  char *conv_end;
  double l = strtof(a, &conv_end);

  if (&a[0] == conv_end) return false;
  *f = l;
  return true;
};

void TeenAstroWifi::handleNotFound()
{
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

void TeenAstroWifi::preparePage(String &data, ServerPage page)
{
  char temp1[80] = "";
  data = FPSTR(html_headB);
  if (!ta_MountStatus.hasFocuser() && page == ServerPage::Focuser)
  {
    page = ServerPage::Index;
  }
  else if (!ta_MountStatus.encodersEnable() && page == ServerPage::Encoders)
  {
    page = ServerPage::Index;
  }

  if (page == ServerPage::Index)
    data += FPSTR(html_headerIdx);
  data += FPSTR(html_main_cssB);
  data += FPSTR(html_main_css1);
  data += FPSTR(html_main_css2);
  data += FPSTR(html_main_css3);
  data += FPSTR(html_main_css4);
  data += FPSTR(html_main_css5);
  data += FPSTR(html_main_css6);
  data += FPSTR(html_main_css7);
  data += FPSTR(html_main_css8);
  data += FPSTR(html_main_css9);
  sendHtml(data);
  if (page == ServerPage::Control ||
      page == ServerPage::Tracking ||
      page == ServerPage::Site)
  {
    data += FPSTR(html_main_css_control1);
    data += FPSTR(html_main_css_control2);
    data += FPSTR(html_main_css_control3);
    data += FPSTR(html_main_css_control4);
    sendHtml(data);
  }
  data += FPSTR(html_main_cssE);
  data += FPSTR(html_headE);
  data += FPSTR(html_bodyB);
 
  // finish the standard http response header
  data += FPSTR(html_header1);
  if (ta_MountStatus.hasInfoV()) data += ta_MountStatus.getVP(); else data += "Connection to TeenAstro Main unit is lost";
  data += FPSTR(html_header2);
  if (ta_MountStatus.hasInfoV())
  {
    data += ta_MountStatus.getVN();
    data += " Board ";
    data += ta_MountStatus.getVB();
    data += " Driver ";
    switch (ta_MountStatus.getVb()[0])
    {
    default:
    case '0':
      data += "Generic";
      break;
    case '1':
      data += "TOS100";
      break;
    case '2':
      data += "TMC2130";
      break;
    case '3':
      data += "TMC5160";
      break;
    case '4':
      data += "TMC2160";
      break;
    }
  }
  else data += "?";
  data += FPSTR(html_header3);
  data += page == ServerPage::Index ? FPSTR(html_links1S) : FPSTR(html_links1N);
  data += page == ServerPage::Control ? FPSTR(html_links2S) : FPSTR(html_links2N);
  if (ta_MountStatus.motorsEnable())
  {
    data += page == ServerPage::Speed ? FPSTR(html_links3S) : FPSTR(html_links3N);
    data += page == ServerPage::Tracking ? FPSTR(html_links4S) : FPSTR(html_links4N);

  }
  data += page == ServerPage::Site ? FPSTR(html_links5S) : FPSTR(html_links5N);
  data += page == ServerPage::Mount ? FPSTR(html_links6S) : FPSTR(html_links6N);
  if (ta_MountStatus.motorsEnable())
  {
    data += page == ServerPage::Motors ? FPSTR(html_links7S) : FPSTR(html_links7N);
  }
  data += page == ServerPage::Limits ? FPSTR(html_links8S) : FPSTR(html_links8N);
  if (ta_MountStatus.encodersEnable())
  {
    data += page == ServerPage::Encoders ? FPSTR(html_links9S) : FPSTR(html_links9N);
  }
  if (ta_MountStatus.hasFocuser())
  {
    data += page == ServerPage::Focuser ? FPSTR(html_links10S) : FPSTR(html_links10N);
  }
#ifndef OETHS
  data += page == ServerPage::Wifi ? FPSTR(html_links11S) : FPSTR(html_links11N);
#endif
  data += FPSTR(html_header4);
}

void TeenAstroWifi::writeStation2EEPROM(const int& k)
{
  if (k < NUM_sta)
  {
    unsigned int adress = EEPROM_start_wifi_sta + k * SIZE_sta;
    EEPROM.write(adress, stationDhcpEnabled[k]); adress++;
    for (int i = 0; i < 4; i++, adress++)
      EEPROM.write(adress, wifi_sta_ip[k][i]);
    for (int i = 0; i < 4; i++, adress++)
      EEPROM.write(adress, wifi_sta_gw[k][i]);
    for (int i = 0; i < 4; i++, adress++)
      EEPROM.write(adress, wifi_sta_sn[k][i]);
    EEPROM_writeString(adress, wifi_sta_ssid[k]); adress += sizeof(wifi_sta_ssid[k]);
    EEPROM_writeString(adress, wifi_sta_pwd[k]); adress += sizeof(wifi_sta_pwd[k]);
  }
}
void TeenAstroWifi::writeAccess2EEPROM()
{
  unsigned int adress = EEPROM_start_wifi_ap;
  for (int i = 0; i < 4; i++, adress++)
    EEPROM.write(adress, wifi_ap_ip[i]);
  for (int i = 0; i < 4; i++, adress++)
    EEPROM.write(adress, wifi_ap_gw[i]);
  for (int i = 0; i < 4; i++, adress++)
    EEPROM.write(adress, wifi_ap_sn[i]);
  EEPROM.write(adress, wifi_ap_ch); adress++;
  EEPROM_writeString(adress, wifi_ap_ssid); adress += sizeof(wifi_ap_ssid);
  EEPROM_writeString(adress, wifi_ap_pwd);
}

void TeenAstroWifi::initFromEEPROM()
{
#ifdef ARDUINO_D1_MINI32
  EEPROM.begin(512);
#else
  EEPROM.begin(1024);
#endif
  unsigned int adress = EEPROM_start;
  // EEPROM Init
  if (EEPROM.read(adress) != 82 || EEPROM.read(adress + 1) != 66 ||
      EEPROM.read(adress + 2) != 0 || EEPROM.read(adress + 3) != 0)
  {
    EEPROM.write(adress, 82); adress++;
    EEPROM.write(adress, 66); adress++;
    EEPROM.write(adress, 0); adress++;
    EEPROM.write(adress, 0); adress++;
    EEPROM.write(EEPROM_WifiOn, (uint8_t)wifiOn);
    EEPROM.write(EEPROM_WifiMode, (uint8_t)activeWifiMode);
    EEPROM.write(EEPROM_WebTimeout, (uint8_t)WebTimeout);
    EEPROM.write(EEPROM_CmdTimeout, (uint8_t)CmdTimeout); 
    EEPROM.write(EEPROM_WifiConnectMode, (uint8_t)activeWifiConnectMode);
    EEPROM_writeString(EPPROM_password, masterPassword);

    for (int k = 0; k < NUM_sta; k++)
    {
      writeStation2EEPROM(k);
    }
    writeAccess2EEPROM();
    EEPROM.commit();
  }
  else {
    wifiOn = EEPROM.read(EEPROM_WifiOn);
    uint8_t val = EEPROM.read(EEPROM_WifiMode);
    if (!wifiOn)
      activeWifiMode = WifiMode::OFF;
    else if (val > 5 || (val <3 && val> NUM_sta))
    {
      activeWifiMode = WifiMode::M_AcessPoint;
      EEPROM.write(EEPROM_WifiMode, (uint8_t)activeWifiMode);
      EEPROM.commit();
    }
    else
    {
      activeWifiMode = static_cast<WifiMode>(val);
    }

    WebTimeout = EEPROM.read(EEPROM_WebTimeout);
    CmdTimeout = EEPROM.read(EEPROM_CmdTimeout);
    val = EEPROM.read(EEPROM_WifiConnectMode);
    activeWifiConnectMode = static_cast<WifiConnectMode>(val < 2 ? val : 0 );
    EEPROM_readString(EPPROM_password, masterPassword);
    bool passwordok = false;
    for (int k = 0; k < 40; k++)
    {
      if (masterPassword[k] == 0)
      {
        if (k == 0)
        {
          break;
        }
        else
        {
          passwordok = true;
          break;
        }
      }
    }
    if (!passwordok)
    {
      strcpy(masterPassword,Default_Password);
      TeenAstroWifi::EEPROM_writeString(EPPROM_password, masterPassword);
      EEPROM.commit();
    }

    for (int k = 0; k < NUM_sta; k++)
    {
      adress = EEPROM_start_wifi_sta + k * SIZE_sta;
      stationDhcpEnabled[k] = EEPROM.read(adress); adress++;
      for (int i = 0; i < 4; i++, adress++)
         wifi_sta_ip[k][i] = EEPROM.read(adress);
      for (int i = 0; i < 4; i++, adress++)
        wifi_sta_gw[k][i] = EEPROM.read(adress);
      for (int i = 0; i < 4; i++, adress++)
        wifi_sta_sn[k][i] = EEPROM.read(adress);
      EEPROM_readString(adress, wifi_sta_ssid[k]); adress += sizeof(wifi_sta_ssid[k]);
      EEPROM_readString(adress, wifi_sta_pwd[k]);
    }
    adress = EEPROM_start_wifi_ap;
    for (int i = 0; i < 4; i++, adress++)
      wifi_ap_ip[i] = EEPROM.read(adress);
    for (int i = 0; i < 4; i++, adress++)
      wifi_ap_gw[i] = EEPROM.read(adress);
    for (int i = 0; i < 4; i++, adress++)
      wifi_ap_sn[i] = EEPROM.read(adress);
    wifi_ap_ch = EEPROM.read(adress); adress++;
    EEPROM_readString(adress, wifi_ap_ssid); adress += sizeof(wifi_ap_ssid);
    EEPROM_readString(adress, wifi_ap_pwd);
  }
}

void TeenAstroWifi::setup()
{
  initFromEEPROM();

#ifndef DEBUG_ON


  byte tb = 0;

  char c = 0;

  // safety net
  if ((c == 'R')) {//  ?? || activeWifiMode == WifiMode::OFF) {
    // reset EEPROM values, triggers an init
    EEPROM_writeInt(0, 0); EEPROM_writeInt(2, 0);
    activeWifiMode = WifiMode::M_AcessPoint;
    EEPROM.commit();
    //Ser.println();
    //Ser.println("Cycle power for reset to defaults.");
    //Ser.println();
  }


#else
  Ser.begin(115200);
  delay(10000);

  Ser.println(accessPointEnabled);
  Ser.println(stationEnabled);
  Ser.println(stationDhcpEnabled);

  Ser.println(WebTimeout);
  Ser.println(CmdTimeout);

  Ser.println(wifi_sta_ssid);
  Ser.println(wifi_sta_pwd);
  Ser.println(wifi_sta_ip.toString());
  Ser.println(wifi_sta_gw.toString());
  Ser.println(wifi_sta_sn.toString());

  Ser.println(wifi_ap_ssid);
  Ser.println(wifi_ap_pwd);
  Ser.println(wifi_ap_ch);
  Ser.println(wifi_ap_ip.toString());
  Ser.println(wifi_ap_gw.toString());
  Ser.println(wifi_ap_sn.toString());

#endif
  switch (activeWifiMode)
  {
  case TeenAstroWifi::OFF:
    WiFi.mode(WIFI_OFF);
    break;
  case TeenAstroWifi::M_AcessPoint:
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifi_ap_ssid, wifi_ap_pwd, wifi_ap_ch);
#ifdef ARDUINO_LOLIN_C3_MINI
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif // ARDUINO_LOLIN_C3_MINI
    delay(1000);
    WiFi.softAPConfig(wifi_ap_ip, wifi_ap_gw, wifi_ap_sn);
    delay(1000);
    break;
  case TeenAstroWifi::M_Station1:
  case TeenAstroWifi::M_Station2:
  case TeenAstroWifi::M_Station3:
    if (!stationDhcpEnabled[activeWifiMode])
    {
      WiFi.config(wifi_sta_ip[activeWifiMode], wifi_sta_gw[activeWifiMode], wifi_sta_sn[activeWifiMode]);
    }
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WiFiSleepType::WIFI_NONE_SLEEP);
    WiFi.begin(wifi_sta_ssid[activeWifiMode], wifi_sta_pwd[activeWifiMode]);
#ifdef ARDUINO_LOLIN_C3_MINI
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif // ARDUINO_LOLIN_C3_MINI
    break;
  default:
    break;
  }

  server.on("/", handleRoot);
  server.on("/index.htm", handleRoot);
  server.on("/configuration_site.htm", handleConfigurationSite);
  server.on("/configuration_speed.htm", handleConfigurationSpeed);
  server.on("/configuration_tracking.htm", handleConfigurationTracking);
  server.on("/configuration_mount.htm", handleConfigurationMount);
  server.on("/configuration_motors.htm", handleConfigurationMotors);
  server.on("/configuration_limits.htm", handleConfigurationLimits);
  server.on("/configuration_encoders.htm", handleConfigurationEncoders);
  server.on("/configuration_focuser.htm", handleConfigurationFocuser);
  server.on("/control.htm", handleControl);
  server.on("/control.txt", controlAjax);
  server.on("/guide.txt", guideAjax);
  server.on("/track.txt", trackAjax);
  server.on("/trackinfo.txt", trackinfoAjax);
  server.on("/wifi.htm", handleWifi);
  server.onNotFound(handleNotFound);

  cmdSvr.begin();
  cmdSvr.setNoDelay(true);
  server.begin();

#ifdef DEBUG_ON
  Ser.println("HTTP server started");
#endif
  //MDNS.begin(host);

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
  httpUpdater.setup(&server);
  httpServer.begin();
#endif
  initOTA();
};

void TeenAstroWifi::update()
{
  if (wifiOn == false)
    return;
  if ((activeWifiMode == WifiMode::M_Station1 ||
    activeWifiMode == WifiMode::M_Station2 ||
    activeWifiMode == WifiMode::M_Station3)
    && WiFi.status() != WL_CONNECTED)
  {
    return;
  }
  server.handleClient();

  ArduinoOTA.handle();

  // disconnect client
  static unsigned long clientTime = 0;
  switch (activeWifiConnectMode)
  {
  case WifiConnectMode::KeepOpened:
    if (cmdSvrClient && !cmdSvrClient.connected())
    {
      cmdSvrClient.stop();
    }
    // new client
    if (!cmdSvrClient && cmdSvr.hasClient())
    {
      cmdSvrClient = cmdSvr.available();
    }
    break;
  case WifiConnectMode::AutoClose:
  default:
    if (cmdSvrClient && (!cmdSvrClient.connected() || clientTime < millis()))
      cmdSvrClient.stop();
    // new client
    if (!cmdSvrClient && cmdSvr.hasClient()) {
      // find free/disconnected spot
      cmdSvrClient = cmdSvr.available();
      clientTime = millis() + 2000UL;
      break;
    }
    break;
  }


  // check clients for data, if found get the command, send cmd and pickup the response, then return the response
  while (cmdSvrClient.connected() && cmdSvrClient.available())
  {
    static char writeBuffer[50] = "";
    static int writeBufferPos = 0;
    while (cmdSvrClient.available())
    {
      // get the data
      byte b = cmdSvrClient.read();
      if (writeBufferPos == 0)
      {
        if (b == (char)6)
        {
          writeBuffer[0] = b;
          writeBuffer[1] = 0;
        }
        else if (b == ':')
        {
          writeBuffer[0] = b;
          writeBuffer[1] = 0;
          writeBufferPos++;
          server.handleClient();
          continue;
        }
        else
        {
          server.handleClient();
          continue;
        }
      }
      else if ((b == (char)32) || (b == (char)10) || (b == (char)13) || (b == (char)6))
      {
        server.handleClient();
        continue;
      }
      else
      {
        writeBuffer[writeBufferPos] = b;
        writeBufferPos++;
        if (writeBufferPos > 49)
        {
          writeBufferPos = 0;
          writeBuffer[writeBufferPos] = 0;
          server.handleClient();
          continue;
        }
        writeBuffer[writeBufferPos] = 0;
      }

      // send cmd and pickup the response
      if ((b == '#') || (b == (char)6))
      {
        char readBuffer[50] = "";
        CMDREPLY cmdreply;
        if (readLX200Bytes(writeBuffer, cmdreply, readBuffer, sizeof(readBuffer), CmdTimeout, true))
        {
          // return the response, if we have one
          if (strlen(readBuffer) > 0)
          {
            if (cmdSvrClient.connected())
            {
              cmdSvrClient.print(readBuffer);
            }
          }
        }
        writeBuffer[0] = 0;
        writeBufferPos = 0;
      }
      else server.handleClient();
    }
  }
}

bool TeenAstroWifi::isWifiOn()
{
  return wifiOn;
}

bool TeenAstroWifi::isWifiRunning()
{
  if (!wifiOn)
    return false;
  if (activeWifiMode == WifiMode::M_Station1 ||
      activeWifiMode == WifiMode::M_Station2 ||
      activeWifiMode == WifiMode::M_Station3)
    return WiFi.status() == WL_CONNECTED;
  return true;
}

void TeenAstroWifi::turnWifiOn(bool turnOn)
{
  wifiOn = turnOn;
  EEPROM.write(EEPROM_WifiOn, turnOn);
  EEPROM.commit();
}

void TeenAstroWifi::getIP(uint8_t* ip)
{
  IPAddress local;
  if (activeWifiMode == WifiMode::M_AcessPoint)
  {
    local = WiFi.softAPIP();
  }
  else
  {
    local = WiFi.localIP();
  }
  ip[0] = local[0];
  ip[1] = local[1];
  ip[2] = local[2];
  ip[3] = local[3];
}

const char* TeenAstroWifi::getPassword()
{
   return &masterPassword[0];
}

int TeenAstroWifi::getWifiMode()
{
  return activeWifiMode;
}

bool TeenAstroWifi::setWifiMode(int k)
{
  if (k < 0 || k>3)
    return false;
  activeWifiMode = static_cast<WifiMode>(k);
  EEPROM.write(EEPROM_WifiMode, k);
  EEPROM.commit();
  return true;
}

void TeenAstroWifi::getStationName(int k, char* SSID )
{
  memcpy(SSID, wifi_sta_ssid[k], 40U);
  return;
}

void TeenAstroWifi::initOTA()
{
  ArduinoOTA.onStart([]() {
    String type;
  if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
  }
  else {  // U_FS
    type = "filesystem";
  }

  // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  Serial.println("Start updating " + type);
    });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  if (error == OTA_AUTH_ERROR) {
    Serial.println("Auth Failed");
  }
  else if (error == OTA_BEGIN_ERROR) {
    Serial.println("Begin Failed");
  }
  else if (error == OTA_CONNECT_ERROR) {
    Serial.println("Connect Failed");
  }
  else if (error == OTA_RECEIVE_ERROR) {
    Serial.println("Receive Failed");
  }
  else if (error == OTA_END_ERROR) {
    Serial.println("End Failed");
  }
    });
  ArduinoOTA.begin();
}