#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_telescope

const char html_configEncoders_1[] PROGMEM =
"<div class='bt'> Encoders Sync Mode: <br/> </div>"
"<form action='/configuration_encoders.htm'>"
"<select name='smE' onchange='this.form.submit()' >";
const char html_configEncoders_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configPPDAxis1[] PROGMEM =
"<div class='bt'> Encoders of Instrument Axis 1: <br/> </div>"
"<form method='get' action='/configuration_encoders.htm'>"
" <input value='%.2f' type='number' name='ppdEa1' min='0' max='3600' step='0.01'>"
"<button type='submit'>Upload</button>"
" (Pulse per degree axis 1 from 0 to 3600)"
"</form>"
"\r\n";

const char html_configPPDAxis2[] PROGMEM =
"<div class='bt'> Encoders of Instrument Axis 2: <br/> </div>"
"<form method='get' action='/configuration_encoders.htm'>"
" <input value='%.2f' type='number' name='ppdEa2' min='0' max='3600' step='0.01'>"
"<button type='submit'>Upload</button>"
" (Pulse per degree axis 2 from 0 to 3600)"
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
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;

  processConfigurationEncodersGet();
  preparePage(data, ServerPage::Encoders);
  sendHtml(data);
  ta_MountStatus.updateMount();
  uint8_t EncodersyncMode = 0;
  if (readEncoderAutoSync(EncodersyncMode) == LX200_VALUEGET)
  {
    data += FPSTR(html_configEncoders_1);
    EncodersyncMode == 0 ? data += PSTR("<option selected value='0'>OFF</option>") : data += PSTR("<option value='0'>OFF</option>");
    EncodersyncMode == 1 ? data += PSTR("<option selected value='1'>60'</option>") : data += PSTR("<option value='1'>60'</option>");
    EncodersyncMode == 2 ? data += PSTR("<option selected value='2'>30'</option>") : data += PSTR("<option value='2'>30'</option>");
    EncodersyncMode == 3 ? data += PSTR("<option selected value='3'>15'</option>") : data += PSTR("<option value='3'>15'</option>");
    EncodersyncMode == 4 ? data += PSTR("<option selected value='4'>8'</option>") : data += PSTR("<option value='4'>8'</option>");
    EncodersyncMode == 5 ? data += PSTR("<option selected value='5'>4'</option>") : data += PSTR("<option value='5'>4'</option>");
    EncodersyncMode == 6 ? data += PSTR("<option selected value='6'>2'</option>") : data += PSTR("<option value='6'>2'</option>");
    EncodersyncMode == 7 ? data += PSTR("<option selected value='7'>ON</option>") : data += PSTR("<option value='7'>ON</option>");
    data += FPSTR(html_configEncoders_2);
    sendHtml(data);
  }
  // PPD and Roatio Encoders
  float ppd = 0;
  bool reverse = false;

  if (readPulsePerDegreeLX200(1, ppd) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configPPDAxis1, ppd/100.0);
    data += temp;
    sendHtml(data);
  }
  if (readEncoderReverseLX200(1, reverse) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configRotEAxis_1, 1);
    data += temp;
    data += reverse ? FPSTR(html_configRotEAxis_r) : FPSTR(html_configRotEAxis_d);
    sprintf_P(temp, html_configRotEAxis_2, 1);
    data += temp;
    sendHtml(data);
  }

  if (readPulsePerDegreeLX200(2, ppd) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configPPDAxis2, ppd/100.0);
    data += temp;
    sendHtml(data);
  }
  if (readEncoderReverseLX200(2, reverse) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configRotEAxis_1, 2);
    data += temp;
    data += reverse ? FPSTR(html_configRotEAxis_r) : FPSTR(html_configRotEAxis_d);
    sprintf_P(temp, html_configRotEAxis_2, 2);
    data += temp;
    sendHtml(data);
  }

  strcpy(temp, "</div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationEncodersGet()
{
  String v;
  int i;
  float f;

  v = server.arg("smE");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 7)))
    {
      writeEncoderAutoSync(i);
    }
  }
  v = server.arg("ppdEa1");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f > 0) && (f <= 3600)))
    {
      writePulsePerDegreeLX200(1, f*100);
    }
  }
  v = server.arg("ppdEa2");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f > 0) && (f <= 3600)))
    {
      writePulsePerDegreeLX200(2, f*100);
    }
  }
  v = server.arg("mrotE1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      writeEncoderReverseLX200(1, i);
    }
  }
  v = server.arg("mrotE2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      writeEncoderReverseLX200(2, i);
    }
  }
}

