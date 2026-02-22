#include "TeenAstroWifi.h"
#include "HtmlCommon.h"

// Decode focuser binary config (:FA#) — 200 base64 chars -> 150 bytes
#define FOC_CFG_BIN 150
#define FOC_CFG_B64 200
static const int8_t FOC_B64DEC[128] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
  52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
  15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
  -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
};
static bool focB64Decode(const char* in, int inLen, uint8_t* out) {
  if (inLen <= 0 || inLen % 4 != 0) return false;
  int o = 0;
  for (int i = 0; i < inLen; i += 4) {
    uint8_t c0 = (uint8_t)in[i], c1 = (uint8_t)in[i+1], c2 = (uint8_t)in[i+2], c3 = (uint8_t)in[i+3];
    if (c0 > 127 || c1 > 127 || c2 > 127 || c3 > 127) return false;
    int8_t v0 = FOC_B64DEC[c0], v1 = FOC_B64DEC[c1], v2 = FOC_B64DEC[c2], v3 = FOC_B64DEC[c3];
    if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) return false;
    uint32_t b = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12) | ((uint32_t)v2 << 6) | (uint32_t)v3;
    out[o++] = (uint8_t)(b >> 16); out[o++] = (uint8_t)(b >> 8); out[o++] = (uint8_t)(b);
  }
  return true;
}
static uint16_t getU16LE(const uint8_t* p, int off) { return (uint16_t)p[off] | ((uint16_t)p[off+1] << 8); }

// -----------------------------------------------------------------------------------
// configuration_focuser
const char html_configParkFocuser[] PROGMEM =
"Position: <br />"
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='Park' min='0' max='65535'>"
"<button type='submit'>Upload</button>"
" (Park position in resolution unit)"
"</form>"
"\r\n";
const char html_configMaxPositionFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='MaxPos' min='0' max='65535'>"
"<button type='submit'>Upload</button>"
" (Max position in resolution unit)"
"</form>"
"<br />\r\n";
const char html_configRotFocuser_1[] PROGMEM =
"<form action='/configuration_focuser.htm'>"
"<select name='Rot'>";
const char html_configRotFocuser_r[] PROGMEM =
"<option value ='0'>Direct</option>"
"<option selected value='1'>Reverse</option>";
const char html_configRotFocuser_d[] PROGMEM =
"<option selected value ='0'>Direct</option>"
"<option value='1'>Reverse</option>";
const char html_configRotFocuser_2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
"(change this value if the focuser moves in the wrong direction)"
"</form>"
"<br />\r\n";
const char html_configLowSpeedFocuser[] PROGMEM =
"Speed & Acceleration: <br />"
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='LowSpeed' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Manual Slewing speed from 1 to 999)"
"</form>"
"\r\n";
const char html_configHighSpeedFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='HighSpeed' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Goto Slewing speed from 1 to 999)"
"</form>"
"\r\n";
const char html_configGotoAccFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='GotoAcc' min='1' max='99'>"
"<button type='submit'>Upload</button>"
" (Acceleration for goto command from 1 to 99)"
"</form>"
"\r\n";
const char html_configManAccFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='ManAcc' min='1' max='99'>"
"<button type='submit'>Upload</button>"
" (Acceleration for manual movement from 1 to 99)"
"</form>"
"\r\n";
const char html_configManDecFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='Dcc' min='1' max='99'>"
"<button type='submit'>Upload</button>"
" (Deceleration for both manual and goto from 1 to 99)"
"</form>"
"\r\n<br/>";
const char html_configResolutionFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='Res' min='1' max='512'>"
"<button type='submit'>Upload</button>"
" (Sampling in steps from 1(high resolution) to 512(low resolution))"
"</form>"
"<br/>\r\n";
const char html_configStepRotFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='StepRot' min='10' max='800'>"
"<button type='submit'>Upload</button>"
" (steps per rotation, from 10 to 800)"
"</form>"
"\r\n";
const char html_configMuFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='MuF' min='4' max='128'>"
"<button type='submit'>Upload</button>"
" (Microsteps Focuser, valid value are 4, 8, 16, 32, 64, 128)"
"</form>"
"\r\n";
const char html_configLCFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='LcF' min='100' max='1600' step='10'>"
"<button type='submit'>Upload</button>"
" (Low Current Focuser, from 100mA to 1600mA)"
"</form>"
"\r\n";
const char html_configHCFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='HcF' min='100' max='1600' step='10'>"
"<button type='submit'>Upload</button>"
" (High Current Focuser, from 100mA to 1600mA)"
"</form>"
"<br/>\r\n";
const char html_configPosFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%s' type='text' name='Fn%d' maxlength='10'>"
" <input value='%d' type='number' name='Fp%d' min='0' max='65535' step='1'>"
"<button type='submit'>Upload</button>"
" "
"</form>"
"\r\n";



void TeenAstroWifi::handleConfigurationFocuser()
{
  if (busyGuard()) return;
  char temp[320] = "";
  String data;
  sendHtmlStart();
  processConfigurationFocuserGet();
  preparePage(data, ServerPage::Focuser);
  sendHtml(data);
  data += "<div class='card'>";

  char raw[220];
  if (s_client->getFocuserAllConfig(raw, sizeof(raw)) != LX200_VALUEGET)
  {
    data += "<p>Focuser communication error</p>";
    data += "</div>";
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    return;
  }

  int len = (int)strlen(raw);
  if (raw[len - 1] == '#') len--;
  uint8_t pkt[FOC_CFG_BIN];
  if (len != FOC_CFG_B64 || !focB64Decode(raw, len, pkt))
  {
    data += "<p>Focuser binary decode error</p>";
    data += "</div>";
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    return;
  }
  uint8_t xorChk = 0;
  for (int i = 0; i < FOC_CFG_BIN - 1; i++) xorChk ^= pkt[i];
  if (xorChk != pkt[FOC_CFG_BIN - 1])
  {
    data += "<p>Focuser checksum error</p>";
    data += "</div>";
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    return;
  }

  int park      = (int)getU16LE(pkt, 0);
  int maxPos    = (int)getU16LE(pkt, 2);
  int lowSpeed  = (int)getU16LE(pkt, 4);
  int highSpeed = (int)getU16LE(pkt, 6);
  int gotoAcc   = (int)pkt[8];
  int manAcc    = (int)pkt[9];
  int manDec    = (int)pkt[10];
  bool reverse  = (pkt[11] != 0);
  unsigned int micro = pkt[12];
  unsigned int resolution = getU16LE(pkt, 13);
  unsigned int curr = pkt[15];
  unsigned int steprot = getU16LE(pkt, 16);

  sprintf_P(temp, html_configParkFocuser, park);
  data += temp; sendHtml(data);
  sprintf_P(temp, html_configMaxPositionFocuser, maxPos);
  data += temp; sendHtml(data);
  sprintf_P(temp, html_configLowSpeedFocuser, lowSpeed);
  data += temp; sendHtml(data);
  sprintf_P(temp, html_configHighSpeedFocuser, highSpeed);
  data += temp; sendHtml(data);
  sprintf_P(temp, html_configGotoAccFocuser, gotoAcc);
  data += temp; sendHtml(data);
  sprintf_P(temp, html_configManAccFocuser, manAcc);
  data += temp; sendHtml(data);
  sprintf_P(temp, html_configManDecFocuser, manDec);
  data += temp; sendHtml(data);

  data += PSTR("Resolution: <br />");
  sprintf_P(temp, html_configResolutionFocuser, resolution);
  data += temp; sendHtml(data);
  data += PSTR("Rotation: <br />");
  data += FPSTR(html_configRotFocuser_1);
  data += reverse ? FPSTR(html_configRotFocuser_r) : FPSTR(html_configRotFocuser_d);
  data += FPSTR(html_configRotFocuser_2);
  sendHtml(data);
  data += PSTR("Motor: <br />");
  sprintf_P(temp, html_configStepRotFocuser, steprot);
  data += temp;
  sprintf_P(temp, html_configMuFocuser, (int)pow(2., (double)micro));
  data += temp;
  sprintf_P(temp, html_configHCFocuser, curr * 10);
  data += temp; sendHtml(data);

  data += PSTR("Userdefined Position: <br />");
  data += PSTR("to remove a position set an empty name <br />");
  for (int k = 0; k < 10; k++)
  {
    int base = 18 + k * 13;
    uint16_t pos = getU16LE(pkt, base);
    char id[12];
    memcpy(id, &pkt[base + 2], 11);
    id[11] = '\0';
    if (id[0] == '\0')
      sprintf_P(temp, html_configPosFocuser, "undefined", k, 0, k);
    else
      sprintf_P(temp, html_configPosFocuser, id, k, (int)pos, k);
    data += temp;
    sendHtml(data);
  }

  data += "</div>"; // close card
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}

void TeenAstroWifi::processConfigurationFocuserGet()
{
  String v;
  int i;
  float f;

  v = server.arg("Park");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 65535)))
      s_client->setFocuserPark((int)f);
  }

  v = server.arg("MaxPos");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 65535)))
      s_client->setFocuserMaxPos((int)f);
  }

  v = server.arg("LowSpeed");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 999)))
      s_client->setFocuserLowSpeed(i);
  }

  v = server.arg("HighSpeed");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 999)))
      s_client->setFocuserHighSpeed(i);
  }

  v = server.arg("GotoAcc");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 99)))
      s_client->setFocuserGotoAcc(i);
  }

  v = server.arg("ManAcc");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 99)))
      s_client->setFocuserManAcc(i);
  }

  v = server.arg("Dcc");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 99)))
      s_client->setFocuserDecel(i);
  }

  v = server.arg("Rot");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
      s_client->setFocuserRotation(i);
  }

  v = server.arg("Res");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 512)))
      s_client->setFocuserResolution(i);
  }

  v = server.arg("StepRot");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 10) && (i <= 800)))
      s_client->setFocuserStepPerRot(i);
  }

  v = server.arg("MuF");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 4) && (i <= 128)))
      s_client->setFocuserMicro((int)log2(i));
  }

  v = server.arg("HcF");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 1600)))
      s_client->setFocuserCurrent(i / 10);
  }

  // User positions
  for (int k = 0; k < 10; k++)
  {
    char argPos[6], argName[6];
    sprintf(argPos, "Fp%d", k);
    v = server.arg(argPos);
    if (v != "")
    {
      if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 65535)))
      {
        sprintf(argName, "Fn%d", k);
        String nameVal = server.arg(argName);
        s_client->setFocuserUserPos(k, (int)f, nameVal.c_str());
      }
    }
  }
}
