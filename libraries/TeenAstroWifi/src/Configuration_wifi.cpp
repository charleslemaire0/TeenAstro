#include "TeenAstroWifi.h"


// -----------------------------------------------------------------------------------
// Wifi setup


const char html_wifiSerial1[] PROGMEM =
"<b>Performance and compatibility:</b><br/>"
"<form method='post' action='/wifi.htm'>"
"Command channel serial read time-out: <input style='width:4em' name='ccto' value='%d' type='number' min='5' max='100'> ms<br/>"
"Web channel serial read time-out: <input style='width:4em' name='wcto' value='%d' type='number' min='5' max='100'> ms<br/>"
"<button type='submit'>Upload</button></form><br />\r\n";
const char html_wifiMode1[] PROGMEM =
"<br/><b>Wifi Mode:</b><br/>"
"<form method='post' action='/wifi.htm'>"
"<select style='width:10em' name='wifimode'>";
const char html_wifiMode2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
"</form>"
"<br/>\r\n";
const char html_wifiConnectMode1[] PROGMEM =
"<br/><b>Wifi connection Mode:</b><br/>"
"<form method='post' action='/wifi.htm'>"
"<select style='width:10em' name='wificonnectionmode'>";
const char html_wifiConnectMode2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
"</form>"
"<br/>\r\n";

const char html_wifiSSID1A[] PROGMEM =
"<br/><b>Station mode %d (connect to an Access-Point):</b><br/>"
"<form method='post' action='/wifi.htm'>";
const char html_wifiSSID1B[] PROGMEM =
"SSID: <input style='width:10em' name='stssid%d' type='text' value='%s' maxlength='32'>&nbsp;&nbsp;&nbsp;";
const char html_wifiSSID1C[] PROGMEM =
"Password: <input style='width:10em' name='stpwd%d' type='password' value='%s' maxlength='39'><br/>";
const char html_wifiSSID2[] PROGMEM =
"Enable DHCP: <input type='checkbox' name='stadhcp%d' value='1' %s> (Note: above addresses are ignored if DHCP is enabled)<br/>"
"<button type='submit'>Upload</button></form><br />\r\n";

const char html_wifiMAC[] PROGMEM =
"MAC: <input style='width:10em' name='stmac%d' type='text' value='%s' maxlength='17' disabled><br/>";

const char html_wifiSTAIP[] PROGMEM =
"<table><tr><td>IP Address: </td><td>"
"<input name='staip0%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='staip1%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='staip2%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='staip3%d' value='%d' type='number' min='0' max='255'></td>";
const char html_wifiSTAGW[] PROGMEM =
"<tr><td>Gateway: </td><td>"
"<input name='stagw0%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='stagw1%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='stagw2%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='stagw3%d' value='%d' type='number' min='0' max='255'></td>";
const char html_wifiSTASN[] PROGMEM =
"<tr><td>Subnet: </td><td>"
"<input name='stasn0%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='stasn1%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='stasn2%d' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='stasn3%d' value='%d' type='number' min='0' max='255'></td></tr></table>";

const char html_wifiSSID3[] PROGMEM =
"<br/><b>Access-Point mode:</b><br/>"
"<form method='post' action='/wifi.htm'>"
"SSID: <input style='width:6em' name='apssid' type='text' value='%s' maxlength='32'>&nbsp;&nbsp;&nbsp;"
"Password: <input style='width:8em' name='appwd' type='password' value='%s' maxlength='39'>&nbsp;&nbsp;&nbsp;"
"Channel: <input style='width:2em' name='apch' value='%d' type='number' min='1' max='11'><br/>";

const char html_wifiApMAC[] PROGMEM =
"MAC: <input style='width:10em' name='apmac' type='text' value='%s' maxlength='17' disabled><br/>";

const char html_wifiSSID4[] PROGMEM =
"<table><tr><td>IP Address: </td><td>"
"<input name='apip0' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apip1' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apip2' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apip3' value='%d' type='number' min='0' max='255'></td>";
const char html_wifiSSID5[] PROGMEM =
"<tr><td>Gateway: </td><td>"
"<input name='apgw0' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apgw1' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apgw2' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apgw3' value='%d' type='number' min='0' max='255'></td>";
const char html_wifiSSID6[] PROGMEM =
"<tr><td>Subnet: </td><td>"
"<input name='apsn0' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apsn1' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apsn2' value='%d' type='number' min='0' max='255'>&nbsp;.&nbsp;"
"<input name='apsn3' value='%d' type='number' min='0' max='255'></td></tr></table>";

const char html_wifiSSID7[] PROGMEM =
"<button type='submit'>Upload</button></form><br />\r\n";

const char html_logout[] =
"<br/><b>WiFi configuration_telescope Security:</b><br/>"
"<form method='post' action='/wifi.htm'>"
"Password: <input style='width:8em' name='webpwd' type='password' maxlength='39'> "
"<button type='submit'>Upload</button></form>"
"<form method='post' action='/wifi.htm'>"
"<button type='submit' name='logout' value='1'>Logout</button></form><br />\r\n";

const char html_reboot[] PROGMEM =
"<br/><br/><br/><br/><br/><form method='get' action='/wifi.htm'>"
"<b>You must <u>manually</u> restart for changes to take effect.</b><br/><br/>"
"<button type='submit'>Continue</button>"
"</form><br/><br/><br/><br/>"
"\r\n";

const char html_login[] PROGMEM =
"<br/><form method='post' action='/wifi.htm'>"
"<br/>Enter password to change WiFi configuration_telescope:<br />"
"<input style='width:8em' name='login' type='password' maxlength='39'>"
"<button type='submit'>Ok</button>"
"</form><br/><br/><br/>"
"\r\n";

const char html_update[] PROGMEM =

"<br/><form>"
"<br/><b>Smart Hand Controller Firmware Update:</b><br />"
"<input type='button' style='width:12em' value='Update Firmware' onclick = ""Javascript:location.href='/update'"" ></form>"
"Firmware extention is *.bin and can be found in the TeenAstroLoader.exe directory<br/><br/>"
"\r\n";

bool restartRequired = false;
bool loginRequired = true;

// convert hex to int with error checking
// returns -1 on error
int TeenAstroWifi::hexToInt(String s)
{
  int i0;
  int i1;
  if (s.length() != 2) return -1;
  char c0 = s.charAt(0);
  char c1 = s.charAt(1);
  if ((((c0 >= '0') && (c0 <= '9')) || ((c0 >= 'A') && (c0 <= 'F'))) &&
    (((c1 >= '0') && (c1 <= '9')) || ((c1 >= 'A') && (c1 <= 'F'))))
  {
    if ((c0 >= '0') && (c0 <= '9'))
    {
      i0 = c0 - '0';
    }
    else
    {
      i0 = (c0 - 'A') + 10;
    }
    if ((c1 >= '0') && (c1 <= '9'))
    {
      i1 = c1 - '0';
    }
    else
    {
      i1 = (c1 - 'A') + 10;
    }
    return i0 * 16 + i1;
  }
  else return -1;
}



void TeenAstroWifi::handleWifi()
{
  char temp[300] = "";
  char temp1[80] = "";
  String data;
  sendHtmlStart();
  processWifiGet();
  preparePage(data, ServerPage::Wifi);
  sendHtml(data);
  data += "<div style='width: 40em;'>";

  if (restartRequired)
  {
    data += FPSTR(html_reboot);
    data += "</div></div></body></html>";
    sendHtml(data);
    sendHtmlDone(data);
    restartRequired = false;
    delay(1000);
    return;
  }

  if (loginRequired)
  {
    data += FPSTR(html_login);
    data += "</div></div></body></html>";
    sendHtml(data);
    sendHtmlDone(data);
    return;
  }

  data += FPSTR(html_wifiMode1);
  activeWifiMode == WifiMode::M_Station1 ? data += "<option selected value='0'>StationMode0</option>" : data += "<option value='0'>StationMode0</option>";
  activeWifiMode == WifiMode::M_Station2 ? data += "<option selected value='1'>StationMode1</option>" : data += "<option value='1'>StationMode1</option>";
  activeWifiMode == WifiMode::M_Station3 ? data += "<option selected value='2'>StationMode2</option>" : data += "<option value='2'>StationMode2</option>";
  activeWifiMode == WifiMode::M_AcessPoint ? data += "<option selected value='3'>AccessPoint</option>" : data += "<option value='3'>AccessPoint</option>";
  data += FPSTR(html_wifiMode2);
  sendHtml(data);
  sprintf_P(temp, html_wifiSerial1, CmdTimeout, WebTimeout); data += temp;
  data += FPSTR(html_wifiConnectMode1);
  activeWifiConnectMode == WifiConnectMode::AutoClose ? data += "<option selected value='0'>One to Many</option>" : data += "<option value='0'>One to Many</option>";
  activeWifiConnectMode == WifiConnectMode::KeepOpened ? data += "<option selected value='1'>One to One</option>" : data += "<option value='1'>One to One</option>";
  data += FPSTR(html_wifiConnectMode2);
  sendHtml(data);
  for (int k = 0; k < NUM_sta; k++)
  {
    sprintf_P(temp, html_wifiSSID1A, k); data += temp;
    sprintf_P(temp, html_wifiSSID1B, k, wifi_sta_ssid[k]); data += temp;
    sprintf_P(temp, html_wifiSSID1C, k, ""); data += temp;
    uint8_t mac[6] = { 0,0,0,0,0,0 }; WiFi.macAddress(mac);
    char wifi_sta_mac[80] = "";
    for (int i = 0; i < 6; i++)
    {
      sprintf(wifi_sta_mac, "%s%02x:", wifi_sta_mac, mac[i]);
    } wifi_sta_mac[strlen(wifi_sta_mac) - 1] = 0;
    sprintf_P(temp, html_wifiMAC, k, wifi_sta_mac); data += temp;
    IPAddress tempIP = wifi_sta_ip[k];
    sprintf_P(temp, html_wifiSTAIP, k, wifi_sta_ip[k][0], k, wifi_sta_ip[k][1], k, wifi_sta_ip[k][2], k, wifi_sta_ip[k][3]); data += temp;
    sprintf_P(temp, html_wifiSTAGW, k, wifi_sta_gw[k][0], k, wifi_sta_gw[k][1], k, wifi_sta_gw[k][2], k, wifi_sta_gw[k][3]); data += temp;
    sprintf_P(temp, html_wifiSTASN, k, wifi_sta_sn[k][0], k, wifi_sta_sn[k][1], k, wifi_sta_sn[k][2], k, wifi_sta_sn[k][3]); data += temp;
    sprintf_P(temp, html_wifiSSID2, k, stationDhcpEnabled[k] ? "checked" : ""); data += temp;
    sendHtml(data);
  }
  sprintf_P(temp, html_wifiSSID3, wifi_ap_ssid, "", wifi_ap_ch); data += temp;
  uint8_t macap[6] = { 0,0,0,0,0,0 }; WiFi.softAPmacAddress(macap);
  char wifi_ap_mac[80] = "";
  for (int i = 0; i < 6; i++)
  {
    sprintf(wifi_ap_mac, "%s%02x:", wifi_ap_mac, macap[i]);
  } wifi_ap_mac[strlen(wifi_ap_mac) - 1] = 0;
  sprintf_P(temp, html_wifiApMAC, wifi_ap_mac); data += temp;
  sendHtml(data);
  sprintf_P(temp, html_wifiSSID4, wifi_ap_ip[0], wifi_ap_ip[1], wifi_ap_ip[2], wifi_ap_ip[3]); data += temp;
  sprintf_P(temp, html_wifiSSID5, wifi_ap_gw[0], wifi_ap_gw[1], wifi_ap_gw[2], wifi_ap_gw[3]); data += temp;
  sprintf_P(temp, html_wifiSSID6, wifi_ap_sn[0], wifi_ap_sn[1], wifi_ap_sn[2], wifi_ap_sn[3]); data += temp;
  sprintf_P(temp, html_wifiSSID7, activeWifiMode == WifiMode::M_AcessPoint ? "checked" : ""); data += temp;
  data += FPSTR(html_logout);
  sendHtml(data);
  data += FPSTR(html_update);
  sendHtml(data);

  strcpy(temp, "</div></div></body></html>");
  data += temp;

  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processWifiGet()
{
  String v, v1;

  boolean EEwrite = false;

  // Login --------------------------------------------------------------------
  v = server.arg("login");
  if (v != "")
  {
    if (!strcmp(masterPassword, (char*)v.c_str())) loginRequired = false;
  }
  v = server.arg("logout");
  if (v != "") loginRequired = true;
  if (loginRequired) return;
  v = server.arg("webpwd");
  if (v != "")
  {
    strcpy(masterPassword, (char*)v.c_str());
    TeenAstroWifi::EEPROM_writeString(EPPROM_password, masterPassword);
    EEwrite = true;
  }
  //wifi Mode

  v = server.arg("wifimode");
  if (v != "")
  {
    activeWifiMode = static_cast<WifiMode>(v.toInt());
    EEPROM.write(EEPROM_WifiMode, activeWifiMode);
    EEwrite = true;
    restartRequired = true;
  }

  v = server.arg("wificonnectionmode");
  if (v != "")
  {
    activeWifiConnectMode = static_cast<WifiConnectMode>(v.toInt() > 0);
    EEPROM.write(EEPROM_WifiConnectMode, activeWifiConnectMode);
    EEwrite = true;
    restartRequired = true;
  }

  // Timeouts -----------------------------------------------------------------
  // Cmd channel timeout
  v = server.arg("ccto");
  if (v != "")
  {
    CmdTimeout = v.toInt();
    EEPROM.write(EEPROM_CmdTimeout, CmdTimeout);
    EEwrite = true;
  }

  // Web channel timeout
  v = server.arg("wcto");
  if (v != "")
  {
    WebTimeout = v.toInt();
    EEPROM.write(EEPROM_WebTimeout, WebTimeout);
    EEwrite = true;
  }

  // --------------------------------------------------------------------------------------------------------
  for (int k = 0; k < NUM_sta; k++)
  {
    char cmd[20];
    // Station MAC
    sprintf(cmd, "stmac%d", k);
    v = server.arg(cmd);
    if (v != "")
    {
      // 5c:cf:7f:0f:ad:85
      // first the length should be 17
      if (v.length() == 17)
      {
        // seperators all in place
        if ((v.charAt(2) == ':') && (v.charAt(5) == ':') && (v.charAt(8) == ':') && (v.charAt(11) == ':') && (v.charAt(14) == ':'))
        {
          // digits all in 0..9,A..F and validate
          v.toUpperCase();
          uint8_t mac[6];
          mac[0] = hexToInt(v.substring(0, 2)); mac[1] = hexToInt(v.substring(3, 2)); mac[2] = hexToInt(v.substring(6, 2));
          mac[3] = hexToInt(v.substring(9, 2)); mac[4] = hexToInt(v.substring(12, 2)); mac[5] = hexToInt(v.substring(15, 2));
          if ((mac[0] >= 0) && (mac[1] >= 0) && (mac[2] >= 0) && (mac[3] >= 0) && (mac[4] >= 0) && (mac[5] >= 0))
          {
            WiFi.macAddress(mac); restartRequired = true;
          }
        }
      }
    }

    // Station SSID
    sprintf(cmd, "stssid%d", k);
    v = server.arg(cmd); v1 = v;
    if (v != "")
    {
      if (!strcmp(wifi_sta_ssid[k], (char*)v.c_str())) restartRequired = true;
      strcpy(wifi_sta_ssid[k], (char*)v.c_str());
      // if this section was submitted set the stationEnabled default to false
      stationDhcpEnabled[k] = false;
    }

    // Station password
    sprintf(cmd, "stpwd%d", k);
    v = server.arg(cmd);
    if (v != "")
    {
      if (!strcmp(wifi_sta_pwd[k], (char*)v.c_str())) restartRequired = true;
      strcpy(wifi_sta_pwd[k], (char*)v.c_str());
    }

    // Station dhcp enabled
    sprintf(cmd, "stadhcp%d", k);
    v = server.arg(cmd);
    if (v != "")
    {
      stationDhcpEnabled[k] = v.toInt();
    }


    // Station Access-Point ip

    for (int i = 0; i < 4; i++)
    {
      sprintf(cmd, "staip%d%d", i, k);
      v = server.arg(cmd); if (v != "") wifi_sta_ip[k][i] = v.toInt();
    }
    for (int i = 0; i < 4; i++)
    {
      sprintf(cmd, "stasn%d%d", i, k);
      v = server.arg(cmd); if (v != "") wifi_sta_sn[k][i] = v.toInt();
    }
    for (int i = 0; i < 4; i++)
    {
      sprintf(cmd, "stagw%d%d", i, k);
      v = server.arg(cmd); if (v != "") wifi_sta_gw[k][i] = v.toInt();
    }
    if (v1 != "")
    {
      writeStation2EEPROM(k);
      EEwrite = true;
      restartRequired = true;
    }
  }

  // -------------------------------------------------------------------------------------------

  // Access-Point MAC
  v = server.arg("apmac");
  if (v != "")
  {
    // 5c:cf:7f:0f:ad:85
    // first the length should be 17
    if (v.length() == 17)
    {
      // seperators all in place
      if ((v.charAt(2) == ':') && (v.charAt(5) == ':') && (v.charAt(8) == ':') && (v.charAt(11) == ':') && (v.charAt(14) == ':'))
      {
        // digits all in 0..9,A..F and validate
        v.toUpperCase();
        uint8_t mac[6];
        mac[0] = hexToInt(v.substring(0, 2)); mac[1] = hexToInt(v.substring(3, 2)); mac[2] = hexToInt(v.substring(6, 2));
        mac[3] = hexToInt(v.substring(9, 2)); mac[4] = hexToInt(v.substring(12, 2)); mac[5] = hexToInt(v.substring(15, 2));
        if ((mac[0] >= 0) && (mac[1] >= 0) && (mac[2] >= 0) && (mac[3] >= 0) && (mac[4] >= 0) && (mac[5] >= 0))
        {
          WiFi.softAPmacAddress(mac); restartRequired = true;
        }
      }
    }
  }

  // Access-Point SSID
  v = server.arg("apssid");
  if (v != "")
  {
    if (!strcmp(wifi_ap_ssid, (char*)v.c_str())) restartRequired = true;
    strcpy(wifi_ap_ssid, (char*)v.c_str());

    // if this section was submitted set the accessPointEnabled default to false
    //accessPointEnabled[k]=false;
  }

  // Access-Point password
  v = server.arg("appwd");
  if (v != "")
  {
    if (!strcmp(wifi_ap_pwd, (char*)v.c_str())) restartRequired = true;
    strcpy(wifi_ap_pwd, (char*)v.c_str());
  }

  // Access-Point channel
  v = server.arg("apch");
  if (v != "")
  {
    if (wifi_ap_ch != v.toInt()) restartRequired = true;
    wifi_ap_ch = v.toInt();
  }

  // Access-Point ip
  v = server.arg("apip0"); if (v != "") wifi_ap_ip[0] = v.toInt();
  v = server.arg("apip1"); if (v != "") wifi_ap_ip[1] = v.toInt();
  v = server.arg("apip2"); if (v != "") wifi_ap_ip[2] = v.toInt();
  v = server.arg("apip3"); if (v != "") wifi_ap_ip[3] = v.toInt();

  // Access-Point SubNet
  v = server.arg("apsn0"); if (v != "") wifi_ap_sn[0] = v.toInt();
  v = server.arg("apsn1"); if (v != "") wifi_ap_sn[1] = v.toInt();
  v = server.arg("apsn2"); if (v != "") wifi_ap_sn[2] = v.toInt();
  v = server.arg("apsn3"); if (v != "") wifi_ap_sn[3] = v.toInt();

  // Access-Point Gateway
  v = server.arg("apgw0"); if (v != "") wifi_ap_gw[0] = v.toInt();
  v = server.arg("apgw1"); if (v != "") wifi_ap_gw[1] = v.toInt();
  v = server.arg("apgw2"); if (v != "") wifi_ap_gw[2] = v.toInt();
  v = server.arg("apgw3"); if (v != "") wifi_ap_gw[3] = v.toInt();

  if (v != "")
  {
    writeAccess2EEPROM();
    EEwrite = true;
    restartRequired = true;
  }
  if (EEwrite) EEPROM.commit();
}

