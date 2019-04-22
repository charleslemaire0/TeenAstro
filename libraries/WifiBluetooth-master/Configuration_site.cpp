#include <TeenAstroLX200io.h>
#include "WifiBluetooth.h"
// -----------------------------------------------------------------------------------
// configuration_Site

const char html_configSiteSelect1[] PROGMEM =
"Selected Site:<br/>"
"<form method='post' action='/configuration_site.htm'>"
"<select onchange='this.form.submit()' style='width:11em' name='site_select'>";
const char html_configSiteSelect2[] PROGMEM =
"</select>"
" (Select your predefined site)"
"</form>"
"\r\n";
const char html_configSiteName[] PROGMEM =
"Selected Site definition: <br />"
"<form method='get' action='/configuration_site.htm'>"
" <input value='%s' style='width:10.25em' type='text' name='site_n' maxlength='14'>"
"<button type='submit'>Upload</button>"
" (Edit the name of the selected site)"
"</form>"
"\r\n";
const char html_configLongWE1[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>"
"<select style='width:5em' name='site_g0'>";
const char html_configLongWE2[] PROGMEM =
"</select>";
const char html_configLongDeg[] PROGMEM =
" <input value='%s' type='number' name='site_g1' min='0' max='179'>&nbsp;&deg;&nbsp;";
const char html_configLongMin[] PROGMEM =
" <input value='%s' type='number' name='site_g2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;"
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
" <input value='%s' type='number' name='site_t1' min='0' max='90'>&nbsp;&deg;&nbsp;";
const char html_configLatMin[] PROGMEM =
" <input value='%s' type='number' name='site_t2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;"
"<button type='submit'>Upload</button>"
" (Latitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configElev[] PROGMEM =
"<form method='get' action='/configuration_site.htm'>"
" <input value='%s' type='number' name='site_e' min='-200' max='8000'>"
"<button type='submit'>Upload</button>"
" (Elevation, in meter min -200m max 8000m)"
"</form>"
"<br />\r\n";


void wifibluetooth::handleConfigurationSite() {
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[80] = "";
  char temp2[80] = "";
  String data;
  processConfigurationSiteGet();
  preparePage(data, 3);
  sendHtml(data);
  if (GetLX200(":W?#", temp1, sizeof(temp1)) == LX200VALUEGET)
  {
    int selectedsite = 0;
    if ((atoi2(temp1, &selectedsite)) && ((selectedsite >= 0) && (selectedsite <= 3)))
    {
      char m[16]; char n[16]; char o[16]; char p[16];
      GetLX200(":GM#", m, sizeof(m));
      GetLX200(":GN#", n, sizeof(n));
      GetLX200(":GO#", o, sizeof(o));
      GetLX200(":GP#", p, sizeof(p));
      data += FPSTR(html_configSiteSelect1);
      selectedsite == 0 ? data += "<option selected value='0'>" : data += "<option value='0'>";
      sprintf(temp, "%s</option>", m);
      data += temp;
      selectedsite == 1 ? data += "<option selected value='1'>" : data += "<option value='1'>";
      sprintf(temp, "%s</option>", n);
      data += temp;
      selectedsite == 2 ? data += "<option selected value='2'>" : data += "<option value='2'>";
      sprintf(temp, "%s</option>", o);
      data += temp;
      selectedsite == 3 ? data += "<option selected value='3'>" : data += "<option value='3'>";
      sprintf(temp, "%s</option>", p);
      data += temp;
      data += FPSTR(html_configSiteSelect2);
      // name

      if (GetLX200(":Gn#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "error!");
      sprintf_P(temp, html_configSiteName, temp1);
      data += temp;

      // Latitude
      if (GetLX200(":Gt#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "+00*00");
      temp1[3] = 0; // deg. part only
      data += FPSTR(html_configLatNS1);
      temp1[0] == '+' ? data += "<option selected value='0'>North</option>" : data += "<option value='0'>North</option>";
      temp1[0] == '-' ? data += "<option selected value='1'>Sud</option>" : data += "<option value='1'>Sud</option>";
      data += FPSTR(html_configLatNS2);
      temp1[0] = '0'; // remove +
      sprintf_P(temp, html_configLatDeg, temp1);
      data += temp;
      sprintf_P(temp, html_configLatMin, (char*)&temp1[4]);
      data += temp;

      // Longitude
      if (GetLX200(":Gg#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "+000*00");
      temp1[4] = 0; // deg. part only
      data += FPSTR(html_configLongWE1);
      temp1[0] == '+' ? data += "<option selected value='0'>West</option>" : data += "<option value='0'>West</option>";
      temp1[0] == '-' ? data += "<option selected value='1'>East</option>" : data += "<option value='1'>East</option>";
      data += FPSTR(html_configLongWE2);
      temp1[0] = '0'; // sign
      sprintf_P(temp, html_configLongDeg, temp1);
      data += temp;
      sprintf_P(temp, html_configLongMin, (char*)&temp1[5]);
      data += temp;
      // Elevation
      if (GetLX200(":Ge#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "+000");
      if (temp1[0] == '+') temp1[0] = '0';
      sprintf_P(temp, html_configElev, temp1);
      data += temp;

    }
  }
  strcpy(temp, "</div></div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void wifibluetooth::processConfigurationSiteGet() {
  String v;
  int i;
  float f;
  char temp[20] = "";
  // selected site
  v = server.arg("site_select");
  if (v != "") {
    sprintf(temp, ":W%s#", (char*)v.c_str());
    SetLX200(temp);
  }
  // name
  v = server.arg("site_n");
  if (v != "") {
    sprintf(temp, ":Sn%s#", (char*)v.c_str());
    SetLX200(temp);
  }
  // Location
  int long_deg = -999;
  int sign = -1;
  v = server.arg("site_g0");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) { sign = i; }
  }
  v = server.arg("site_g1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -180) && (i <= 180)))
    {
      long_deg = sign ? -i: i;
    }
  }
  v = server.arg("site_g2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 60))) {
      if ((long_deg >= -180) && (long_deg <= 180)) {
        sprintf(temp, ":Sg%+04d*%02d#", long_deg, i);
        SetLX200(temp);
      }
    }
  }
  int lat_deg = -999;
  v = server.arg("site_t0");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) { sign = i; }
  }
  v = server.arg("site_t1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 90))) {
      lat_deg = sign ? -i : i; 
    }
  }
  v = server.arg("site_t2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 60))) {
      if ((lat_deg >= -90) && (lat_deg <= 90)) {
        v = server.arg("site_t0");
        sprintf(temp, ":St%+03d*%02d#", lat_deg, i);
        SetLX200(temp);
      }
    }
  }
  v = server.arg("site_e");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -200) && (i <= 8000))) {
      sprintf(temp, ":Se%+04d#", i);
      SetLX200(temp);
    }
  }
}

