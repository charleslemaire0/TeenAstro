#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_motors



const char html_on_1[] PROGMEM = "<option selected value='1'>On</option>";
const char html_on_2[] PROGMEM = "<option value='1'>On</option>";
const char html_off_1[] PROGMEM = "<option selected value='2'>Off</option>";
const char html_off_2[] PROGMEM = "<option value='2'>Off</option>";

const char html_configRotAxis_1[] PROGMEM =
"<form action='/configuration_motors.htm'>"
"<select name='mrot%d'>";
const char html_configRotAxis_r[] PROGMEM =
"<option value ='0'>Direct</option>"
"<option selected value='1'>Reverse</option>";
const char html_configRotAxis_d[] PROGMEM =
"<option selected value ='0'>Direct</option>"
"<option value='1'>Reverse</option>";
const char html_configRotAxis_2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Rotation Axis%d)"
"</form>"
"\r\n";


const char html_configSettleTime[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mw' min='0' max='20'>"
"<button type='submit'>Upload</button>"
" (Settle Time, in seconds from 0 to 20)"
"</form>"
"\r\n";
const char html_configBlAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mbl%d' min='0' max='999'>"
"<button type='submit'>Upload</button>"
" (Backlash Axis%d, in arc-seconds from 0 to 999)"
"</form>"
"\r\n";
const char html_configBlRateAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mblr%d' min='16' max='64'>"
"<button type='submit'>Upload</button>"
" (Backlash Rate Axis%d, in sideral speed from 16 to 64)"
"</form>"
"\r\n";
const char html_configGeAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%.3f' type='number' name='mge%d' min='1' max='60000' step='.001'>"
"<button type='submit'>Upload</button>"
" (Gear Axis%d, from 1 to 60000)"
"</form>"
"\r\n";
const char html_configStAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mst%d' min='1' max='400'>"
"<button type='submit'>Upload</button>"
" (Steps per Rotation Axis%d, from 1 to 400)"
"</form>"
"\r\n";
const char html_configMuAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mmu%d' min='8' max='256'>"
"<button type='submit'>Upload</button>"
" (Microsteps Axis%d, valid value are 8, 16, 32, 64, 128, 256)"
"</form>"
"\r\n";
const char html_configLCAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%u' type='number' name='mlc%d' min='200' max='2800' step='200'>"
"<button type='submit'>Upload</button>"
" (Low Current Axis%d, from 200mA to 2800mA)"
"</form>"
"\r\n";
const char html_configHCAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%u' type='number' name='mhc%d' min='200' max='2800' step='200'>"
"<button type='submit'>Upload</button>"
" (High Current Axis%d, from 200mA to 2800mA)"
"</form>"
"\r\n";

const char html_configSilentAxis_1[] PROGMEM =
"<form action='/configuration_motors.htm'>"
"<select name='ms%d'>";
const char html_configSilentAxis_r[] PROGMEM =
"<option value ='0'>Off</option>"
"<option selected value='1'>On</option>";
const char html_configSilentAxis_d[] PROGMEM =
"<option selected value ='0'>Off</option>"
"<option value='1'>On</option>";

const char html_configSilentAxis_2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Silent Axis%d)"
"</form>"
"\r\n";

const char html_reboot_t[] PROGMEM =
"<br/><form method='get' action='/configuration_motors.htm'>"
"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
"<button type='submit'>Continue</button>"
"</form><br/><br/><br/><br/>"
"\r\n";



bool restartRequired_t1 = false;

void TeenAstroWifi::handleConfigurationMotors()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;

  processConfigurationMotorsGet();
  preparePage(data, ServerPage::Motors);
  sendHtml(data);
  if (restartRequired_t1)
  {
    data += FPSTR(html_reboot_t);
    data += "</div></div></body></html>";
    sendHtml(data);
    sendHtmlDone(data);
    restartRequired_t1 = false;
    delay(1000);
    return;
  }
  //update


    //Axis1
  data += "<div class='bt'> Motor: <br/> </div>";

  if (GetLX200(":GXOS#", temp2, sizeof(temp2)) == LX200_VALUEGET)
  {
    long wt = (long)strtol(&temp2[0], NULL, 10);
    sprintf_P(temp, html_configSettleTime, wt);
    data += temp;
    sendHtml(data);
  }
  bool reverse = false;
  uint8_t silent = false;
  if (readReverseLX200(1, reverse) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configRotAxis_1, 1);
    data += temp;
    data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
    sprintf_P(temp, html_configRotAxis_2, 1);
    data += temp;
    sendHtml(data);
  }
  reverse = false;
  if (readReverseLX200(2, reverse) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configRotAxis_1, 2);
    data += temp;
    data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
    sprintf_P(temp, html_configRotAxis_2, 2);
    data += temp;
    sendHtml(data);
  }
  float gear = 0;
  if (readTotGearLX200(1, gear) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configGeAxis, gear, 1, 1);
    data += temp;
    sendHtml(data);
  }
  if (readTotGearLX200(2, gear) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configGeAxis, gear, 2, 2);
    data += temp;
    sendHtml(data);
  }
  float step;

  if (readStepPerRotLX200(1, step) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configStAxis, (int)step, 1, 1);
    data += temp;
    sendHtml(data);
  }
  if (readStepPerRotLX200(2, step) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configStAxis, (int)step, 2, 2);
    data += temp;
    sendHtml(data);
  }
  uint8_t micro;
  if (readMicroLX200(1, micro) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), 1, 1);
    data += temp;
    sendHtml(data);
  }
  if (readMicroLX200(2, micro) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), 2, 2);
    data += temp;
    sendHtml(data);
  }
  float backlashAxis;
  if (readBacklashLX200(1, backlashAxis) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configBlAxis, (int)backlashAxis, 1, 1);
    data += temp;
    sendHtml(data);
  }
  if (readBacklashLX200(2, backlashAxis) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configBlAxis, (int)backlashAxis, 2, 2);
    data += temp;
    sendHtml(data);
  }
  float backlashAxisRate;
  if (readBacklashRateLX200(1, backlashAxisRate) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configBlRateAxis, (int)backlashAxisRate, 1, 1);
    data += temp;
    sendHtml(data);
  }
  if (readBacklashRateLX200(2, backlashAxisRate) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configBlRateAxis, (int)backlashAxisRate, 2, 2);
    data += temp;
    sendHtml(data);
  }
  unsigned int lowC;
  if (readLowCurrLX200(1, lowC) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configLCAxis, lowC, 1, 1);
    data += temp;
    sendHtml(data);
  }

  if (readLowCurrLX200(2, lowC) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configLCAxis, lowC, 2, 2);
    data += temp;
    sendHtml(data);
  }
  unsigned int highC;
  if (readHighCurrLX200(1, highC) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configHCAxis, highC, 1, 1);
    data += temp;
    sendHtml(data);
  }
  if (readHighCurrLX200(2, highC) == LX200_VALUEGET)
  {
    sprintf_P(temp, html_configHCAxis, highC, 2, 2);
    data += temp;
    sendHtml(data);
  }
  const char* board = ta_MountStatus.getVb();
  if (board[0] - '0' > 1)
  {
    if (readSilentStepLX200(1, silent) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configSilentAxis_1, 1);
      data += temp;
      data += silent ? FPSTR(html_configSilentAxis_r) : FPSTR(html_configSilentAxis_d);
      sprintf_P(temp, html_configSilentAxis_2, 1);
      data += temp;
      sendHtml(data);
    }
    if (readSilentStepLX200(2, silent) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configSilentAxis_1, 2);
      data += temp;
      data += silent ? FPSTR(html_configSilentAxis_r) : FPSTR(html_configSilentAxis_d);
      sprintf_P(temp, html_configSilentAxis_2, 2);
      data += temp;
      sendHtml(data);
    }
  }
  
  strcpy(temp, "</div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationMotorsGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";


  v = server.arg("mw");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 20)))
    {
      sprintf(temp, ":SXOS,%02d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("MaxR");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4000)))
    {
      sprintf(temp, ":SXRX,%04d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("Acc");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25)))
    {
      sprintf(temp, ":SXRA,%04d#", (int)(f * 10));
      SetLX200(temp);
    }
  }

  v = server.arg("R3");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 255)))
    {
      sprintf(temp, ":SXR3,%03d#", (int)f);
      SetLX200(temp);
    }
  }

  v = server.arg("R2");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 255)))
    {
      sprintf(temp, ":SXR2,%03d#", (int)f);
      SetLX200(temp);
    }
  }

  v = server.arg("R1");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 255)))
    {
      sprintf(temp, ":SXR1,%03d#", (int)f);
      SetLX200(temp);
    }
  }

  v = server.arg("R0");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.01) && (f <= 100)))
    {
      sprintf(temp, ":SXR0,%03d#", (int)(f * 100));
      SetLX200(temp);
    }
  }

  v = server.arg("RD");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4)))
    {
      sprintf(temp, ":SXRD,X#");
      temp[6] = '0' + i;
      SetLX200(temp);
    }
  }

  v = server.arg("mrot1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      writeReverseLX200(1, i);
    }
  }

  v = server.arg("mrot2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      writeReverseLX200(2, i);
    }
  }

  v = server.arg("mge1");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 60000)))
    {
      writeTotGearLX200(1, f);
    }
  }

  v = server.arg("mge2");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 60000)))
    {
      writeTotGearLX200(2, f);
    }
  }

  v = server.arg("mst1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400)))
    {
      writeStepPerRotLX200(1, i);
    }
  }

  v = server.arg("mst2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400)))
    {
      writeStepPerRotLX200(2, i);
    }
  }

  v = server.arg("mmu1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 8) && (i <= 256)))
    {
      writeMicroLX200(1, (float)((int)log2(i)));
    }
  }

  v = server.arg("mmu2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 8) && (i <= 256)))
    {
      writeMicroLX200(2, (float)((int)log2(i)));
    }
  }

  v = server.arg("mbl1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      writeBacklashLX200(1, (float)i);
    }
  }

  v = server.arg("mbl2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      writeBacklashLX200(2, (float)i);
    }
  }

  v = server.arg("mblr1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 16) && (i <= 64)))
    {
      writeBacklashRateLX200(1, (float)i);
    }
  }

  v = server.arg("mblr2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 16) && (i <= 64)))
    {
      writeBacklashRateLX200(2, (float)i);
    }
  }

  v = server.arg("mlc1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 200) && (i <= 2800)))
    {
      writeLowCurrLX200(1, i);
    }
  }
  v = server.arg("mlc2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 200) && (i <= 2800)))
    {
      writeLowCurrLX200(2, i);
    }
  }

  v = server.arg("mhc1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 200) && (i <= 2800)))
    {
      writeHighCurrLX200(1, i);
    }
  }
  v = server.arg("mhc2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 200) && (i <= 2800)))
    {
      writeHighCurrLX200(2, i);
    }
  }

  //silent mode
  v = server.arg("ms1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      writeSilentStepLX200(1, i);
    }
  }
  v = server.arg("ms2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      writeSilentStepLX200(2, i);
    }
  }

  int ut_hrs = -999;
  v = server.arg("u1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -13) && (i <= 13)))
    {
      ut_hrs = i;
    }
  }
  v = server.arg("u2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i == 00) || (i == 30) || (i == 45)))
    {
      if ((ut_hrs >= -13) && (ut_hrs <= 13))
      {
        sprintf(temp, ":SG%+03d:%02d#", ut_hrs, i);
        SetLX200(temp);
      }
    }
  }
}

