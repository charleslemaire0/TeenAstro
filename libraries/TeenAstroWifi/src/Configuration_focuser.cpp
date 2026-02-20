#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
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
  char temp1[80] = "";
  String data;
  sendHtmlStart();
  processConfigurationFocuserGet();
  preparePage(data, ServerPage::Focuser);
  sendHtml(data);
  data += "<div class='card'>";

  // Read focuser config via named method
  if (s_client->getFocuserConfigRaw(temp1, sizeof(temp1)) == LX200_VALUEGET && temp1[0] == '~')
  {
    int park      = (int)strtol(&temp1[1], NULL, 10);
    int maxPos    = (int)strtol(&temp1[7], NULL, 10);
    int lowSpeed  = (int)strtol(&temp1[13], NULL, 10);
    int highSpeed = (int)strtol(&temp1[17], NULL, 10);
    int gotoAcc   = (int)strtol(&temp1[21], NULL, 10);
    int manAcc    = (int)strtol(&temp1[25], NULL, 10);

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
  }

  bool reverse = false;
  unsigned int micro = 3, resolution = 100, curr = 100, steprot = 100;
  if (s_client->readFocuserMotor(reverse, micro, resolution, curr, steprot))
  {
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
    sprintf_P(temp, html_configMuFocuser, (int)pow(2., micro));
    data += temp;
    sprintf_P(temp, html_configHCFocuser, curr * 10);
    data += temp; sendHtml(data);
  }

  // User-defined positions
  data += PSTR("Userdefined Position: <br />");
  data += PSTR("to remove a position set an empty name <br />");
  for (int k = 0; k < 10; k++)
  {
    if (s_client->getFocuserUserPos(k, temp1, sizeof(temp1)) == LX200_VALUEGET && temp1[0] != 0)
    {
      if (temp1[0] == '0')
      {
        sprintf_P(temp, html_configPosFocuser, "undefined", k, 0, k);
        data += temp;
      }
      else if (temp1[0] == 'P')
      {
        char id[11];
        memcpy(id, &temp1[7], sizeof(id));
        int pos = (int)strtol(&temp1[1], NULL, 10);
        sprintf_P(temp, html_configPosFocuser, id, k, pos, k);
        data += temp;
        sendHtml(data);
      }
    }
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
