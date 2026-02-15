#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_speed


const char html_configRateD_0[] PROGMEM =
"<div class='bt'> Speed & Acceleration: <br/> </div>"
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
" (Acceleration, number of degrees to reach the Max Speed from 0.1? to 25?)"
"</form>"
"<br />\r\n";


void TeenAstroWifi::handleConfigurationSpeed()
{
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;

  processConfigurationSpeedGet();
  preparePage(data, ServerPage::Speed);
  sendHtml(data);

  //update
  ta_MountStatus.updateMount();

  if (s_client->get(":GXRD#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "4"); int val = temp1[0] - '0';
  val = min(max(val, 0), 4);
  data += FPSTR(html_configRateD_0);
  val == 0 ? data += "<option selected value='0'>Guiding</option>" : data += "<option value='0'>Guiding</option>";
  val == 1 ? data += "<option selected value='1'>Slow</option>" : data += "<option value='1'>Slow</option>";
  val == 2 ? data += "<option selected value='2'>Medium</option>" : data += "<option value='2'>Medium</option>";
  val == 3 ? data += "<option selected value='3'>Fast</option>" : data += "<option value='3'>Fast</option>";
  val == 4 ? data += "<option selected value='4'>Max</option>" : data += "<option value='4'>Max</option>";
  data += FPSTR(html_configRateD_1);
  sendHtml(data);

  if (s_client->get(":GXRX#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); int maxRate = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMaxRate, maxRate);
  data += temp;
  sendHtml(data);

  if (s_client->get(":GXR3#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); float Rate3 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate3, Rate3);
  data += temp;
  sendHtml(data);

  if (s_client->get(":GXR2#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); float Rate2 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate2, Rate2);
  data += temp;
  sendHtml(data);

  if (s_client->get(":GXR1#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); float Rate1 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate1, Rate1);
  data += temp;
  sendHtml(data);

  if (s_client->get(":GXR0#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); float Rate0 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate0, Rate0);
  data += temp;
  sendHtml(data);

  if (s_client->get(":GXRA#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); float acc = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configAcceleration, acc);
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationSpeedGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";

  v = server.arg("MaxR");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4000)))
    {
      sprintf(temp, ":SXRX,%04d#", i);
      s_client->set(temp);
    }
  }

  v = server.arg("Acc");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25)))
    {
      sprintf(temp, ":SXRA,%04d#", (int)(f * 10));
      s_client->set(temp);
    }
  }

  v = server.arg("R3");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 255)))
    {
      sprintf(temp, ":SXR3,%03d#", (int)f);
      s_client->set(temp);
    }
  }

  v = server.arg("R2");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 255)))
    {
      sprintf(temp, ":SXR2,%03d#", (int)f);
      s_client->set(temp);
    }
  }

  v = server.arg("R1");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 255)))
    {
      sprintf(temp, ":SXR1,%03d#", (int)f);
      s_client->set(temp);
    }
  }

  v = server.arg("R0");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.01) && (f <= 100)))
    {
      sprintf(temp, ":SXR0,%03d#", (int)(f * 100));
      s_client->set(temp);
    }
  }

  v = server.arg("RD");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4)))
    {
      sprintf(temp, ":SXRD,X#");
      temp[6] = '0' + i;
      s_client->set(temp);
    }
  }
}

