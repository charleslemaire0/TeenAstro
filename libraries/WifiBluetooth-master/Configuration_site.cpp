#include "config.h"
#include "WifiBluetooth.h"
// -----------------------------------------------------------------------------------
// configuration_Site

const char html_configSiteSelect1[] =
"Selected Site:<br/>"
"<form method='post' action='/configuration_site.htm'>"
"<select style='width:11em' name='site_select'>";
const char html_configSiteSelect2[] =
"</select>"
"<button type='submit'>Upload</button>"
" (Select your predefined site)"
"</form>"
"\r\n";
const char html_configSiteName[] =
"Selected Site definition: <br />"
"<form method='get' action='/configuration_site.htm'>"
" <input value='%s' style='width:10.25em' type='text' name='site_n' maxlength='14'>"
"<button type='submit'>Upload</button>"
" (Edit the name of the selected site)"
"</form>"
"\r\n";
const char html_configLongWE1[] =
"<form method='get' action='/configuration_site.htm'>"
"<select style='width:5em' name='site_g0'>";
const char html_configLongWE2[] =
"</select>";
const char html_configLongDeg[] =
" <input value='%s' type='number' name='site_g1' min='0' max='179'>&nbsp;&deg;&nbsp;";
const char html_configLongMin[] =
" <input value='%s' type='number' name='site_g2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;"
"<button type='submit'>Upload</button>"
" (Longitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configLatNS1[] =
"<form method='get' action='/configuration_site.htm'>"
"<select style='width:5em' name='site_t0'>";
const char html_configLatNS2[] =
"</select>";
const char html_configLatDeg[] =
" <input value='%s' type='number' name='site_t1' min='0' max='90'>&nbsp;&deg;&nbsp;";
const char html_configLatMin[] =
" <input value='%s' type='number' name='site_t2' min='0' max='59'>&nbsp;'&nbsp;&nbsp;"
"<button type='submit'>Upload</button>"
" (Latitude, in degree and minute)"
"</form>"
"\r\n";
const char html_configElev[] =
"<form method='get' action='/configuration_site.htm'>"
" <input value='%s' type='number' name='site_e' min='-200' max='8000'>"
"<button type='submit'>Upload</button>"
" (Elevation, in meter min -200m max 8000m)"
"</form>"
"<br />\r\n";

#ifdef OETHS
void wifibluetooth::handleConfigurationSite(EthernetClient *client) {
#else
void wifibluetooth::handleConfigurationSite() {
#endif
  Ser.setTimeout(WebTimeout);
  serialRecvFlush();

  char temp[320] = "";
  char temp1[80] = "";
  char temp2[80] = "";
  String data;

  processConfigurationSiteGet();
  preparePage(data, 3);
  
  if (sendCommand(":W?#", temp1))
  {
    int selectedsite = 0;
    if ((atoi2(temp1, &selectedsite)) && ((selectedsite >= 0) && (selectedsite <= 3)))
    {
      char m[16]; char n[16]; char o[16]; char p[16];
      sendCommand(":GM#", m); sendCommand(":GN#", n); sendCommand(":GO#", o); sendCommand(":GP#", p);
      data += html_configSiteSelect1;
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
      data += html_configSiteSelect2;
      // name

      if (!sendCommand(":Gn#", temp1)) strcpy(temp1, "error!");
      sprintf(temp, html_configSiteName, temp1);
      data += temp;
#ifdef OETHS
      client->print(data); data = "";
#endif
      // Longitude
      if (!sendCommand(":Gg#", temp1)) strcpy(temp1, "+000*00");
      temp1[4] = 0; // deg. part only
      data += html_configLongWE1;
      temp1[0] == '+' ? data += "<option selected value='0'>West</option>" : data += "<option value='0'>West</option>";
      temp1[0] == '-' ? data += "<option selected value='1'>East</option>" : data += "<option value='1'>East</option>";
      data += html_configLongWE2;
      temp1[0] = '0'; // sign
      sprintf(temp, html_configLongDeg, temp1);
      data += temp;
      sprintf(temp, html_configLongMin, (char*)&temp1[5]);
      data += temp;
#ifdef OETHS
      client->print(data); data = "";
#endif

      // Latitude
      if (!sendCommand(":Gt#", temp1)) strcpy(temp1, "+00*00");
      temp1[3] = 0; // deg. part only
      data += html_configLatNS1;
      temp1[0] == '+' ? data += "<option selected value='0'>North</option>" : data += "<option value='0'>North</option>";
      temp1[0] == '-' ? data += "<option selected value='1'>Sud</option>" : data += "<option value='1'>Sud</option>";
      data += html_configLatNS2;
      temp1[0] = '0'; // remove +
      sprintf(temp, html_configLatDeg, temp1);
      data += temp;
      sprintf(temp, html_configLatMin, (char*)&temp1[4]);
      data += temp;
#ifdef OETHS
      client->print(data); data = "";
#endif
      // Elevation
      if (!sendCommand(":Ge#", temp1)) strcpy(temp1, "+000");
      if (temp1[0] == '+') temp1[0] = '0';
      sprintf(temp, html_configElev, temp1);
      data += temp;
#ifdef OETHS
      client->print(data); data = "";
#endif
    }
  }
#ifdef OETHS
  client->print(data); data = "";
#endif

  strcpy(temp, "</div></div></body></html>");
  data += temp;

#ifdef OETHS
  client->print(data); data = "";
#else
  server.send(200, "text/html", data);
#endif
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
    Ser.print(temp);
  }
  // name
  v = server.arg("site_n");
  if (v != "") {
    sprintf(temp, ":Sn%s#", (char*)v.c_str());
    Ser.print(temp);
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
        Ser.print(temp);
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
        Ser.print(temp);
      }
    }
  }
  v = server.arg("site_e");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -200) && (i <= 8000))) {
      sprintf(temp, ":Se%+04d#", i);
      Ser.print(temp);
    }
  }
  // clear any possible response
  temp[Ser.readBytesUntil('#', temp, 20)] = 0;
}

