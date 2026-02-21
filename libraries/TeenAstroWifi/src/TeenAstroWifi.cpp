#include "TeenAstroWifi.h"


const char html_headB[] PROGMEM = "<!DOCTYPE HTML>\r\n<html lang='en'>\r\n<head>\r\n"
"<meta charset='UTF-8'>\r\n"
"<meta name='viewport' content='width=device-width,initial-scale=1'>\r\n";
const char html_headerIdx[] PROGMEM = "<meta http-equiv=\"refresh\" content=\"5; URL=/index.htm\">\r\n";
const char html_headE[] PROGMEM = "</head>\r\n";
const char html_bodyB[] PROGMEM = "<body>\r\n";

// Navigation guard: disables nav links after a click to prevent rapid page loads
const char html_navGuard[] PROGMEM =
"<script>\n"
"document.addEventListener('click',function(e){\n"
"var a=e.target.closest('nav a');\n"
"if(!a||a.classList.contains('sel'))return;\n"
"if(window._navBusy){e.preventDefault();return;}\n"
"window._navBusy=true;\n"
"var links=document.querySelectorAll('nav a');\n"
"for(var i=0;i<links.length;i++){links[i].style.opacity='0.4';links[i].style.pointerEvents='none';}\n"
"a.textContent=a.textContent+' ...';\n"
"});\n"
"</script>\n";

// #region agent log
const char html_debugPageLog[] PROGMEM =
  "<script>document.addEventListener('DOMContentLoaded',function(){"
  "var h=document.getElementById('_dbg');"
  "var hp=h?parseInt(h.textContent):0;"
  "var l=document.body?document.body.innerHTML.length:0;"
  "fetch('http://127.0.0.1:7242/ingest/22318eea-23d2-4990-b49b-089aaf334f55',"
  "{method:'POST',mode:'no-cors',headers:{'Content-Type':'text/plain'},"
  "body:JSON.stringify({location:'page',message:'loaded',"
  "data:{url:location.href,bodyLen:l,heap:hp},"
  "timestamp:Date.now(),hypothesisId:'B'})}).catch(function(){});"
  "});</script>\n";
// #endregion

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
"border-bottom:1px solid var(--border);align-items:center}\n"
"nav a{color:var(--nav-text);text-decoration:none;padding:8px 14px;border-radius:6px;"
"font-size:.85em;font-weight:500;transition:background .15s,color .15s;"
"min-height:44px;display:inline-flex;align-items:center}\n"
"nav a:link,nav a:visited{color:var(--nav-text);background:transparent;border:none;"
"text-align:center}\n"
"nav a:hover,nav a:active{background:var(--accent-bg);color:var(--accent-h)}\n"
"nav a.sel{background:var(--nav-sel);color:#fff;font-weight:600}\n"
"#navtog{display:none}\n"
".hamburger{display:none;cursor:pointer;padding:8px;font-size:1.5em;color:var(--nav-text);"
"min-height:44px;align-items:center}\n";

const char html_main_css5[] PROGMEM =
".content{max-width:900px;margin:20px auto;padding:0 16px}\n"
".bt{font-size:1.1em;font-weight:600;color:var(--text-hi);margin:18px 0 8px;padding-bottom:4px;"
"border-bottom:1px solid var(--border)}\n"
".card{background:var(--card);border:1px solid var(--card-bd);border-radius:var(--radius);"
"padding:16px;margin-bottom:12px;box-shadow:var(--shadow)}\n";

const char html_main_css6[] PROGMEM =
"form{margin:6px 0}"
"input[type=number],input[type=text]{background:var(--bg3);border:1px solid var(--border);"
"color:var(--text-hi);padding:8px 10px;border-radius:var(--radius);font-size:16px;"
"width:7em;font-weight:600;outline:none;transition:border-color .15s;min-height:44px}\n"
"input:focus{border-color:var(--accent)}\n"
"select{background:var(--bg3);border:1px solid var(--border);color:var(--text-hi);"
"padding:8px 10px;border-radius:var(--radius);font-size:16px;font-weight:600;"
"width:auto;min-width:7em;outline:none;min-height:44px}\n"
"select:focus{border-color:var(--accent)}\n";

const char html_main_css7[] PROGMEM =
"button{background:var(--accent);color:#fff;font-weight:600;border:none;"
"border-radius:var(--radius);padding:8px 16px;font-size:.9em;cursor:pointer;"
"transition:background .15s,transform .1s;min-height:44px}\n"
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
".bb{min-height:44px;min-width:8em;margin:3px}\n"
".bbh{min-height:44px;min-width:6em;margin:3px}\n";

const char html_main_css_control4[] PROGMEM =
"@media(max-width:600px){"
".content{padding:0 8px;margin:10px auto}"
".hdr{padding:8px 12px;flex-direction:column;text-align:center}"
".hdr-info{text-align:center}"
".hamburger{display:flex}"
"nav{padding:0;flex-direction:column;gap:0}"
"nav a{display:none;padding:10px 16px;font-size:.95em;width:100%;"
"border-radius:0;border-bottom:1px solid var(--border)}"
"#navtog:checked~a{display:flex}"
"nav a.sel{display:flex}"
"input[type=number],input[type=text]{width:100%;max-width:100%}"
"select{width:100%;max-width:100%}"
".panel{margin:4px;padding:12px;min-width:0;width:100%}"
".bb,.bbh{width:100%;min-height:48px}"
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
bool TeenAstroWifi::s_handlerBusy = false;
unsigned long TeenAstroWifi::s_lastPageMs = 0;

// Minimum interval between full page loads (ms).
// If a new page is requested within this cooldown, return a lightweight
// "please wait" that auto-retries after 1 second.  This prevents rapid
// clicks from queueing heavy serial-IO handlers back to back.
#define PAGE_COOLDOWN_MS 800

bool TeenAstroWifi::busyGuard()
{
  unsigned long now = millis();
  if (s_handlerBusy || (now - s_lastPageMs < PAGE_COOLDOWN_MS))
  {
    // #region agent log
    unsigned long cd = now - s_lastPageMs;
    uint32_t heap = ESP.getFreeHeap();
    String html;
    html.reserve(750);
    html = F("<!DOCTYPE HTML><html><head>"
      "<meta http-equiv='refresh' content='1'>"
      "</head><body style='background:#0d1117;color:#c9d1d9;"
      "display:flex;justify-content:center;align-items:center;height:90vh;"
      "font-family:sans-serif'>"
      "<div style='text-align:center'>"
      "<div style='font-size:1.5em;margin-bottom:8px'>&#8987;</div>"
      "Loading...<br><small style='color:#8b949e'>heap:");
    html += heap;
    html += F(" busy:");
    html += (int)s_handlerBusy;
    html += F(" cd:");
    html += cd;
    html += F("ms</small></div>"
      "<script>fetch('http://127.0.0.1:7242/ingest/22318eea-23d2-4990-b49b-089aaf334f55',"
      "{method:'POST',mode:'no-cors',headers:{'Content-Type':'text/plain'},"
      "body:JSON.stringify({location:'busyGuard',message:'wait_page',"
      "data:{url:location.href,heap:");
    html += heap;
    html += F(",busy:");
    html += (int)s_handlerBusy;
    html += F(",cd:");
    html += cd;
    html += F("},timestamp:Date.now(),hypothesisId:'A'})}).catch(function(){});</script>"
      "</body></html>");
    server.send(200, "text/html", html);
    // #endregion
    return true;
  }
  s_handlerBusy = true;
  s_lastPageMs = now;
  return false;
}

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
ESP8266HTTPUpdateServer httpUpdater;
#endif 

#ifdef ARDUINO_ARCH_ESP32
WebServer TeenAstroWifi::server;
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
  data += FPSTR(html_navGuard);
  // #region agent log
  data += FPSTR(html_debugPageLog);
  // #endregion
  data += FPSTR(html_headE);
  data += FPSTR(html_bodyB);
  // #region agent log
  data += "<span id='_dbg' style='display:none'>";
  data += ESP.getFreeHeap();
  data += "</span>";
  // #endregion

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

  // Navigation with hamburger toggle for mobile
  data += "<nav>\n";
  data += "<label class='hamburger' for='navtog'>&#9776;</label>\n";
  data += "<input type='checkbox' id='navtog'>\n";
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

  // HTTP OTA: register /update route on the main web server
  httpUpdater.setup(&server);
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
      // Idle timeout must exceed both the poll interval (~2 s) and the serial CmdTimeout (up to 8 s)
      clientTime = millis() + (unsigned long)(CmdTimeout + 2) * 1000UL;
      break;
    }
    break;
  }


  // Refresh all-state cache every 500 ms so :GXAS# requests from the app
  // can be served without an additional serial round-trip to the MainUnit.
  ta_MountStatus.updateAllState();

  // Process at most ONE complete command from the TCP client per update() cycle.
  // This prevents the TCP channel from monopolising the serial port and ensures
  // the SHC display, buttons, and HTTP server all get their turns.
  // IMPORTANT: server.handleClient() must NOT be called from inside this loop
  // to avoid re-entrant serial access (HTTP handlers also use s_client).
  {
    static char writeBuffer[50] = "";
    static int writeBufferPos = 0;
    bool cmdDone = false;

    while (cmdSvrClient.connected() && cmdSvrClient.available() && !cmdDone)
    {
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
          continue;
        }
        else
        {
          continue;
        }
      }
      else if ((b == (char)32) || (b == (char)10) || (b == (char)13) || (b == (char)6))
      {
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
          continue;
        }
        writeBuffer[writeBufferPos] = 0;
      }

      // send cmd and pickup the response
      if ((b == '#') || (b == (char)6))
      {
        // :GXAS# — serve from all-state cache when fresh (< 500 ms old).
        if (strcmp(writeBuffer, ":GXAS#") == 0 && !ta_MountStatus.allStateCacheStale())
        {
          if (cmdSvrClient.connected())
            cmdSvrClient.print(ta_MountStatus.getAllStateB64Cached());
          writeBuffer[0] = 0;
          writeBufferPos = 0;
          cmdDone = true;
          if (activeWifiConnectMode == WifiConnectMode::AutoClose)
            clientTime = millis() + (unsigned long)(CmdTimeout + 2) * 1000UL;
          break;
        }

        // :GXAS# response is 64 base64 chars + '#' = 65 chars — use a larger
        // read buffer than the default to avoid truncation.
        char readBuffer[130] = "";
        CMDREPLY cmdreply;
        if (s_client->sendReceiveAuto(writeBuffer, cmdreply, readBuffer, sizeof(readBuffer), CmdTimeout, true))
        {
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
        cmdDone = true;  // exit after one complete command
        // In AutoClose mode, refresh idle timeout after every processed command.
        // Must exceed CmdTimeout (serial round-trip) + poll interval to avoid race.
        if (activeWifiConnectMode == WifiConnectMode::AutoClose)
          clientTime = millis() + (unsigned long)(CmdTimeout + 2) * 1000UL;
      }
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
