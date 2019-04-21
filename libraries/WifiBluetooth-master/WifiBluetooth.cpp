#include "WifiBluetooth.h"
#include "config.h"


#define html_headB  "<!DOCTYPE HTML>\r\n<html>\r\n<head>\r\n"
#define html_headerIdx  "<meta http-equiv=\"refresh\" content=\"5; URL=/index.htm\">\r\n"
#define html_headE "</head>\r\n"
#define html_bodyB  "<body bgcolor='#26262A'>\r\n"

#define html_main_cssB "<STYLE>"
#define html_main_css1  ".clear { clear: both; } .a { background-color: #111111; } .t { padding: 10px 10px 20px 10px; border: 5px solid #551111;"
#define html_main_css2  " margin: 25px 25px 0px 25px; color: #999999; background-color: #111111; min-width: 10em; } input { font-weight: bold; width:6em; background-color: #A01010; padding: 2px 2px; }"
#define html_main_css3  ".b { padding: 10px; border-left: 5px solid #551111; border-right: 5px solid #551111; border-bottom: 5px solid #551111; margin: 0px 25px 25px 25px; color: #999999;"
#define html_main_css4  "background-color: #111111; min-width: 10em; } select { width:7em; font-weight: bold; background-color: #A01010; padding: 2px 2px; } .c { color: #A01010; font-weight: bold; }"
#define html_main_css5  "h1 { text-align: right; } a:hover, a:active { background-color: red; } .y { color: #FFFF00; font-weight: bold; }"
#define html_main_css6  "a:link, a:visited { background-color: #332222; color: #a07070; border:1px solid red; padding: 5px 10px;"
#define html_main_css7  " margin: none; text-align: center; text-decoration: none; display: inline-block; }"
#define html_main_css8  "button { background-color: #A01010; font-weight: bold; border-radius: 5px; margin: 2px; padding: 4px 8px; }"
#define html_main_css_control1 ".b1 { float: left; border: 2px solid #551111; background-color: #181818; text-align: center; margin: 5px; padding: 15px; padding-top: 3px; }"
#define html_main_css_control2 ".gb { width: 60px; height: 50px; padding: 0px; }"
#define html_main_css_control3 ".bb { height: 2.5em; width: 8em;} .bbh {   height: 2.5em; width: 4em;}"
#define html_main_cssE  "</STYLE>"

#define html_onstep_header1 "<div class='t'><table width='100%%'><tr><td><b><font size='5'>"
#define html_onstep_header2 "</font></b></td><td align='right'><b>" Product " " FirmwareVersionMajor"."FirmwareVersionMinor "."FirmwareVersionPatch" (TeenAstro "
#define html_onstep_header3 ")</b></td></tr></table>"
#define html_onstep_header4 "</div><div class='b'>\r\n"

#define html_links1S "<a href='/index.htm' style='background-color: #552222;'>Status</a>"
#define html_links1N "<a href='/index.htm'>Status</a>"
#define html_links2S "<a href='/control.htm' style='background-color: #552222;'>Control</a>"
#define html_links2N "<a href='/control.htm'>Control</a>"
#define html_links3S "<a href='/configuration_site.htm' style='background-color: #552222;'>Site</a>"
#define html_links3N "<a href='/configuration_site.htm'>Site</a>"
#define html_links4S "<a href='/configuration_telescope.htm' style='background-color: #552222;'>Telescope</a>"
#define html_links4N "<a href='/configuration_telescope.htm'>Telescope</a>"
#define html_links5S "<a href='/configuration_focuser.htm' style='background-color: #552222;'>Focuser</a>"
#define html_links5N "<a href='/configuration_focuser.htm'>Focuser</a>"
#define html_links6S "<a href='/wifi.htm' style='background-color: #552222;'>WiFi</a><br />"
#define html_links6N "<a href='/wifi.htm'>WiFi</a><br />"


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


const char* wifibluetooth::HighSpeedCommsStr(long baud)
{
  if (baud == 115200) { return ":SB0#"; }
  if (baud == 57600) { return ":SB1#"; }
  if (baud == 38400) { return ":SB2#"; }
  if (baud == 28800) { return ":SB3#"; }
  if (baud == 19200) { return ":SB4#"; }
  else { return ":SB5#"; }
}

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
  data = html_headB;
  if (page == 1)
    data += html_headerIdx;
  data += html_main_cssB;
  data += html_main_css1;
  data += html_main_css2;
  data += html_main_css3;
  data += html_main_css4;
  data += html_main_css5;
  data += html_main_css6;
  data += html_main_css7;
  data += html_main_css8;
  sendHtml(data);
  if (page == 2)
  {
    data += html_main_css_control1;
    data += html_main_css_control2;
    data += html_main_css_control3;
    sendHtml(data);
  }
  data += html_main_cssE;
  data += html_headE;
#ifdef OETHS
  client->print(data); data = "";
#endif

  data += html_bodyB;
  // get status
  if (!ta_MountStatus.validConnection()) ta_MountStatus.updateV();
  // finish the standard http response header
  data += html_onstep_header1;
  if (ta_MountStatus.validConnection()) data += ta_MountStatus.getVP(); else data += "Connection to TeenAstro Main unit is lost";
  data += html_onstep_header2;
  if (ta_MountStatus.validConnection()) data += ta_MountStatus.getVN(); else data += "?";
  data += html_onstep_header3;
  data += page == 1 ? html_links1S : html_links1N;
  data += page == 2 ? html_links2S : html_links2N;
  data += page == 3 ? html_links3S : html_links3N;
  data += page == 4 ? html_links4S : html_links4N;
  data += page == 5 ? html_links5S : html_links5N;
#ifndef OETHS
  data += page == 6 ? html_links6S : html_links6N;
#endif
  data += html_onstep_header4;
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

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
#endif

  initFromEEPROM();

#ifndef DEBUG_ON
  Ser.begin(SERIAL_BAUD);

  byte tb = 0;
Again:

  char c = 0;

  // clear the buffers and any noise on the serial lines
  for (int i = 0; i < 3; i++) {
    Ser.print(":#");

    delay(100);
    Ser.flush();
    c = serialRecvFlush();

    delay(100);
  }

  // safety net
  if ((c == 'R') || activeWifiMode == WifiMode::OFF) {
    // reset EEPROM values, triggers an init
    EEPROM_writeInt(0, 0); EEPROM_writeInt(2, 0);
    activeWifiMode = WifiMode::M_AcessPoint;
    EEPROM.commit();
    Ser.println();
    Ser.println("Cycle power for reset to defaults.");
    Ser.println();
  }


  // switch OnStep Serial1 up to ? baud
  Ser.print(HighSpeedCommsStr(SERIAL_BAUD));
  delay(100);
  int count = 0; c = 0;
  while (Ser.available() > 0) { count++; if (count == 1) c = Ser.read(); }
  if (c == '1') {
    Ser.begin(SERIAL_BAUD);


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


  // clear the buffers and any noise on the serial lines
  for (int i = 0; i < 3; i++) {
    Ser.print(":#");
    delay(50);
    serialRecvFlush();
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
  //encoders.init();
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
  if (cmdSvrClient && (!cmdSvrClient.connected())) cmdSvrClient.stop();
  if (cmdSvrClient && ((long)(clientTime - millis()) < 0)) cmdSvrClient.stop();

  // new client
  if (!cmdSvrClient && (cmdSvr.hasClient())) {
    // find free/disconnected spot
    cmdSvrClient = cmdSvr.available();
    clientTime = millis() + 2000UL;
  }

  static char writeBuffer[40] = "";
  static int writeBufferPos = 0;
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

  // encoders.poll();

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
  //if (k < 0 || k>2)
  //  return;
  memcpy(SSID, wifi_sta_ssid[k], 40U);
  return;
}

