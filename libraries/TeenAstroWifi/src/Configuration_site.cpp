#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_Site

#define CLOCK_CH "&#x1F565;"
#define LOCATION "&#127760;"
 //Request https!!!
//const char html_configSiteScript[] PROGMEM =
//"<script>\n"
//"function ConvertDDToDMS(D) {\n"
//"  return {\n"
//"    dir : D < 0 ? 1 : 0,"
//"    deg : 0 | (D < 0 ? (D = -D) : D),\n"
//"    min : 0 | (((D += 1e-9) % 1) * 60),\n"
//"    sec : (0 | (((D * 60) % 1) * 6000)) / 100,\n"
//"  };\n"
//"}\n"
//"function getLocation() {\n"
//"  window.alert('sometext');\n"
//"  if (navigator.geolocation) {\n"
//"    navigator.geolocation.getCurrentPosition(showPosition);\n"
//"  }\n"
//"  else {\n"
//"  }\n"
//"}\n"
//"function showPosition(position) {\n"
//"var lat = ConvertDDToDMS(position.coords.latitude);\n"
//"var long = ConvertDDToDMS(position.coords.longitude);\n"
//"document.getElementById('site_g0').value = long.dir;\n"
//"document.getElementById('site_g1').value = long.deg;\n"
//"document.getElementById('site_g2').value = long.min;\n"
//"document.getElementById('site_g3').value = 0;\n"
//"document.getElementById('site_t0').value = lat.dir;\n"
//"document.getElementById('site_t1').value = lat.deg;\n"
//"document.getElementById('site_t2').value = lat.min;\n"
//"document.getElementById('site_t3').value = 0;\n"
//"}\n"
//"</script>\n";


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

const char html_siteQuick0[] PROGMEM =
"<div class='b1' style='width: 35em'>\n"
"<div class='bct' align='left'>Get Time and Location:</div>\n"
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
"<br class='clear' />\r\n\n";
const char html_configSiteSelect1[] PROGMEM =
"<div class='bt' align='left'> Selected Site :<br/> </div>"
"<form method='post' action='/configuration_site.htm'>"
"<select onchange='this.form.submit()' style='width:11em' name='site_select'>";
const char html_configSiteSelect2[] PROGMEM =
"</select>"
" (Select your predefined site)"
"</form>"
"<br/>\r\n";
const char html_configSiteName1[] PROGMEM =
"<div class='bt' align='left'> Selected Site definition: <br/> </div>"
"<form method='get' action='/configuration_site.htm'>";
const char html_configSiteName2[] PROGMEM =
" <input value='%s' style='width:10.25em' type='text' name='site_n' maxlength='14'>";
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
"<select style='width:5em' name='site_g0'>";
const char html_configLongWE2[] PROGMEM =
"</select>";
const char html_configLongDeg[] PROGMEM =
" <input value='%d' type='number' name='site_g1' min='0' max='179'>&nbsp;&deg;&nbsp;";
const char html_configLongMin[] PROGMEM =
" <input value='%d' type='number' name='site_g2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;";
const char html_configLongSec[] PROGMEM =
" <input value='%d' type='number' name='site_g3' min='0' max='59'>&nbsp;\"&nbsp;&nbsp;";
const char html_uploadLong[] PROGMEM =
"<button type='submit'>Upload</button>"
" (Longitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configLatNS1[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>"
"<select style='width:5em' name='site_t0'>";
const char html_configLatNS2[] PROGMEM =
"</select>";
const char html_configLatDeg[] PROGMEM =
" <input value='%d' type='number' name='site_t1' min='0' max='89'>&nbsp;&deg;&nbsp;";
const char html_configLatMin[] PROGMEM =
" <input value='%d' type='number' name='site_t2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;";
const char html_configLatSec[] PROGMEM =
" <input value='%d' type='number' name='site_t3' min='0' max='59'>&nbsp;\"&nbsp;&nbsp;";
const char html_uploadLat[] PROGMEM =
"<button type='submit'>Upload</button>"
" (Latitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configElev1[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>";
const char html_configElev2[] PROGMEM =
" <input value='%s' type='number' name='site_e' min='-200' max='8000'>";
const char html_configElev3[] PROGMEM =
"<button type='submit'>Upload</button>"
" (Elevation, in meter min -200m max 8000m)"
"</form>"
"<br />\r\n";


void TeenAstroWifi::handleConfigurationSite()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[150] = "";
  char temp1[50] = "";
  int tmp;
  String data;
  processConfigurationSiteGet();
  preparePage(data, ServerPage::Site);
  sendHtml(data);
  data += FPSTR(html_BrowserTimeScript1);
  sendHtml(data);
  data += FPSTR(html_siteQuick0);
  sendHtml(data);
  data += FPSTR(html_siteQuick1);
  sendHtml(data);
  if (ta_MountStatus.hasGNSSBoard() && (ta_MountStatus.atHome() || ta_MountStatus.getParkState() == TeenAstroMountStatus::PRK_PARKED))
  {
    data += FPSTR(html_siteGNSS);
    sendHtml(data);
  }
  data += FPSTR(html_siteQuick1a);
  sendHtml(data);
  if (GetLX200(":W?#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  {
    int selectedsite = 0;
    if ((atoi2(temp1, &selectedsite)) && ((selectedsite >= 0) && (selectedsite <= 3)))
    {
      char m[32]; char n[32]; char o[32];
      GetLX200(":GM#", m, sizeof(m));
      GetLX200(":GN#", n, sizeof(n));
      GetLX200(":GO#", o, sizeof(o));
      data += FPSTR(html_configSiteSelect1);
      sendHtml(data);
      selectedsite == 0 ? data += "<option selected value='0'>" : data += "<option value='0'>";
      sprintf(temp, "%s</option>", m);
      data += temp;
      selectedsite == 1 ? data += "<option selected value='1'>" : data += "<option value='1'>";
      sprintf(temp, "%s</option>", n);
      data += temp;
      selectedsite == 2 ? data += "<option selected value='2'>" : data += "<option value='2'>";
      sprintf(temp, "%s</option>", o);
      data += temp;
      data += FPSTR(html_configSiteSelect2);
      sendHtml(data);

      // Name
      if (GetLX200(":Gn#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "error!");
      data += FPSTR(html_configSiteName1);
      sprintf_P(temp, html_configSiteName2, temp1);
      data += temp;
      data += FPSTR(html_configSiteName3);
      sendHtml(data);

      // Time Zone
      if (GetLX200(":GG#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); float TShift = -(float)strtof(&temp1[0], NULL);
      sprintf_P(temp, html_configTimeZone, TShift);
      data += temp;
      sendHtml(data);

      // Latitude
      if (GetLX200(":Gtf#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "+00*00*00");
      data += FPSTR(html_configLatNS1);
      sendHtml(data);
      temp1[0] == '+' ? data += "<option selected value='0'>North</option>" : data += "<option value='0'>North</option>";
      temp1[0] == '-' ? data += "<option selected value='1'>Sud</option>" : data += "<option value='1'>Sud</option>";
      data += FPSTR(html_configLatNS2);
      temp1[0] = '0'; // remove +
      temp1[3] = 0;
      temp1[6] = 0;
      temp1[9] = 0;
      tmp = (int)strtol(&temp1[0], NULL, 10);
      sprintf_P(temp, html_configLatDeg, tmp);
      data += temp;
      tmp = (int)strtol(&temp1[4], NULL, 10);
      sprintf_P(temp, html_configLatMin, tmp);
      data += temp;
      tmp = (int)strtol(&temp1[7], NULL, 10);
      sprintf_P(temp, html_configLatSec, tmp);
      data += temp;
      data += FPSTR(html_uploadLat);
      sendHtml(data);
      // Longitude
      if (GetLX200(":Ggf#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "+000*00*00");
      data += FPSTR(html_configLongWE1);
      temp1[0] == '+' ? data += "<option selected value='0'>West</option>" : data += "<option value='0'>West</option>";
      temp1[0] == '-' ? data += "<option selected value='1'>East</option>" : data += "<option value='1'>East</option>";
      data += FPSTR(html_configLongWE2);
      temp1[0] = '0'; // sign
      temp1[4] = 0;
      temp1[7] = 0;
      temp1[10] = 0;
      tmp = (int)strtol(&temp1[0], NULL, 10);
      sprintf_P(temp, html_configLongDeg, tmp);
      data += temp;
      tmp = (int)strtol(&temp1[5], NULL, 10);
      sprintf_P(temp, html_configLongMin, tmp);
      data += temp;
      tmp = (int)strtol(&temp1[8], NULL, 10);
      sprintf_P(temp, html_configLongSec, tmp);
      data += temp;
      data += FPSTR(html_uploadLong);
      sendHtml(data);
      // Elevation
      if (GetLX200(":Ge#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "+000");
      if (temp1[0] == '+') temp1[0] = '0';
      data += FPSTR(html_configElev1);
      sprintf_P(temp, html_configElev2, temp1);
      data += temp;
      data += FPSTR(html_configElev3);
      sendHtml(data);

    }
  }
  strcpy(temp, "</div></div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

int get_temp_month;
int get_temp_day;
int get_temp_year;
int get_temp_hour;
int get_temp_minute;
int get_temp_second;

void TeenAstroWifi::processConfigurationSiteGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";
  // selected site
  v = server.arg("site_select");
  if (v != "")
  {
    sprintf(temp, ":W%s#", (char*)v.c_str());
    SetLX200(temp);
  }
  // name
  v = server.arg("site_n");
  if (v != "")
  {
    sprintf(temp, ":Sn%s#", (char*)v.c_str());
    SetLX200(temp);
  }
  //Time Zone
  v = server.arg("TimeZ");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= -12) && (f <= 12)))
    {
      sprintf(temp, ":SG%+05.1f#", -f);
      SetLX200(temp);
    }
  }

  // Set DATE/TIME
  v = server.arg("dm");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 11)))
    {
      get_temp_month = i + 1;
    }
  }
  v = server.arg("dd");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 31)))
    {
      get_temp_day = i;
    }
  }
  v = server.arg("dy");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 2016) && (i <= 9999)))
    {
      get_temp_year = i - 2000;
      char temp[10];
      sprintf(temp, ":SXT1%02d/%02d/%02d#", get_temp_month, get_temp_day, get_temp_year);
      SetLX200(temp);
    }
  }
  v = server.arg("th");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 23)))
    {
      get_temp_hour = i;
    }
  }
  v = server.arg("tm");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 59)))
    {
      get_temp_minute = i;
    }
  }
  v = server.arg("ts");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 59)))
    {
      get_temp_second = i;
      char temp[10];
      sprintf(temp, ":SXT0%02d:%02d:%02d#", get_temp_hour, get_temp_minute, get_temp_second);
      SetLX200(temp);
    }
  }

  // Location
  int long_deg = -999;
  int long_min = 0;
  int sign = -1;
  v = server.arg("site_g0");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      sign = i;
    }
  }
  v = server.arg("site_g1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 180)))
    {
      long_deg = i;
    }
  }
  v = server.arg("site_g2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 60)))
    {
      long_min = i;
    }
  }
  v = server.arg("site_g3");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 60)))
    {
      if ((long_deg >= 0) && (long_deg < 180) && (long_min >= 0) && (long_min < 60))
      {
        sprintf(temp, ":Sg%+04d:%02d:%02d#", long_deg, long_min, i);
        if (sign)
          temp[3] = '-';
        SetLX200(temp);
      }
    }
  }

  int lat_deg = -999;
  int lat_min = 0;
  v = server.arg("site_t0");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      sign = i;
    }
  }
  v = server.arg("site_t1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 90)))
    {
      lat_deg = i;
    }
  }
  v = server.arg("site_t2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 60)))
    {
      lat_min = i;
    }
  }
  v = server.arg("site_t3");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i < 60)))
    {
      if ((lat_deg >= 0) && (lat_deg < 90) && (lat_min >= 0) && (lat_min < 60))
      {
        sprintf(temp, ":St%+03d:%02d:%02d#", lat_deg, lat_min, i);
        if (sign)
          temp[3] = '-';
        SetLX200(temp);
      }
    }
  }
  v = server.arg("site_e");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -200) && (i <= 8000)))
    {
      sprintf(temp, ":Se%+04d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("GNSSS");
  if (v != "")
  {
    SetLX200(":gs#");
  }
  v = server.arg("GNSST");
  if (v != "")
  {
    SetLX200(":gt#");
  }

}

