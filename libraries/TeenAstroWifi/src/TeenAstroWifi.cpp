#include <ArduinoOTA.h>
#include "TeenAstroWifi.h"


const char html_headB[] PROGMEM = "<!DOCTYPE HTML>\r\n<html lang='en'>\r\n<head>\r\n"
"<meta charset='UTF-8'>\r\n"
"<meta name='viewport' content='width=device-width,initial-scale=1'>\r\n";
const char html_headerIdx[] PROGMEM = "<meta http-equiv=\"refresh\" content=\"5; URL=/index.htm\">\r\n";
const char html_headE[] PROGMEM = "</head>\r\n";
const char html_bodyB[] PROGMEM = "<body>\r\n";

// ---- Modern consolidated CSS with CSS custom properties ----
const char html_main_css1[] PROGMEM = "<style>\n"
":root{"
"--bg:#0d1117;--bg2:#161b22;--bg3:#1c2128;--border:#30363d;"
"--accent:#c9453a;--accent-h:#e05544;--accent-bg:rgba(201,69,58,.12);"
"--text:#c9d1d9;--text2:#8b949e;--text-hi:#f0f6fc;"
"--nav:#21262d;--nav-sel:#c9453a;--nav-text:#c9d1d9;"
"--card:#161b22;--card-bd:#30363d;"
"--radius:8px;--shadow:0 2px 8px rgba(0,0,0,.3);"
"}\n";

const char html_main_css2[] PROGMEM =
"*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Helvetica,Arial,sans-serif;"
"background:var(--bg);color:var(--text);font-size:14px;line-height:1.6;padding:0}\n";

const char html_main_css3[] PROGMEM =
".hdr{background:var(--bg2);border-bottom:1px solid var(--border);"
"padding:12px 20px;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:8px}\n"
".hdr-title{font-size:1.3em;font-weight:700;color:var(--text-hi)}\n"
".hdr-info{font-size:.85em;color:var(--text2);text-align:right}\n";

const char html_main_css4[] PROGMEM =
"nav{background:var(--nav);padding:6px 12px;display:flex;flex-wrap:wrap;gap:4px;"
"border-bottom:1px solid var(--border)}\n"
"nav a{color:var(--nav-text);text-decoration:none;padding:6px 14px;border-radius:6px;"
"font-size:.85em;font-weight:500;transition:background .15s,color .15s}\n"
"nav a:link,nav a:visited{color:var(--nav-text);background:transparent;border:none;"
"display:inline-block;text-align:center}\n"
"nav a:hover,nav a:active{background:var(--accent-bg);color:var(--accent-h)}\n"
"nav a.sel{background:var(--nav-sel);color:#fff;font-weight:600}\n";

const char html_main_css5[] PROGMEM =
".content{max-width:900px;margin:20px auto;padding:0 16px}\n"
".bt{font-size:1.1em;font-weight:600;color:var(--text-hi);margin:18px 0 8px;padding-bottom:4px;"
"border-bottom:1px solid var(--border)}\n"
".card{background:var(--card);border:1px solid var(--card-bd);border-radius:var(--radius);"
"padding:16px;margin-bottom:12px;box-shadow:var(--shadow)}\n";

const char html_main_css6[] PROGMEM =
"form{margin:6px 0}"
"input[type=number],input[type=text]{background:var(--bg3);border:1px solid var(--border);"
"color:var(--text-hi);padding:6px 10px;border-radius:var(--radius);font-size:.9em;"
"width:7em;font-weight:600;outline:none;transition:border-color .15s}\n"
"input:focus{border-color:var(--accent)}\n"
"select{background:var(--bg3);border:1px solid var(--border);color:var(--text-hi);"
"padding:6px 10px;border-radius:var(--radius);font-size:.9em;font-weight:600;"
"width:auto;min-width:7em;outline:none}\n"
"select:focus{border-color:var(--accent)}\n";

const char html_main_css7[] PROGMEM =
"button{background:var(--accent);color:#fff;font-weight:600;border:none;"
"border-radius:var(--radius);padding:7px 16px;font-size:.85em;cursor:pointer;"
"transition:background .15s,transform .1s}\n"
"button:hover{background:var(--accent-h)}\n"
"button:active{transform:scale(.97)}\n"
".c{color:var(--accent-h);font-weight:700}\n"
".y{color:#f0c040;font-weight:700}\n";

const char html_main_css_control1[] PROGMEM =
".panel{background:var(--card);border:1px solid var(--card-bd);"
"border-radius:var(--radius);padding:16px;margin:8px;box-shadow:var(--shadow);"
"display:inline-block;vertical-align:top;text-align:center}\n"
".panels{display:flex;flex-wrap:wrap;gap:8px;justify-content:center}\n"
".panel-title{font-size:1.1em;font-weight:600;color:var(--text-hi);margin-bottom:10px;"
"text-align:left;padding-bottom:6px;border-bottom:1px solid var(--border)}\n";

const char html_main_css_control2[] PROGMEM =
".gb{width:64px;height:52px;font-size:1.1em;padding:0;margin:3px;"
"background:var(--bg3);color:var(--text-hi);border:1px solid var(--border);"
"border-radius:var(--radius);cursor:pointer;transition:background .15s}\n"
".gb:hover{background:var(--accent-bg);border-color:var(--accent)}\n"
".gb:active{background:var(--accent);color:#fff}\n";

const char html_main_css_control3[] PROGMEM =
".bb{height:2.5em;min-width:8em;margin:3px}\n"
".bbh{height:2.5em;min-width:6em;margin:3px}\n";

const char html_main_css_control4[] PROGMEM =
"@media(max-width:600px){"
".content{padding:0 8px;margin:10px auto}"
".hdr{padding:8px 12px}"
"nav{padding:4px 8px;gap:2px}"
"nav a{padding:5px 10px;font-size:.8em}"
".panel{margin:4px;padding:12px;min-width:0;width:100%}"
".gb{width:56px;height:46px}"
"}\n</style>\n";

const char html_header1[] PROGMEM = "<div class='hdr'><span class='hdr-title'>\n";
const char html_header2[] PROGMEM = "</span><span class='hdr-info'>" Product " " ServerFirmwareVersionMajor "." ServerFirmwareVersionMinor "." ServerFirmwareVersionPatch "<br>Main Unit \n";
const char html_header3[] PROGMEM = "</span></div>\n";
const char html_header4[] PROGMEM = "</nav><div class='content'>\r\n";

const char html_links1S[] PROGMEM = "<a class='sel' href='/index.htm'>Status</a>\n";
const char html_links1N[] PROGMEM = "<a href='/index.htm'>Status</a>\n";
const char html_links2S[] PROGMEM = "<a class='sel' href='/control.htm'>Control</a>\n";
const char html_links2N[] PROGMEM = "<a href='/control.htm'>Control</a>\n";
const char html_links3S[] PROGMEM = "<a class='sel' href='/configuration_speed.htm'>Speed</a>\n";
const char html_links3N[] PROGMEM = "<a href='/configuration_speed.htm'>Speed</a>\n";
const char html_links4S[] PROGMEM = "<a class='sel' href='/configuration_tracking.htm'>Tracking</a>\n";
const char html_links4N[] PROGMEM = "<a href='/configuration_tracking.htm'>Tracking</a>\n";
const char html_links5S[] PROGMEM = "<a class='sel' href='/configuration_site.htm'>Site</a>\n";
const char html_links5N[] PROGMEM = "<a href='/configuration_site.htm'>Site</a>\n";
const char html_links6S[] PROGMEM = "<a class='sel' href='/configuration_mount.htm'>Mount</a>\n";
const char html_links6N[] PROGMEM = "<a href='/configuration_mount.htm'>Mount</a>\n";
const char html_links7S[] PROGMEM = "<a class='sel' href='/configuration_motors.htm'>Motors</a>\n";
const char html_links7N[] PROGMEM = "<a href='/configuration_motors.htm'>Motors</a>\n";
const char html_links8S[] PROGMEM = "<a class='sel' href='/configuration_limits.htm'>Limits</a>\n";
const char html_links8N[] PROGMEM = "<a href='/configuration_limits.htm'>Limits</a>\n";
const char html_links9S[] PROGMEM = "<a class='sel' href='/configuration_encoders.htm'>Encoders</a>\n";
const char html_links9N[] PROGMEM = "<a href='/configuration_encoders.htm'>Encoders</a>\n";
const char html_links10S[] PROGMEM = "<a class='sel' href='/configuration_focuser.htm'>Focuser</a>\n";
const char html_links10N[] PROGMEM = "<a href='/configuration_focuser.htm'>Focuser</a>\n";
const char html_links11S[] PROGMEM = "<a class='sel' href='/wifi.htm'>WiFi</a>\n";
const char html_links11N[] PROGMEM = "<a href='/wifi.htm'>WiFi</a>\n";

LX200Client* TeenAstroWifi::s_client = nullptr;
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
  data = FPSTR(html_headB);
  if (!ta_MountStatus.hasFocuser() && page == ServerPage::Focuser)
    page = ServerPage::Index;
  else if (!ta_MountStatus.encodersEnable() && page == ServerPage::Encoders)
    page = ServerPage::Index;

  if (page == ServerPage::Index)
    data += FPSTR(html_headerIdx);

  // CSS
  data += FPSTR(html_main_css1);
  data += FPSTR(html_main_css2);
  data += FPSTR(html_main_css3);
  data += FPSTR(html_main_css4);
  data += FPSTR(html_main_css5);
  sendHtml(data);
  data += FPSTR(html_main_css6);
  data += FPSTR(html_main_css7);
  if (page == ServerPage::Control ||
      page == ServerPage::Tracking ||
      page == ServerPage::Site)
  {
    data += FPSTR(html_main_css_control1);
    data += FPSTR(html_main_css_control2);
    data += FPSTR(html_main_css_control3);
  }
  data += FPSTR(html_main_css_control4);  // responsive rules (always)
  sendHtml(data);
  data += FPSTR(html_headE);
  data += FPSTR(html_bodyB);

  // Header bar
  data += FPSTR(html_header1);
  if (ta_MountStatus.hasInfoV()) data += ta_MountStatus.getVP(); else data += "Connection lost";
  data += FPSTR(html_header2);
  if (ta_MountStatus.hasInfoV())
  {
    data += ta_MountStatus.getVN();
    data += " &middot; Board ";
    data += ta_MountStatus.getVB();
    data += " &middot; ";
    switch (ta_MountStatus.getVb()[0])
    {
    default:
    case '0': data += "Generic"; break;
    case '1': data += "TOS100"; break;
    case '2': data += "TMC2130"; break;
    case '3': data += "TMC5160"; break;
    case '4': data += "TMC2160"; break;
    }
  }
  else data += "?";
  data += FPSTR(html_header3);

  // Navigation
  data += "<nav>\n";
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
    data += page == ServerPage::Motors ? FPSTR(html_links7S) : FPSTR(html_links7N);
  data += page == ServerPage::Limits ? FPSTR(html_links8S) : FPSTR(html_links8N);
  if (ta_MountStatus.encodersEnable())
    data += page == ServerPage::Encoders ? FPSTR(html_links9S) : FPSTR(html_links9N);
  if (ta_MountStatus.hasFocuser())
    data += page == ServerPage::Focuser ? FPSTR(html_links10S) : FPSTR(html_links10N);
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
        if (s_client->sendReceiveAuto(writeBuffer, cmdreply, readBuffer, sizeof(readBuffer), CmdTimeout, true))
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