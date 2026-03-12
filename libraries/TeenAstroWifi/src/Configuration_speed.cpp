#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_speed

const char html_configRateD_0[] PROGMEM =
"<div class='bt'>Speed &amp; Acceleration</div>"
"<form action='/configuration_speed.htm'>"
"<select name='RD'>";
const char html_configRateD_1[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Active speed after main unit start)"
"</form>"
"\r\n";

const char html_configMaxRate[] PROGMEM =
"<form method='get' action='/configuration_speed.htm'>"
" <input value='%d' type='number' name='MaxR' min='32' max='4000'>"
"<button type='submit'>Upload</button>"
" (Maximum Slewing speed from 32x to 4000x)"
"</form>"
"\r\n";
const char html_configRate3[] PROGMEM =
"<form method='get' action='/configuration_speed.htm'>"
" <input value='%.0f' type='number' name='R3' min='1' max='255' step='1'>"
"<button type='submit'>Upload</button>"
" (Fast Recenter speed from 1 to 255x)"
"</form>"
"\r\n";
const char html_configRate2[] PROGMEM =
"<form method='get' action='/configuration_speed.htm'>"
" <input value='%.0f' type='number' name='R2' min='1' max='255' step='1'>"
"<button type='submit'>Upload</button>"
" (Medium Recenter speed from 1x to 255x)"
"</form>"
"\r\n";
const char html_configRate1[] PROGMEM =
"<form method='get' action='/configuration_speed.htm'>"
" <input value='%.0f' type='number' name='R1' min='1' max='255' step='1'>"
"<button type='submit'>Upload</button>"
" (Slow Recenter speed from 1x to 255x)"
"</form>"
"\r\n";
const char html_configRate0[] PROGMEM =
"<form method='get' action='/configuration_speed.htm'>"
" <input value='%.2f' type='number' name='R0' min='0.01' max='1' step='.01'>"
"<button type='submit'>Upload</button>"
" (Guiding speed from 0.01x to 1x)"
"</form>"
"\r\n";

const char html_configAcceleration[] PROGMEM =
"<form method='get' action='/configuration_speed.htm'>"
" <input value='%.1f' type='number' name='Acc' min='0.1' max='25' step='.1'>"
"<button type='submit'>Upload</button>"
" (Acceleration, number of degrees to reach the Max Speed from 0.1° to 25°)"
"</form>"
"<br />\r\n";


void TeenAstroWifi::handleConfigurationSpeed()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  String data;

  processConfigurationSpeedGet();
  preparePage(data, ServerPage::Speed);
  sendHtml(data);

  ta_MountStatus.updateMount();
  data += "<div class='card'>";

  // Default speed after start
  int deadband = 4;
  s_client->getDeadband(deadband);
  deadband = min(max(deadband, 0), 4);
  data += FPSTR(html_configRateD_0);
  const char* speedNames[] = { "Guiding", "Slow", "Medium", "Fast", "Max" };
  for (int k = 0; k < 5; k++)
  {
    char opt[60];
    sprintf(opt, "<option %svalue='%d'>%s</option>", (k == deadband) ? "selected " : "", k, speedNames[k]);
    data += opt;
  }
  data += FPSTR(html_configRateD_1);
  sendHtml(data);

  // Max rate
  int maxRate = 0;
  s_client->getMaxRate(maxRate);
  sprintf_P(temp, html_configMaxRate, maxRate);
  data += temp;
  sendHtml(data);

  // Speed rates 3..0
  float rate;
  const char* rateFmts[] = { html_configRate0, html_configRate1, html_configRate2, html_configRate3 };
  for (int idx = 3; idx >= 0; idx--)
  {
    rate = 0;
    s_client->getSpeedRate(idx, rate);
    sprintf_P(temp, rateFmts[idx], rate);
    data += temp;
    sendHtml(data);
  }

  // Acceleration
  float acc = 0;
  s_client->getAcceleration(acc);
  sprintf_P(temp, html_configAcceleration, acc);
  data += temp;
  data += "</div>"; // close card
  sendHtml(data);
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}

void TeenAstroWifi::processConfigurationSpeedGet()
{
  String v;
  int i;
  float f;

  v = server.arg("MaxR");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4000)))
      s_client->setMaxRate(i);
  }

  v = server.arg("Acc");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25)))
      s_client->setAcceleration(f);
  }

  // Speed rates R0..R3
  for (uint8_t idx = 0; idx <= 3; idx++)
  {
    char argName[4];
    sprintf(argName, "R%d", idx);
    v = server.arg(argName);
    if (v != "")
    {
      if (atof2((char*)v.c_str(), &f))
        s_client->setSpeedRate(idx, f);
    }
  }

  v = server.arg("RD");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4)))
      s_client->setDeadband(i);
  }
}
