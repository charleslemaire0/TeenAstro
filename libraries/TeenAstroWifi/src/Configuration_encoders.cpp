#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_encoders

const char html_configEncoders_1[] PROGMEM =
"<div class='bt'>Encoders Sync Mode</div>"
"<form action='/configuration_encoders.htm'>"
"<select name='smE' onchange='this.form.submit()' >";
const char html_configEncoders_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configPPDAxis[] PROGMEM =
"<div class='bt'>Encoders of Instrument Axis %d</div>"
"<form method='get' action='/configuration_encoders.htm'>"
" <input value='%.2f' type='number' name='ppdEa%d' min='0' max='3600' step='0.01'>"
"<button type='submit'>Upload</button>"
" (Pulse per degree axis %d from 0 to 3600)"
"</form>"
"\r\n";

const char html_configRotEAxis_1[] PROGMEM =
"<form action='/configuration_encoders.htm'>"
"<select name='mrotE%d'>";
const char html_configRotEAxis_r[] PROGMEM =
"<option value ='0'>Direct</option>"
"<option selected value='1'>Reverse</option>";
const char html_configRotEAxis_d[] PROGMEM =
"<option selected value ='0'>Direct</option>"
"<option value='1'>Reverse</option>";
const char html_configRotEAxis_2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Rotation Encoder Axis%d)"
"</form>"
"\r\n";

void TeenAstroWifi::handleConfigurationEncoders()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  if (processConfigurationEncodersGet())
  {
    ta_MountStatus.invalidateAllConfig();
    sendRedirectAfterMutation("/configuration_encoders.htm");
    return;
  }
  sendHtmlStart();
  char temp[416] = "";
  String data;

  preparePage(data, ServerPage::Encoders);
  sendHtml(data);

  ta_MountStatus.updateAllConfig();
  data += "<div class='card'>";

  if (!ta_MountStatus.hasConfig())
  {
    data += "<p>Mount config unavailable</p></div>";
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    return;
  }

  // Sync mode selector
  const uint8_t syncMode = ta_MountStatus.getCfgEncSyncMode();
  data += FPSTR(html_configEncoders_1);
  const char* modeLabels[] = { "OFF", "60'", "30'", "15'", "8'", "4'", "2'", "ON" };
  for (uint8_t k = 0; k < 8; k++)
  {
    char opt[60];
    snprintf(opt, sizeof(opt), "<option %svalue='%d'>%s</option>", (k == syncMode) ? "selected " : "", k, modeLabels[k]);
    data += opt;
  }
  data += FPSTR(html_configEncoders_2);
  sendHtml(data);

  // Per-axis: PPD + rotation (:GXCS# stores ppd×100)
  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    const float ppd = (ax == 1 ? (float)ta_MountStatus.getCfgPPD1()
                               : (float)ta_MountStatus.getCfgPPD2()) / 100.0f;
    snprintf_P(temp, sizeof(temp), html_configPPDAxis, ax, ppd, ax, ax);
    data += temp;
    sendHtml(data);

    const bool reverse = ta_MountStatus.getCfgEncReverse(ax - 1);
    snprintf_P(temp, sizeof(temp), html_configRotEAxis_1, ax);
    data += temp;
    data += reverse ? FPSTR(html_configRotEAxis_r) : FPSTR(html_configRotEAxis_d);
    snprintf_P(temp, sizeof(temp), html_configRotEAxis_2, ax);
    data += temp;
    sendHtml(data);
  }

  data += "</div>"; // close card
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}

bool TeenAstroWifi::processConfigurationEncodersGet()
{
  bool any = false;
  String v;
  int i;
  float f;

  v = server.arg("smE");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 7)))
      s_client->writeEncoderAutoSync(i);
  }

  // Per-axis PPD and rotation
  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    char argPPD[10], argRot[10];
    snprintf(argPPD, sizeof(argPPD), "ppdEa%d", ax);
    snprintf(argRot, sizeof(argRot), "mrotE%d", ax);

    v = server.arg(argPPD);
    if (v != "")
    {
      any = true;
      if ((atof2((char*)v.c_str(), &f)) && ((f > 0) && (f <= 3600)))
        s_client->writePulsePerDegree(ax, f * 100);
    }
    v = server.arg(argRot);
    if (v != "")
    {
      any = true;
      if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
        s_client->writeEncoderReverse(ax, i);
    }
  }

  return any;
}
