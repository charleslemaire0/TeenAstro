#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_Site

#define CLOCK_CH "&#x1F565;"
#define LOCATION "&#127760;"

const char html_BrowserTimeScript1[] PROGMEM =
"<script>\r\n"
"function SetDateTime(value) {\n"
"if ( value == 0 ){"
"  var d1 = new Date();\n"
"  document.getElementById('dd').value = d1.getUTCDate();\n"
"  document.getElementById('dm').value = d1.getUTCMonth();\n"
"  document.getElementById('dy').value = d1.getUTCFullYear();\n"
"  document.getElementById('th').value = d1.getUTCHours();\n"
"  document.getElementById('tm').value = d1.getUTCMinutes();\n"
"  document.getElementById('ts').value = d1.getUTCSeconds();\n"
"}\n"
"else if ( value == 1 ){ document.getElementById('GNSST').value = 1;}\n"
"else if ( value == 2 ){ document.getElementById('GNSSS').value = 1;}\n"
"}\r\n"
"</script>\r\n";
const char html_siteInfo[] PROGMEM =
"<div class='bt'>Site definition can be modified only at Home or at Park position!</div>";

const char html_siteQuick0[] PROGMEM =
"<div class='panel' style='width:100%;max-width:35em'>\n"
"<div class='panel-title'>Get Time and Location:</div>\n"
"<form style='display: inline;' method='get' action='/configuration_site.htm'>\n";
const char html_siteQuick1[] PROGMEM =
"<button name='b1' class='bb' value='st' type='submit' onpointerdown='SetDateTime(0);'> Browser " CLOCK_CH "</button>\n";
const char html_siteGNSS[] PROGMEM =
"<button name='b2' class='bb' value='st' type='submit' onpointerdown='SetDateTime(1);'> GNSS " CLOCK_CH "</button>\n"
"<button name='b3' class='bb' value='st' type='submit' onpointerdown='SetDateTime(2);'> GNSS " LOCATION "" CLOCK_CH "</button>\n";
const char html_siteQuick1a[] PROGMEM =
"<input id='dm' type='hidden' name='dm'>\n"
"<input id='dd' type='hidden' name='dd'>\n"
"<input id='dy' type='hidden' name='dy'>\n"
"<input id='th' type='hidden' name='th'>\n"
"<input id='tm' type='hidden' name='tm'>\n"
"<input id='ts' type='hidden' name='ts'>\n"
"<input id='GNSST' type='hidden' name='GNSST'>\n"
"<input id='GNSSS' type='hidden' name='GNSSS'>\n"
"</form></div>\n"
"\r\n\n";
const char html_configSiteSelect1[] PROGMEM =
"<div class='bt'>Selected Site</div>"
"<form method='post' action='/configuration_site.htm'>"
"<select onchange='this.form.submit()' style='width:100%;max-width:11em' name='site_select'>";
const char html_configSiteSelect2[] PROGMEM =
"</select>"
" (Select your predefined site)"
"</form>"
"<br/>\r\n";
const char html_configSiteName1[] PROGMEM =
"<div class='bt'>Selected Site Definition</div>"
"<form method='get' action='/configuration_site.htm'>";
const char html_configSiteName2[] PROGMEM =
" <input value='%s' style='width:100%%;max-width:10.25em' type='text' name='site_n' maxlength='14'>";
const char html_configSiteName3[] PROGMEM =
"<button type='submit'>Upload</button>"
" (Edit the name of the selected site)"
"</form>"
"\r\n";
const char html_configTimeZone[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>"
" <input value='%.1f' type='number' name='TimeZ' min='-12.' max='12' step='.5'>"
"<button type='submit'>Upload</button>"
" (Time Zone from -12 hour to 12 hour)"
"</form>"
"\r\n";
const char html_configLongWE1[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>"
"<select style='width:100%;max-width:5em' name='site_g0'>";
const char html_configLongWE2[] PROGMEM =
"</select>";
const char html_configLongDeg[] PROGMEM =
" <input value='%d' type='number' name='site_g1' min='0' max='179'>&nbsp;&deg;&nbsp;";
const char html_configLongMin[] PROGMEM =
" <input value='%d' type='number' name='site_g2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;";
const char html_configLongSec[] PROGMEM =
" <input value='%d' type='number' name='site_g3' min='0' max='59'>&nbsp;\"&nbsp;&nbsp;";
const char html_uploadLong1[] PROGMEM =
"<button type='submit'>Upload</button>";
const char html_uploadLong2[] PROGMEM =
" (Longitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configLatNS1[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>"
"<select style='width:100%;max-width:5em' name='site_t0'>";
const char html_configLatNS2[] PROGMEM =
"</select>";
const char html_configLatDeg[] PROGMEM =
" <input value='%d' type='number' name='site_t1' min='0' max='89'>&nbsp;&deg;&nbsp;";
const char html_configLatMin[] PROGMEM =
" <input value='%d' type='number' name='site_t2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;";
const char html_configLatSec[] PROGMEM =
" <input value='%d' type='number' name='site_t3' min='0' max='59'>&nbsp;\"&nbsp;&nbsp;";
const char html_uploadLat1[] PROGMEM =
"<button type='submit'>Upload</button>";
const char html_uploadLat2[] PROGMEM =
" (Latitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configElev1[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>";
const char html_configElev2[] PROGMEM =
" <input value='%s' type='number' name='site_e' min='-200' max='8000'>";
const char html_configElev3[] PROGMEM =
"<button type='submit'>Upload</button>";
const char html_configElev4[] PROGMEM =
" (Elevation, in meter min -200m max 8000m)"
"</form>"
"<br />\r\n";

// Short-lived cache of Site page LX200 reads (serial is the bottleneck).
// Invalidated on any Site mutation so PRG reload shows fresh values.
struct SitePageCache {
  unsigned long ms = 0;
  bool valid = false;
  int selected = 0;
  char names[3][32];
  char tz[20];
  char lat[20];
  char lon[20];
  char elev[20];
};
static SitePageCache s_siteCache;
static const unsigned long SITE_CACHE_MS = 3000;

static void invalidateSiteCache() { s_siteCache.valid = false; }

static bool fillSiteCacheFromMount(LX200Client* client)
{
  int selectedsite = 0;
  if (client->getSelectedSite(selectedsite) != LX200_VALUEGET ||
      selectedsite < 0 || selectedsite > 3)
    return false;

  for (int k = 0; k < 3; k++)
  {
    if (client->getSiteName(k, s_siteCache.names[k], sizeof(s_siteCache.names[k])) != LX200_VALUEGET)
      strcpy(s_siteCache.names[k], "?");
  }
  if (client->getTimeZoneStr(s_siteCache.tz, sizeof(s_siteCache.tz)) != LX200_VALUEGET)
    strcpy(s_siteCache.tz, "0");
  if (client->getLatitudeStr(s_siteCache.lat, sizeof(s_siteCache.lat)) != LX200_VALUEGET)
    strcpy(s_siteCache.lat, "+00*00*00");
  if (client->getLongitudeStr(s_siteCache.lon, sizeof(s_siteCache.lon)) != LX200_VALUEGET)
    strcpy(s_siteCache.lon, "+000*00*00");
  if (client->getElevation(s_siteCache.elev, sizeof(s_siteCache.elev)) == LX200_GETVALUEFAILED)
    strcpy(s_siteCache.elev, "+000");

  s_siteCache.selected = selectedsite;
  s_siteCache.ms = millis();
  s_siteCache.valid = true;
  return true;
}

static bool ensureSiteCache(LX200Client* client)
{
  if (s_siteCache.valid && (millis() - s_siteCache.ms < SITE_CACHE_MS))
    return true;
  return fillSiteCacheFromMount(client);
}

void TeenAstroWifi::handleConfigurationSite()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  if (processConfigurationSiteGet())
  {
    invalidateSiteCache();
    sendRedirectAfterMutation("/configuration_site.htm");
    return;
  }
  sendHtmlStart();
  char temp[150] = "";
  char temp1[50] = "";
  int tmp;
  String data;
  preparePage(data, ServerPage::Site);
  sendHtml(data);
  data += FPSTR(html_BrowserTimeScript1);
  sendHtml(data);
  data += FPSTR(html_siteInfo);
  data += "<div class='card'>";
  sendHtml(data);
  data += FPSTR(html_siteQuick0);
  sendHtml(data);
  data += FPSTR(html_siteQuick1);
  sendHtml(data);

  bool allowchange = (ta_MountStatus.atHome() || ta_MountStatus.getParkState() == TeenAstroMountStatus::PRK_PARKED);
  if (ta_MountStatus.hasGNSSBoard() && allowchange)
  {
    data += FPSTR(html_siteGNSS);
    sendHtml(data);
  }
  data += FPSTR(html_siteQuick1a);
  sendHtml(data);

  if (ensureSiteCache(s_client))
  {
    const int selectedsite = s_siteCache.selected;
    if (allowchange)
    {
      data += FPSTR(html_configSiteSelect1);
      sendHtml(data);
      for (int k = 0; k < 3; k++)
      {
        selectedsite == k ? data += "<option selected value='" : data += "<option value='";
        char kc = '0' + k;
        data += kc;
        data += "'>";
        data += s_siteCache.names[k];
        data += "</option>";
      }
      data += FPSTR(html_configSiteSelect2);
      sendHtml(data);
    }
    // Name — reuse cached names for sites 0..2 (avoids a 4th :GMx# round-trip).
    char siteName[50];
    if (selectedsite >= 0 && selectedsite <= 2)
      strncpy(siteName, s_siteCache.names[selectedsite], sizeof(siteName) - 1);
    else if (s_client->getSiteName(selectedsite, siteName, sizeof(siteName)) != LX200_VALUEGET)
      strcpy(siteName, "error!");
    siteName[sizeof(siteName) - 1] = '\0';
    data += FPSTR(html_configSiteName1);
    sprintf_P(temp, html_configSiteName2, siteName);
    data += temp;
    data += FPSTR(html_configSiteName3);
    sendHtml(data);

    // Time Zone
    float TShift = -(float)strtof(s_siteCache.tz, NULL);
    sprintf_P(temp, html_configTimeZone, TShift);
    data += temp;
    sendHtml(data);

    // Latitude
    strncpy(temp1, s_siteCache.lat, sizeof(temp1) - 1);
    temp1[sizeof(temp1) - 1] = '\0';
    data += FPSTR(html_configLatNS1);
    sendHtml(data);
    temp1[0] == '+' ? data += "<option selected value='0'>North</option>" : data += "<option value='0'>North</option>";
    temp1[0] == '-' ? data += "<option selected value='1'>Sud</option>" : data += "<option value='1'>Sud</option>";
    data += FPSTR(html_configLatNS2);
    temp1[0] = '0';
    temp1[3] = 0; temp1[6] = 0; temp1[9] = 0;
    tmp = (int)strtol(&temp1[0], NULL, 10);
    sprintf_P(temp, html_configLatDeg, tmp);
    data += temp;
    tmp = (int)strtol(&temp1[4], NULL, 10);
    sprintf_P(temp, html_configLatMin, tmp);
    data += temp;
    tmp = (int)strtol(&temp1[7], NULL, 10);
    sprintf_P(temp, html_configLatSec, tmp);
    data += temp;
    if (allowchange) data += FPSTR(html_uploadLat1);
    data += FPSTR(html_uploadLat2);
    sendHtml(data);

    // Longitude
    strncpy(temp1, s_siteCache.lon, sizeof(temp1) - 1);
    temp1[sizeof(temp1) - 1] = '\0';
    data += FPSTR(html_configLongWE1);
    temp1[0] == '+' ? data += "<option selected value='0'>West</option>" : data += "<option value='0'>West</option>";
    temp1[0] == '-' ? data += "<option selected value='1'>East</option>" : data += "<option value='1'>East</option>";
    data += FPSTR(html_configLongWE2);
    temp1[0] = '0';
    temp1[4] = 0; temp1[7] = 0; temp1[10] = 0;
    tmp = (int)strtol(&temp1[0], NULL, 10);
    sprintf_P(temp, html_configLongDeg, tmp);
    data += temp;
    tmp = (int)strtol(&temp1[5], NULL, 10);
    sprintf_P(temp, html_configLongMin, tmp);
    data += temp;
    tmp = (int)strtol(&temp1[8], NULL, 10);
    sprintf_P(temp, html_configLongSec, tmp);
    data += temp;
    if (allowchange) data += FPSTR(html_uploadLong1);
    data += FPSTR(html_uploadLong2);
    sendHtml(data);

    // Elevation
    strncpy(temp1, s_siteCache.elev, sizeof(temp1) - 1);
    temp1[sizeof(temp1) - 1] = '\0';
    if (temp1[0] == '+') temp1[0] = '0';
    data += FPSTR(html_configElev1);
    sprintf_P(temp, html_configElev2, temp1);
    data += temp;
    if (allowchange) data += FPSTR(html_configElev3);
    data += FPSTR(html_configElev4);
    sendHtml(data);
  }
  data += "</div>"; // close card
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}

int get_temp_month;
int get_temp_day;
int get_temp_year;
int get_temp_hour;
int get_temp_minute;
int get_temp_second;

bool TeenAstroWifi::processConfigurationSiteGet()
{
  bool any = false;
  String v;
  int i;
  float f;

  // selected site
  v = server.arg("site_select");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 3)))
      s_client->setSelectedSite(i);
  }
  // name
  v = server.arg("site_n");
  if (v != "")
  {
    any = true;
    s_client->setSiteName(v.c_str());
  }

  // Time Zone
  v = server.arg("TimeZ");
  if (v != "")
  {
    any = true;
    if ((atof2((char*)v.c_str(), &f)) && ((f >= -12) && (f <= 12)))
      s_client->setTimeZone(-f);
  }

  // Set DATE/TIME
  v = server.arg("dm");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 11)))
      get_temp_month = i + 1;
  }
  v = server.arg("dd");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 31)))
      get_temp_day = i;
  }
  v = server.arg("dy");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 2016) && (i <= 9999)))
    {
      get_temp_year = i - 2000;
      s_client->setUTCDateRaw(get_temp_month, get_temp_day, get_temp_year);
    }
  }
  v = server.arg("th");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 23)))
      get_temp_hour = i;
  }
  v = server.arg("tm");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 59)))
      get_temp_minute = i;
  }
  v = server.arg("ts");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 59)))
    {
      get_temp_second = i;
      s_client->setUTCTimeRaw(get_temp_hour, get_temp_minute, get_temp_second);
    }
  }

  // Location — Longitude
  int long_deg = -999;
  int long_min = 0;
  int sign = -1;
  v = server.arg("site_g0");
  if (v != "") { any = true; if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) sign = i; }
  v = server.arg("site_g1");
  if (v != "") { any = true; if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 180))) long_deg = i; }
  v = server.arg("site_g2");
  if (v != "") { any = true; if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 60))) long_min = i; }
  v = server.arg("site_g3");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 60)))
    {
      if ((long_deg >= 0) && (long_deg < 180) && (long_min >= 0) && (long_min < 60))
        s_client->setLongitudeDMS(sign, long_deg, long_min, i);
    }
  }

  // Location — Latitude
  int lat_deg = -999;
  int lat_min = 0;
  v = server.arg("site_t0");
  if (v != "") { any = true; if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) sign = i; }
  v = server.arg("site_t1");
  if (v != "") { any = true; if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 90))) lat_deg = i; }
  v = server.arg("site_t2");
  if (v != "") { any = true; if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 60))) lat_min = i; }
  v = server.arg("site_t3");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 60)))
    {
      if ((lat_deg >= 0) && (lat_deg < 90) && (lat_min >= 0) && (lat_min < 60))
        s_client->setLatitudeDMS(sign, lat_deg, lat_min, i);
    }
  }

  // Elevation
  v = server.arg("site_e");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -200) && (i <= 8000)))
      s_client->setElevation(i);
  }

  // GNSS sync
  v = server.arg("GNSSS");
  if (v != "") { any = true; s_client->syncTime(); }
  v = server.arg("GNSST");
  if (v != "") { any = true; s_client->syncLocation(); }

  return any;
}
