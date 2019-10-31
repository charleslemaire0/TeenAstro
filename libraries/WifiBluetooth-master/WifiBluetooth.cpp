#include <TeenAstroLX200io.h>
#include "WifiBluetooth.h"


const char html_headB[] PROGMEM = "<!DOCTYPE HTML>\r\n<html>\r\n<head>\r\n";
const char html_headerIdx[] PROGMEM = "<meta http-equiv=\"refresh\" content=\"5; URL=/index.htm\">\r\n";
const char html_headE[] PROGMEM = "</head>\r\n";
const char html_bodyB[] PROGMEM = "<body bgcolor='#26262A'>\r\n";

const char html_main_cssB[] PROGMEM = "<STYLE>";
const char html_main_css1[] PROGMEM = ".clear { clear: both; } .a { background-color: #111111; } .t { padding: 10px 10px 20px 10px; border: 5px solid #551111;";
const char html_main_css2[] PROGMEM = " margin: 25px 25px 0px 25px; color: #999999; background-color: #111111; min-width: 10em; } input { font-weight: bold; width:6em; background-color: #A01010; padding: 2px 2px; }";
const char html_main_css3[] PROGMEM = ".b { padding: 10px; border-left: 5px solid #551111; border-right: 5px solid #551111; border-bottom: 5px solid #551111; margin: 0px 25px 25px 25px; color: #999999;";
const char html_main_css4[] PROGMEM = "background-color: #111111; min-width: 10em; } select { width:7em; font-weight: bold; background-color: #A01010; padding: 2px 2px; } .c { color: #A01010; font-weight: bold; }";
const char html_main_css5[] PROGMEM = "h1 { text-align: right; } a:hover, a:active { background-color: red; } .y { color: #FFFF00; font-weight: bold; }";
const char html_main_css6[] PROGMEM = "a:link, a:visited { background-color: #332222; color: #a07070; border:1px solid red; padding: 5px 10px;";
const char html_main_css7[] PROGMEM = " margin: none; text-align: center; text-decoration: none; display: inline-block; }";
const char html_main_css8[] PROGMEM = "button { background-color: #A01010; font-weight: bold; border-radius: 5px; margin: 2px; padding: 4px 8px; }";
const char html_main_css_control1[] PROGMEM = ".b1 { float: left; border: 2px solid #551111; background-color: #181818; text-align: center; margin: 5px; padding: 15px; padding-top: 3px; }";
const char html_main_css_control2[] PROGMEM = ".gb { width: 60px; height: 50px; padding: 0px; }";
const char html_main_css_control3[] PROGMEM = ".bb { height: 2.5em; width: 8em;} .bbh {   height: 2.5em; width: 4em;}";
const char html_main_cssE[] PROGMEM = "</STYLE>";

const char html_onstep_header1[] PROGMEM = "<div class='t'><table width='100%%'><tr><td><b><font size='5'>";
const char html_onstep_header2[] PROGMEM = "</font></b></td><td align='right'><b>" Product " " FirmwareVersionMajor"."FirmwareVersionMinor "."FirmwareVersionPatch" (TeenAstro ";
const char html_onstep_header3[] PROGMEM = ")</b></td></tr></table>";
const char html_onstep_header4[] PROGMEM = "</div><div class='b'>\r\n";

const char html_links1S[] PROGMEM = "<a href='/index.htm' style='background-color: #552222;'>Status</a>";
const char html_links1N[] PROGMEM = "<a href='/index.htm'>Status</a>";
const char html_links2S[] PROGMEM = "<a href='/control.htm' style='background-color: #552222;'>Control</a>";
const char html_links2N[] PROGMEM = "<a href='/control.htm'>Control</a>";
const char html_links3S[] PROGMEM = "<a href='/configuration_site.htm' style='background-color: #552222;'>Site</a>";
const char html_links3N[] PROGMEM = "<a href='/configuration_site.htm'>Site</a>";
const char html_links4S[] PROGMEM = "<a href='/configuration_telescope.htm' style='background-color: #552222;'>Telescope</a>";
const char html_links4N[] PROGMEM = "<a href='/configuration_telescope.htm'>Telescope</a>";
const char html_links5S[] PROGMEM = "<a href='/configuration_focuser.htm' style='background-color: #552222;'>Focuser</a>";
const char html_links5N[] PROGMEM = "<a href='/configuration_focuser.htm'>Focuser</a>";
const char html_links6S[] PROGMEM = "<a href='/wifi.htm' style='background-color: #552222;'>WiFi</a><br />";
const char html_links6N[] PROGMEM = "<a href='/wifi.htm'>WiFi</a><br />";


bool wifibluetooth::wifiOn = true;

int wifibluetooth::WebTimeout = TIMEOUT_WEB;
int wifibluetooth::CmdTimeout = TIMEOUT_CMD;

char wifibluetooth::masterPassword[40] = Default_Password;

wifibluetooth::WifiMode wifibluetooth::activeWifiMode = WifiMode::M_AcessPoint;
bool wifibluetooth::stationDhcpEnabled[3] = { true, true, true };

char wifibluetooth::wifi_sta_ssid[3][40] = { "", "", "" };
char wifibluetooth::wifi_sta_pwd[3][40] = { "", "", "" };

IPAddress wifibluetooth::wifi_sta_ip[3] = { IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1) };
IPAddress wifibluetooth::wifi_sta_gw[3] = { IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1) };
IPAddress wifibluetooth::wifi_sta_sn[3] = { IPAddress(255, 255, 255, 0), IPAddress(255, 255, 255, 0), IPAddress(255, 255, 255, 0) };

char wifibluetooth::wifi_ap_ssid[40] = "TeenAstro";
char wifibluetooth::wifi_ap_pwd[40] = "password";
byte wifibluetooth::wifi_ap_ch = 7;

IPAddress wifibluetooth::wifi_ap_ip = IPAddress(192, 168, 0, 1);
IPAddress wifibluetooth::wifi_ap_gw = IPAddress(192, 168, 0, 1);
IPAddress wifibluetooth::wifi_ap_sn = IPAddress(255, 255, 255, 0);

ESP8266WebServer wifibluetooth::server;
WiFiServer wifibluetooth::cmdSvr = WiFiServer(9999);
WiFiClient wifibluetooth::cmdSvrClient;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// -----------------------------------------------------------------------------------
// EEPROM related functions

// write int numbers into EEPROM at position i (2 bytes)
void wifibluetooth::EEPROM_writeInt(int i, int j) {
  uint8_t *k = (uint8_t*)&j;
  EEPROM.write(i + 0, *k); k++;
  EEPROM.write(i + 1, *k);
}

// read int numbers from EEPROM at position i (2 bytes)
int wifibluetooth::EEPROM_readInt(int i) {
  uint16_t j;
  uint8_t *k = (uint8_t*)&j;
  *k = EEPROM.read(i + 0); k++;
  *k = EEPROM.read(i + 1);
  return j;
}

// write 4 byte variable into EEPROM at position i (4 bytes)
void wifibluetooth::EEPROM_writeQuad(int i, byte *v) {
  EEPROM.write(i + 0, *v); v++;
  EEPROM.write(i + 1, *v); v++;
  EEPROM.write(i + 2, *v); v++;
  EEPROM.write(i + 3, *v);
}

// read 4 byte variable from EEPROM at position i (4 bytes)
void wifibluetooth::EEPROM_readQuad(int i, byte *v) {
  *v = EEPROM.read(i + 0); v++;
  *v = EEPROM.read(i + 1); v++;
  *v = EEPROM.read(i + 2); v++;
  *v = EEPROM.read(i + 3);
}

// write String into EEPROM at position i (40 bytes)
void wifibluetooth::EEPROM_writeString(int i, char l[]) {
  for (int l1 = 0; l1<40; l1++) {
    EEPROM.write(i + l1, *l); l++;
  }
}

// read String from EEPROM at position i (40 bytes)
void wifibluetooth::EEPROM_readString(int i, char l[]) {
  for (int l1 = 0; l1<40; l1++) {
    *l = EEPROM.read(i + l1); l++;
  }
}

// write 4 byte float into EEPROM at position i (4 bytes)
void wifibluetooth::EEPROM_writeFloat(int i, float f) {
  EEPROM_writeQuad(i, (byte*)&f);
}

// read 4 byte float from EEPROM at position i (4 bytes)
float wifibluetooth::EEPROM_readFloat(int i) {
  float f;
  EEPROM_readQuad(i, (byte*)&f);
  return f;
}

// write 4 byte long into EEPROM at position i (4 bytes)
void wifibluetooth::EEPROM_writeLong(int i, long l) {
  EEPROM_writeQuad(i, (byte*)&l);
}

// read 4 byte long from EEPROM at position i (4 bytes)
long wifibluetooth::EEPROM_readLong(int i) {
  long l;
  EEPROM_readQuad(i, (byte*)&l);
  return l;
}

bool wifibluetooth::atoi2(char *a, int *i) {
  char *conv_end;
  long l = strtol(a, &conv_end, 10);

  if ((l<-32767) || (l>32768) || (&a[0] == conv_end)) return false;
  *i = l;
  return true;
};

bool wifibluetooth::atof2(char *a, float *f) {
  char *conv_end;
  double l = strtof(a, &conv_end);

  if (&a[0] == conv_end) return false;
  *f = l;
  return true;
};

void wifibluetooth::handleNotFound()
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

void wifibluetooth::preparePage(String &data, int page)
{
  char temp1[80] = "";
  data = FPSTR(html_headB);
  if (page == 1)
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
  sendHtml(data);
  if (page == 2)
  {
    data += FPSTR(html_main_css_control1);
    data += FPSTR(html_main_css_control2);
    data += FPSTR(html_main_css_control3);
    sendHtml(data);
  }
  data += FPSTR(html_main_cssE);
  data += FPSTR(html_headE);
  data += FPSTR(html_bodyB);
  // get status
  if (!ta_MountStatus.validConnection()) ta_MountStatus.updateV();
  // finish the standard http response header
  data += FPSTR(html_onstep_header1);
  if (ta_MountStatus.validConnection()) data += ta_MountStatus.getVP(); else data += "Connection to TeenAstro Main unit is lost";
  data += FPSTR(html_onstep_header2);
  if (ta_MountStatus.validConnection()) data += ta_MountStatus.getVN(); else data += "?";
  data += FPSTR(html_onstep_header3);
  data += page == 1 ? FPSTR(html_links1S) : FPSTR(html_links1N);
  data += page == 2 ? FPSTR(html_links2S) : FPSTR(html_links2N);
  data += page == 3 ? FPSTR(html_links3S) : FPSTR(html_links3N);
  data += page == 4 ? FPSTR(html_links4S) : FPSTR(html_links4N);
  data += page == 5 ? FPSTR(html_links5S) : FPSTR(html_links5N);
#ifndef OETHS
  data += page == 6 ? FPSTR(html_links6S) : FPSTR(html_links6N);
#endif
  data += FPSTR(html_onstep_header4);
}

void wifibluetooth::writeStation2EEPROM(const int& k)
{
  unsigned int adress = EEPROM_start_wifi_sta + k * 100;
  EEPROM.write(adress, stationDhcpEnabled[k]); adress++;
  for (int i = 0; i < 4; i++, adress++)
    EEPROM.write(adress, wifi_sta_ip[k][i]);
  for (int i = 0; i < 4; i++, adress++)
    EEPROM.write(adress, wifi_sta_gw[k][i]);
  for (int i = 0; i < 4; i++, adress++)
    EEPROM.write(adress, wifi_sta_sn[k][i]);
  EEPROM_writeString(adress, wifi_sta_ssid[k]); adress += sizeof(wifi_sta_ssid[k]);
  EEPROM_writeString(adress, wifi_sta_pwd[k]);
}
void wifibluetooth::writeAccess2EEPROM()
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

void wifibluetooth::initFromEEPROM()
{
  EEPROM.begin(1024);
  unsigned int adress = EEPROM_start;
  // EEPROM Init
  if (EEPROM.read(adress) != 82 || EEPROM.read(adress + 1) != 66 ||
      EEPROM.read(adress + 2) != 0 || EEPROM.read(adress + 3) != 0)
  {
    EEPROM.write(adress, 82); adress++;
    EEPROM.write(adress, 66); adress++;
    EEPROM.write(adress, 0); adress++;
    EEPROM.write(adress, 0); adress++;
    EEPROM.write(EEPROM_WifiOn, wifiOn);
    EEPROM.write(EEPROM_WifiMode, activeWifiMode);
    EEPROM.write(EEPROM_WebTimeout, (uint8_t)WebTimeout);
    EEPROM.write(EEPROM_CmdTimeout, (uint8_t)CmdTimeout); 
    EEPROM_writeString(EPPROM_password, masterPassword);
    for (int k = 0; k < 3; k++)
    {
      writeStation2EEPROM(k);
    }
    writeAccess2EEPROM();
    EEPROM.commit();
  }
  else {
    wifiOn = EEPROM.read(EEPROM_WifiOn);
    activeWifiMode = static_cast<WifiMode>(EEPROM.read(EEPROM_WifiMode)); 
    WebTimeout = EEPROM.read(EEPROM_WebTimeout);
    CmdTimeout = EEPROM.read(EEPROM_CmdTimeout);
    EEPROM_readString(EPPROM_password, masterPassword);
    for (int k = 0; k < 3; k++)
    {
      adress = EEPROM_start_wifi_sta + k * 100;
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

void wifibluetooth::setup()
{
  initFromEEPROM();

#ifndef DEBUG_ON


  byte tb = 0;
Again:

  char c = 0;

  // safety net
  if ((c == 'R') || activeWifiMode == WifiMode::OFF) {
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
  case wifibluetooth::M_AcessPoint:
    WiFi.softAPConfig(wifi_ap_ip, wifi_ap_gw, wifi_ap_sn);
    WiFi.softAP(wifi_ap_ssid, wifi_ap_pwd, wifi_ap_ch);
    WiFi.mode(WIFI_AP);
    break;
  case wifibluetooth::M_Station1:
  case wifibluetooth::M_Station2:
  case wifibluetooth::M_Station3:
    if (!stationDhcpEnabled[activeWifiMode])
    {
      WiFi.config(wifi_sta_ip[activeWifiMode], wifi_sta_gw[activeWifiMode], wifi_sta_sn[activeWifiMode]);
    }
    WiFi.softAPdisconnect(true);
    WiFi.begin(wifi_sta_ssid[activeWifiMode], wifi_sta_pwd[activeWifiMode]);
    WiFi.mode(WIFI_STA);
  default:
    break;
  }

  server.on("/", handleRoot);
  server.on("/index.htm", handleRoot);
  server.on("/configuration_site.htm", handleConfigurationSite);
  server.on("/configuration_telescope.htm", handleConfigurationTelescope);
  server.on("/configuration_focuser.htm", handleConfigurationFocuser);
  server.on("/control.htm", handleControl);
  server.on("/control.txt", controlAjax);
  server.on("/guide.txt", guideAjax);
  server.on("/wifi.htm", handleWifi);
  server.onNotFound(handleNotFound);

  cmdSvr.begin();
  cmdSvr.setNoDelay(true);
  server.begin();

#ifdef DEBUG_ON
  Ser.println("HTTP server started");
#endif
  //MDNS.begin(host);

  httpUpdater.setup(&server);
  httpServer.begin();
};

void wifibluetooth::update()
{
  if ((activeWifiMode == WifiMode::M_Station1 ||
       activeWifiMode == WifiMode::M_Station2 ||
       activeWifiMode == WifiMode::M_Station3)
        && WiFi.status() != WL_CONNECTED)
  {
    return;
  }
  server.handleClient();

  // disconnect client
  static unsigned long clientTime = 0;
  if (cmdSvrClient && (!cmdSvrClient.connected()||clientTime < millis()))
    cmdSvrClient.stop();

  // new client
  if (!cmdSvrClient && cmdSvr.hasClient()) {
    // find free/disconnected spot
    cmdSvrClient = cmdSvr.available();
    clientTime = millis() + 2000UL;
  }

  static char writeBuffer[50] = "";
  static int writeBufferPos = 0;
  // check clients for data, if found get the command, send cmd and pickup the response, then return the response
  while (cmdSvrClient && cmdSvrClient.connected() && (cmdSvrClient.available() > 0)) {
    // get the data
    byte b = cmdSvrClient.read();
    if (writeBufferPos == 0 && b != ':')
      continue;
    writeBuffer[writeBufferPos] = b;
    writeBufferPos++;
    if (writeBufferPos > 49)
    {
      writeBufferPos = 0;
      writeBuffer[writeBufferPos] = 0;
      continue;
    }
    writeBuffer[writeBufferPos] = 0;
    // send cmd and pickup the response
    if ((b == '#') || ((strlen(writeBuffer) == 1) && (b == (char)6))) {
      char readBuffer[50] = "";
      if (readLX200Bytes(writeBuffer, readBuffer, sizeof(readBuffer), CmdTimeout, true))
      {
        // return the response, if we have one
        if (strlen(readBuffer) > 0)
        {
          if (cmdSvrClient && cmdSvrClient.connected()) {
            cmdSvrClient.print(readBuffer);
            delay(2);
          }
        }
      }
      writeBuffer[0] = 0;
      writeBufferPos = 0;
    }
    else server.handleClient();
  }
}

bool wifibluetooth::isWifiOn()
{
  return wifiOn;
}

bool wifibluetooth::isWifiRunning()
{
  if (!wifiOn)
    return false;
  if (activeWifiMode == WifiMode::M_Station1 ||
      activeWifiMode == WifiMode::M_Station2 ||
      activeWifiMode == WifiMode::M_Station3)
    return WiFi.status() == WL_CONNECTED;
  return true;
}

void wifibluetooth::turnWifiOn(bool turnOn)
{
  wifiOn = turnOn;
  EEPROM.write(EEPROM_WifiOn, turnOn);
  EEPROM.commit();
}

void wifibluetooth::getIP(uint8_t* ip)
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

const char* wifibluetooth::getPassword()
{
   return &masterPassword[0];
}

int wifibluetooth::getWifiMode()
{
  return activeWifiMode;
}

bool wifibluetooth::setWifiMode(int k)
{
  if (k < 0 || k>3)
    return false;
  activeWifiMode = static_cast<WifiMode>(k);
  EEPROM.write(EEPROM_WifiMode, k);
  EEPROM.commit();
  return true;
}

void wifibluetooth::getStationName(int k, char* SSID )
{
  memcpy(SSID, wifi_sta_ssid[k], 40U);
  return;
}

