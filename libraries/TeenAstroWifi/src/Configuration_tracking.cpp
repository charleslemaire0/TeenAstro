#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
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
"<div class='panel' style='width:100%;max-width:27em'>\n"
"<div class='panel-title'>Tracking:</div>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Ts')\">" SIDEREAL_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Tl')\">" LUNAR_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Th')\">" SOLAR_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('Tt')\">" USER_CH "</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('on')\">On</button>\n"
"<button type='button' class='bbh' onpointerdown=\"g('off')\">Off</button><br/></div>\n"
"\r\n\n";

const char html_configTrackingOptions[] PROGMEM =
"<div class='bt'>Tracking Options</div>\n";
const char html_configTrackingDrift[] PROGMEM =
"<div class='bt'>Tracking Drift Options " USER_CH "</div>\n";

const char html_Opt_1[] PROGMEM =
"<form action='/configuration_tracking.htm'>\n"
"<select name='%s' onchange='this.form.submit()' >\n";

void TeenAstroWifi::handleConfigurationTracking()
{
  ta_MountStatus.updateMount();

  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  String data;
  processConfigurationTrackingGet();
  preparePage(data, ServerPage::Tracking);
  sendHtml(data);

  data += FPSTR(html_trackButton);
  sendHtml(data);

  data += FPSTR(html_trackInfo);
  sendHtml(data);

  data += FPSTR(html_indexTrackingInfo);
  sendHtml(data);
  data += "<div class='card'>";

  // Refraction tracking option
  data += FPSTR(html_configTrackingOptions);
  sprintf_P(temp, html_Opt_1, "trackr");
  data += temp;
  if (s_client->getRefractionEnabled(temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "n");
  temp1[0] == 'y' ? data += FPSTR(html_optOnSel) : data += FPSTR(html_optOnUnsel);
  temp1[0] == 'n' ? data += FPSTR(html_optOffSel) : data += FPSTR(html_optOffUnsel);
  data += "</select> Consider Refraction for Tracking</form><br/>\r\n";
  sendHtml(data);

  // Dual-axis tracking option
  if (!ta_MountStatus.isAltAz() && ta_MountStatus.getRateCompensation() != TeenAstroMountStatus::RC_UNKNOWN)
  {
    sprintf_P(temp, html_Opt_1, "trackboth");
    data += temp;
    ta_MountStatus.getRateCompensation() == TeenAstroMountStatus::RC_BOTH ? data += FPSTR(html_optOnSel) : data += FPSTR(html_optOnUnsel);
    ta_MountStatus.getRateCompensation() != TeenAstroMountStatus::RC_BOTH ? data += FPSTR(html_optOffSel) : data += FPSTR(html_optOffUnsel);
    data += "</select> Appply Tracking on both Axis</form><br/>\r\n";
    sendHtml(data);
  }

  // Drift rates
  if (ta_MountStatus.updateStoredTrackingRate())
  {
    data += FPSTR(html_configTrackingDrift);
    double Rate = ((double)ta_MountStatus.getStoredTrackingRateRa()) / 10000.0;
    sprintf_P(temp, html_configRateRA, Rate);
    data += temp;
    sendHtml(data);
    Rate = ((double)ta_MountStatus.getStoredTrackingRateDec()) / 10000.0;
    sprintf_P(temp, html_configRateDEC, Rate);
    data += temp;
    sendHtml(data);
  }

  data += "</div>"; // close card
  data += FPSTR(html_controlTrack);
  sendHtml(data);

  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationTrackingGet()
{
  String v;
  int i;
  float f;

  v = server.arg("trackr");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
      i == 1 ? s_client->enableRefraction(true) : s_client->enableRefraction(false);
  }
  v = server.arg("trackboth");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? s_client->setStepperMode(2) : s_client->setStepperMode(1);
      ta_MountStatus.updateMount(true);
    }
  }
  v = server.arg("RRA");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= -5) && (f <= 5)))
      s_client->setStoredTrackRateRA((long)(f * 10000));
  }
  v = server.arg("RDEC");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= -5) && (f <= 5)))
      s_client->setStoredTrackRateDec((long)(f * 10000));
  }
  v = server.arg("dt");
  if (v != "")
  {
    if (v == "on") s_client->enableTracking(true);
    else if (v == "off") s_client->enableTracking(false);
    else if (v == "f") s_client->incrementTrackRate();
    else if (v == "-") s_client->decrementTrackRate();
    else if (v == "r") s_client->resetTrackRate();
    else if (v == "Ts") s_client->setTrackRateSidereal();
    else if (v == "Tl") s_client->setTrackRateLunar();
    else if (v == "Th") s_client->setTrackRateSolar();
    else if (v == "Tt") s_client->setTrackRateUser();
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
