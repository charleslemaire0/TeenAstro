#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_tracking
#define SIDEREAL_CH "&#9733;"
#define LUNAR_CH "&#9790;"
#define SOLAR_CH "&#9737;"
#define USER_CH "&#9732;"

const char  html_trackButton[] PROGMEM = "<script>\n"
"function s(key,v1) {\n"
"var xhttp = new XMLHttpRequest();\n"
"xhttp.open('GET', 'track.txt?'+key+'='+v1, true);\n"
"xhttp.send();\n"
"}\n"
"function g(v1){s('dt',v1);}\n"
"</script>\n";

const char html_trackInfo[] PROGMEM =
"<script>\n"
"var auto1=setInterval(load,1000);\n"
"function load() {\n"
"  var xhttp = new XMLHttpRequest();\n"
"  xhttp.onreadystatechange = function() {\n"
"    if (this.readyState == 4 && this.status == 200) {\n"
"      document.getElementById('TrackingInfo').innerHTML = this.responseText;\n"
"    }\n"
"  };\n"
"  xhttp.open('GET', 'trackinfo.txt', true);\n"
"  xhttp.send();\n"
"}\n"
"</script>\n";

const char html_indexTrackingInfo[] PROGMEM =
"<div id='TrackingInfo'>\n</div><br/>\n";

const char html_configRateRA[] PROGMEM =
"<form method='get' action='/configuration_tracking.htm'>"
" <input value='%f' type='number' name='RRA' min='-5' max='+5' step='0.0001'>"
"<button type='submit'>Upload</button>"
" (user defined RA drift in sec/SI)"
"</form>"
"\r\n";
const char html_configRateDEC[] PROGMEM =
"<form method='get' action='/configuration_tracking.htm'>"
" <input value='%f' type='number' name='RDEC' min='-5' max='+5' step='0.0001'>"
"<button type='submit'>Upload</button>"
" (user defined DEC drift in arcsec/SI)"
"</form>"
"\r\n";

const char html_controlTrack[] PROGMEM =
"<div class='b1' style='width: 27em'>\n"
"<div class='bct' align='left'>Tracking:</div>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Ts')\" type='submit'>" SIDEREAL_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Tl')\" type='submit'>" LUNAR_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Th')\" type='submit'>" SOLAR_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Tt')\" type='submit'>" USER_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('on')\" type='submit'>On</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('off')\" type='submit'>Off</button><br/></div>\n"
//"<button type='button' class='bbh' style='width: 2.6em' onpointerdown=\"g('-')\" type='submit'>" MINUS_CH "</button>"
//"<button type='button' class='bbh' style='width: 2.6em' onpointerdown=\"g('f')\" type='submit'>" PLUS_CH " </button>"
//"<button type='button' class='bbh' onpointerdown=\"g('r')\" type='submit'>Reset</button>""</div>"
"<br class='clear' />\r\n\n";

const char html_compTrack[] PROGMEM =
"<div class='b1' style='width: 27em'>\n"
"<div class='bct' align='left'>Tracking Compensation:</div>\n"
"<button type='button' class='bbh' onpointerdown=\"g('T0')\" type='submit'>NONE</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('T1')\" type='submit'>RA AXIS</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('T2')\" type='submit'>BOTH</button><br/></div>\n"
"<br class='clear' />\r\n";

const char html_configRefraction[] PROGMEM =
"<div class='bt'> Refraction Options: <br/> </div>\n";

const char html_Opt_1[] PROGMEM =
"<form action='/configuration_tracking.htm'>\n"
"<select name='%s' onchange='this.form.submit()' >\n";

const char html_configTrackingOptions[] PROGMEM =
"<div class='bt'> Tracking Options: <br/> </div>\n";
const char html_configTrackingDrift[] PROGMEM =
"<div class='bt'> Tracking Drift Options " USER_CH " : <br/> </div>\n";

const char html_on_1[] PROGMEM = "<option selected value='1'>On</option>";
const char html_on_2[] PROGMEM = "<option value='1'>On</option>\n";
const char html_off_1[] PROGMEM = "<option selected value='2'>Off</option>";
const char html_off_2[] PROGMEM = "<option value='2'>Off</option>\n";

void TeenAstroWifi::handleConfigurationTracking()
{
  //update mount
  ta_MountStatus.updateMount();

  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;
  processConfigurationTrackingGet();
  preparePage(data, ServerPage::Tracking);
  sendHtml(data);
  // button handling script
  data += FPSTR(html_trackButton);
  sendHtml(data);

  data += FPSTR(html_trackInfo);
  sendHtml(data);

  data += FPSTR(html_indexTrackingInfo);
  sendHtml(data);

  data += FPSTR(html_configTrackingOptions);
  sprintf_P(temp, html_Opt_1, "trackr");
  data += temp;
  if (GetLX200(":GXrt#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "n");
  temp1[0] == 'y' ? data += FPSTR(html_on_1) : data += FPSTR(html_on_2);
  temp1[0] == 'n' ? data += FPSTR(html_off_1) : data += FPSTR(html_off_2);
  data += "</select> Consider Refraction for Tracking</form><br/>\r\n";;
  sendHtml(data);

  if (!ta_MountStatus.isAltAz())
  {
    sprintf_P(temp, html_Opt_1, "alignr");
    data += temp;
    if (GetLX200(":GXAc#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "n");
    temp1[0] == 'y' ? data += FPSTR(html_on_1) : data += FPSTR(html_on_2);
    temp1[0] == 'n' ? data += FPSTR(html_off_1) : data += FPSTR(html_off_2);
    data += "</select> Consider Alignment for Tracking</form><br/>\r\n";;
    sendHtml(data);
  }
  if (ta_MountStatus.updateStoredTrackingRate())
  {
    data += FPSTR(html_configTrackingDrift);
    float Rate = ((float)ta_MountStatus.getStoredTrackingRateRa()) / 10000.0;
    sprintf_P(temp, html_configRateRA, Rate);
    data += temp;
    sendHtml(data);
    Rate = ((float)ta_MountStatus.getStoredTrackingRateDec()) / 10000.0;
    sprintf_P(temp, html_configRateDEC, Rate);
    data += temp;
    sendHtml(data);
  }

  // Tracking control ----------------------------------------
  data += FPSTR(html_controlTrack);
  sendHtml(data);

  data += FPSTR(html_compTrack);
  strcpy(temp, "</div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationTrackingGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";

  v = server.arg("trackr");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? SetLX200(":SXrt,y#") : SetLX200(":SXrt,n#");
    }
  }
  v = server.arg("alignr");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? SetLX200(":SXAc,y#") : SetLX200(":SXAc,n#");
    }
  }
  v = server.arg("RRA");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= -5) && (f <= 5)))
    {
      sprintf(temp, ":SXRe,%05ld#", (long)(f * 10000));
      SetLX200(temp);
    }
  }
  v = server.arg("RDEC");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= -5) && (f <= 5)))
    {
      sprintf(temp, ":SXRf,%05ld#", (long)(f * 10000));
      SetLX200(temp);
    }
  }
  v = server.arg("dt");
  if (v != "")
  {
    // Tracking control
    if (v == "on") SetLX200(":Te#");
    else if (v == "off") SetLX200(":Td#");
    else if (v == "f") SetLX200(":T+#"); // 0.02hz faster
    else if (v == "-") SetLX200(":T-#"); // 0.02hz slower
    else if (v == "r") SetLX200(":TR#"); // reset

    else if (v == "Ts") SetLX200(":TQ#"); // sidereal
    else if (v == "Tl") SetLX200(":TL#"); // lunar
    else if (v == "Th") SetLX200(":TS#"); // solar
    else if (v == "Tt") SetLX200(":TT#"); // user defined

    else if (v == "T0") SetLX200(":T0#"); // None
    else if (v == "T1") SetLX200(":T1#"); // RA
    else if (v == "T2") SetLX200(":T2#"); // BOTH
  }

}

void TeenAstroWifi::trackAjax()
{
  processConfigurationTrackingGet();
  server.send(200, "text/html", "");
}
void TeenAstroWifi::trackinfoAjax()
{
  String data;
  addTrackingInfo(data);
  server.send(200, "text/plain", data);
}


