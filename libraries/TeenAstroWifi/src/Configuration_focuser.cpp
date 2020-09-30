#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
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

  char temp[320] = "";
  char temp1[80] = "";
  char temp2[80] = "";
  String data;
  sendHtmlStart();
  processConfigurationFocuserGet();
  preparePage(data, 5);
  sendHtml(data);

  if (GetLX200(":F~#", temp1, sizeof(temp1)) == LX200VALUEGET && temp1[0] == '~')
  {
    int park = (int)strtol(&temp1[1], NULL, 10);
    int maxPos = (int)strtol(&temp1[7], NULL, 10);
    int lowSpeed = (int)strtol(&temp1[13], NULL, 10);
    int highSpeed = (int)strtol(&temp1[17], NULL, 10);
    int gotoAcc = (int)strtol(&temp1[21], NULL, 10);
    int manAcc = (int)strtol(&temp1[25], NULL, 10);
    int dec = (int)strtol(&temp1[29], NULL, 10);
    sprintf_P(temp, html_configParkFocuser, park);
    data += temp;
    sendHtml(data);
    sprintf_P(temp, html_configMaxPositionFocuser, maxPos);
    data += temp;
    sendHtml(data);
    sprintf_P(temp, html_configLowSpeedFocuser, lowSpeed);
    data += temp;
    sendHtml(data);
    sprintf_P(temp, html_configHighSpeedFocuser, highSpeed);
    data += temp;
    sendHtml(data);
    sprintf_P(temp, html_configGotoAccFocuser, gotoAcc);
    data += temp;
    sendHtml(data);
    sprintf_P(temp, html_configManAccFocuser, manAcc);
    data += temp;
    sendHtml(data);
    //sprintf_P(temp, html_configManDecFocuser, dec);
    //data += temp;
  }
  bool reverse = false;
  unsigned int micro = 3;
  unsigned int resolution = 100;
  unsigned int curr = 100;
  unsigned int steprot = 100;
  if (readFocuserMotor(reverse, micro, resolution, curr, steprot))
  {
    data += "Resolution: <br />";
    sprintf_P(temp, html_configResolutionFocuser, resolution);
    data += temp;
    sendHtml(data);
    data += "Rotation: <br />";
    data += FPSTR(html_configRotFocuser_1);
    data += reverse ? FPSTR(html_configRotFocuser_r) : FPSTR(html_configRotFocuser_d);
    data += FPSTR(html_configRotFocuser_2);
    sendHtml(data);
    data += "Motor: <br />";
    sprintf_P(temp, html_configStepRotFocuser, steprot);
    data += temp;
    sprintf_P(temp, html_configMuFocuser, (int)pow(2., micro));
    data += temp;
    sprintf_P(temp, html_configHCFocuser, curr);
    data += temp;
    sendHtml(data);
  }
  data += "Userdefined Position: <br />";
  data += "to remove a position set an empty name <br />";
  for (int k = 0; k < 10; k++)
  {
    sprintf(temp, ":Fx%d#", k);
    GetLX200(temp, temp1, sizeof(temp1));
    if (temp1[0] != 0)
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

  strcpy(temp, "</div></div></body></html>");
  data += temp;

  sendHtml(data);
  sendHtmlDone(data);

}

void TeenAstroWifi::processConfigurationFocuserGet()
{
  String v;
  int i;
  float f;
  char temp[50] = "";

  v = server.arg("Park");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 65535)))
    {
      sprintf(temp, ":F0,%05d#", (int)f);
      SetLX200(temp);
    }
  }

  v = server.arg("MaxPos");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 65535)))
    {
      sprintf(temp, ":F1,%05d#", (int)f);
      SetLX200(temp);
    }
  }

  v = server.arg("LowSpeed");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 999)))
    {
      sprintf(temp, ":F2,%d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("HighSpeed");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 999)))
    {
      sprintf(temp, ":F3,%d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("GotoAcc");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 99)))
    {
      sprintf(temp, ":F4,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("ManAcc");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 99)))
    {
      sprintf(temp, ":F5,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("Dcc");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 99)))
    {
      sprintf(temp, ":F6,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("Rot");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      sprintf(temp, ":F7,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("Res");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 512)))
    {
      sprintf(temp, ":F8,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("StepRot");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 10) && (i <= 800)))
    {
      sprintf(temp, ":Fr,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("MuF");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 4) && (i <= 128)))
    {
      sprintf(temp, ":Fm,%d#", (int)log2(i));
      SetLX200(temp);
    }
  }

  v = server.arg("HcF");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 1600)))
    {
      sprintf(temp, ":Fc,%d#", i / 10);
      SetLX200(temp);
    }
  }
  for (int k = 0; k < 10; k++)
  {
    sprintf(temp, "Fp%d", k);
    v = server.arg(temp);
    if (v != "")
    {
      if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 65535)))
      {
        sprintf(temp, "Fn%d", k);
        v = server.arg(temp);
        sprintf(temp, ":Fs%d,%05d_%s#", k, (int)f, (char*)v.c_str());
        SetLX200(temp);
      }
    }
  }
  //Ser.flush();
  //serialRecvFlush();
}

