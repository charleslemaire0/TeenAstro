#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_telescope


const char html_configMount_1[] PROGMEM =
"<div class='bt'> Equatorial Mount Type: <br/> </div>"
"<form action='/configuration_telescope.htm'>"
"<select name='mount' onchange='this.form.submit()' >";
const char html_configMount_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configTracking_1[] PROGMEM =
"<div class='bt'> Corrected Tracking: <br/> </div>"
"<form action='/configuration_telescope.htm'>"
"<select name='tracking' onchange='this.form.submit()' >";
const char html_configTracking_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configPolar_1[] PROGMEM =
"<div class='bt'> Polar Alignment: <br/> </div>"
"<form action='/configuration_telescope.htm'>"
"<select name='polar' onchange='this.form.submit()' >";
const char html_configPolar_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configRateD_0[] PROGMEM =
"<div class='bt'> Speed & Acceleration: <br/> </div>"
"<form action='/configuration_telescope.htm'>"
"<select name='RD'>";
const char html_configRateD_1[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Active speed after main unit start)"
"</form>"
"\r\n";

const char html_configMaxRate[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='MaxR' min='32' max='4000'>"
"<button type='submit'>Upload</button>"
" (Maximum Slewing speed from 32x to 4000x)"
"</form>"
"\r\n";
const char html_configRate3[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%.0f' type='number' name='R3' min='1' max='255' step='1'>"
"<button type='submit'>Upload</button>"
" (Fast Recenter speed speed from 1 to 255x)"
"</form>"
"\r\n";
const char html_configRate2[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%.0f' type='number' name='R2' min='1' max='255' step='1'>"
"<button type='submit'>Upload</button>"
" (Medium Recenter speed from 1x to 255x)"
"</form>"
"\r\n";
const char html_configRate1[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%.0f' type='number' name='R1' min='1' max='255' step='1'>"
"<button type='submit'>Upload</button>"
" (Slow Recenter speed from 1x to 255x)"
"</form>"
"\r\n";
const char html_configRate0[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%.2f' type='number' name='R0' min='0.01' max='1' step='.01'>"
"<button type='submit'>Upload</button>"
" (Guiding speed from 0.01x to 1x)"
"</form>"
"\r\n";

const char html_configAcceleration[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%.1f' type='number' name='Acc' min='0.1' max='25' step='.1'>"
"<button type='submit'>Upload</button>"
" (Acceleration, number of degrees to reach the Max Speed from 0.1° to 25°)"
"</form>"
"<br />\r\n";
const char html_configRotAxis_1[] PROGMEM =
"<form action='/configuration_telescope.htm'>"
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

const char html_configBlAxis[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='mbl%d' min='0' max='999'>"
"<button type='submit'>Upload</button>"
" (Backlash Axis%d, in arc-seconds from 0 to 999)"
"</form>"
"\r\n";
const char html_configGeAxis[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='mge%d' min='1' max='60000'>"
"<button type='submit'>Upload</button>"
" (Gear Axis%d, from 1 to 60000)"
"</form>"
"\r\n";
const char html_configStAxis[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='mst%d' min='1' max='400'>"
"<button type='submit'>Upload</button>"
" (Steps per Rotation Axis%d, from 1 to 400)"
"</form>"
"\r\n";
const char html_configMuAxis[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='mmu%d' min='8' max='256'>"
"<button type='submit'>Upload</button>"
" (Microsteps Axis%d, valid value are 8, 16, 32, 64, 128, 256)"
"</form>"
"\r\n";
const char html_configLCAxis[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='mlc%d' min='100' max='2000' step='10'>"
"<button type='submit'>Upload</button>"
" (Low Current Axis%d, from 100mA to 2000mA)"
"</form>"
"\r\n";
const char html_configHCAxis[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='mhc%d' min='100' max='2000' step='10'>"
"<button type='submit'>Upload</button>"
" (High Current Axis%d, from 100mA to 2000mA)"
"</form>"
"\r\n";

const char html_configSilentAxis_1[] PROGMEM =
"<form action='/configuration_telescope.htm'>"
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

const char html_configMinAlt[] PROGMEM =
"<div class='bt'> Limits: <br/> </div>"
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='hl' min='-30' max='30'>"
"<button type='submit'>Upload</button>"
" (Horizon, in degrees +/- 30)"
"</form>"
"\r\n";
const char html_configMaxAlt[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='ol' min='60' max='91'>"
"<button type='submit'>Upload</button>"
" (Overhead, in degrees 60 to 90, set 91 to deactivate)"
"</form>"
"\r\n";
const char html_configPastMerE[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='el' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
" (Past Meridian when East of the pier, in degrees +/-45)"
"</form>"
"\r\n";
const char html_configPastMerW[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='wl' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
" (Past Meridian when West of the pier, in degrees +/-45)"
"</form>"
"<br />\r\n";

const char html_reboot_t[] PROGMEM =
"<br/><form method='get' action='/configuration_telescope.htm'>"
"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
"<button type='submit'>Continue</button>"
"</form><br/><br/><br/><br/>"
"\r\n";
bool restartRequired_t = false;

void TeenAstroWifi::handleConfigurationTelescope()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;

  processConfigurationTelescopeGet();
  preparePage(data, 4);
  sendHtml(data);
  if (restartRequired_t)
  {
    data += FPSTR(html_reboot_t);
    data += "</div></div></body></html>";
    sendHtml(data);
    sendHtmlDone(data);
    restartRequired_t = false;
    delay(1000);
    return;
  }
  //update
  ta_MountStatus.updateMount();

  data += FPSTR(html_configMount_1);
  ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM ? data += "<option selected value='1'>German</option>" : data += "<option value='1'>German</option>";
  ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_FORK ? data += "<option selected value='2'>Fork</option>" : data += "<option value='2'>Fork</option>";
  ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM ? data += "<option selected value='3'>Alt Az</option>" : data += "<option value='3'>Alt Az</option>";
  ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT ? data += "<option selected value='4'>Alt Az Fork</option>" : data += "<option value='4'>Alt Az Fork</option>";
  data += FPSTR(html_configMount_2);
  sendHtml(data);

  if (!ta_MountStatus.isAltAz())
  {
    data += FPSTR(html_configTracking_1);
    ta_MountStatus.isTrackingCorrected() ? data += "<option selected value='1'>On</option>" : data += "<option value='1'>On</option>";
    !ta_MountStatus.isTrackingCorrected() ? data += "<option selected value='2'>Off</option>" : data += "<option value='2'>Off</option>";
    data += FPSTR(html_configTracking_2);
    sendHtml(data);

    data += FPSTR(html_configPolar_1);
    if (GetLX200(":GXAp#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "a");
    temp1[0] == 'a' ? data += "<option selected value='1'>Apparent</option>" : data += "<option value='1'>Apparent</option>";
    temp1[0] == 't' ? data += "<option selected value='2'>True</option>" : data += "<option value='2'>True</option>";
    data += FPSTR(html_configPolar_2);
    sendHtml(data);
  }


  if (GetLX200(":GXRD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "4"); int val = temp1[0] - '0';
  val = min(max(val, 0), 4);
  data += FPSTR(html_configRateD_0);
  val == 0 ? data += "<option selected value='0'>Guiding</option>" : data += "<option value='0'>Guiding</option>";
  val == 1 ? data += "<option selected value='1'>Slow</option>" : data += "<option value='1'>Slow</option>";
  val == 2 ? data += "<option selected value='2'>Medium</option>" : data += "<option value='2'>Medium</option>";
  val == 3 ? data += "<option selected value='3'>Fast</option>" : data += "<option value='3'>Fast</option>";
  val == 4 ? data += "<option selected value='4'>Max</option>" : data += "<option value='4'>Max</option>";
  data += FPSTR(html_configRateD_1);
  sendHtml(data);

  if (GetLX200(":GXRX#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int maxRate = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMaxRate, maxRate);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GXR3#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float Rate3 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate3, Rate3);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GXR2#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float Rate2 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate2, Rate2);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GXR1#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float Rate1 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate1, Rate1);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GXR0#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float Rate0 = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configRate0, Rate0);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GXRA#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float acc = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configAcceleration, acc);
  data += temp;
  sendHtml(data);

  //Axis1
  data += "<div class='bt'> Motor: <br/> </div>";
  bool reverse = false;
  uint8_t silent = false;
  readReverseLX200(1, reverse);
  sprintf_P(temp, html_configRotAxis_1, 1);
  data += temp;
  data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
  sprintf_P(temp, html_configRotAxis_2, 1);
  data += temp;
  sendHtml(data);
  reverse = false;
  readReverseLX200(2, reverse);
  sprintf_P(temp, html_configRotAxis_1, 2);
  data += temp;
  data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
  sprintf_P(temp, html_configRotAxis_2, 2);
  data += temp;
  sendHtml(data);
  float gear = 0;
  readTotGearLX200(1, gear);
  sprintf_P(temp, html_configGeAxis, (int)gear, 1, 1);
  data += temp;
  sendHtml(data);
  readTotGearLX200(2, gear);
  sprintf_P(temp, html_configGeAxis, (int)gear, 2, 2);
  data += temp;
  sendHtml(data);
  float step;
  readStepPerRotLX200(1, step);
  sprintf_P(temp, html_configStAxis, (int)step, 1, 1);
  data += temp;
  sendHtml(data);
  readStepPerRotLX200(2, step);
  sprintf_P(temp, html_configStAxis, (int)step, 2, 2);
  data += temp;
  sendHtml(data);
  uint8_t micro;
  readMicroLX200(1, micro);
  sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), 1, 1);
  data += temp;
  sendHtml(data);
  readMicroLX200(2, micro);
  sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), 2, 2);
  data += temp;
  sendHtml(data);
  float backlashAxis;
  readBacklashLX200(1, backlashAxis);
  sprintf_P(temp, html_configBlAxis, (int)backlashAxis, 1, 1);
  data += temp;
  sendHtml(data);
  readBacklashLX200(2, backlashAxis);
  sprintf_P(temp, html_configBlAxis, (int)backlashAxis, 2, 2);
  data += temp;
  sendHtml(data);
  uint8_t lowC;
  readLowCurrLX200(1, lowC);
  sprintf_P(temp, html_configLCAxis, lowC * 10, 1, 1);
  data += temp;
  sendHtml(data);
  readLowCurrLX200(2, lowC);
  sprintf_P(temp, html_configLCAxis, lowC * 10, 2, 2);
  data += temp;
  sendHtml(data);
  uint8_t highC;
  readHighCurrLX200(1, highC);
  sprintf_P(temp, html_configHCAxis, highC * 10, 1, 1);
  data += temp;
  sendHtml(data);
  readHighCurrLX200(2, highC);
  sprintf_P(temp, html_configHCAxis, highC * 10, 2, 2);
  data += temp;
  const char* board = ta_MountStatus.getVb();
  if (board[0] - '0' > 1)
  {
    readSilentStepLX200(1, silent);
    sprintf_P(temp, html_configSilentAxis_1, 1);
    data += temp;
    data += silent ? FPSTR(html_configSilentAxis_r) : FPSTR(html_configSilentAxis_d);
    sprintf_P(temp, html_configSilentAxis_2, 1);
    data += temp;
    sendHtml(data);
    readSilentStepLX200(2, silent);
    sprintf_P(temp, html_configSilentAxis_1, 2);
    data += temp;
    data += silent ? FPSTR(html_configSilentAxis_r) : FPSTR(html_configSilentAxis_d);
    sprintf_P(temp, html_configSilentAxis_2, 2);
    data += temp;
    sendHtml(data);
  }
  data += "<br />";

  // Overhead and Horizon Limits
  if (GetLX200(":GXLH#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int minAlt = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMinAlt, minAlt);
  data += temp;
  sendHtml(data);
  if (GetLX200(":GXLO#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int maxAlt = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMaxAlt, maxAlt);
  data += temp;
  sendHtml(data);
  // Meridian Limits
  if (GetLX200(":GXLE#", temp1, sizeof(temp1)) == LX200VALUEGET && GetLX200(":GXLW#", temp2, sizeof(temp2)) == LX200VALUEGET)
  {
    int degPastMerE = (int)strtol(&temp1[0], NULL, 10);
    degPastMerE = round((degPastMerE*15.0) / 60.0);
    sprintf_P(temp, html_configPastMerE, degPastMerE);
    data += temp;
    int degPastMerW = (int)strtol(&temp2[0], NULL, 10);
    degPastMerW = round((degPastMerW*15.0) / 60.0);
    sprintf_P(temp, html_configPastMerW, degPastMerW);
    data += temp;
  }
  else data += "<br />\r\n";
  strcpy(temp, "</div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationTelescopeGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";

  v = server.arg("mount");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 4)))
    {
      sprintf(temp, ":S!X#");
      temp[3] = '0' + i;
      SetLX200(temp);
      restartRequired_t = true;
    }
  }

  v = server.arg("tracking");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? SetLX200(":Tc#") : SetLX200(":Tn#");
    }
  }

  v = server.arg("polar");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? SetLX200(":SXAp,a#") : SetLX200(":SXAp,t#");
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
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000)))
    {
      writeTotGearLX200(1, (float)i);
    }
  }

  v = server.arg("mge2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000)))
    {
      writeTotGearLX200(2, (float)i);
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
      writeBacklashLX200(2, (uint8_t)i);
    }
  }

  v = server.arg("mlc1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      writeLowCurrLX200(1, i / 10);
    }
  }
  v = server.arg("mlc2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      writeLowCurrLX200(2, i / 10);
    }
  }

  v = server.arg("mhc1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      writeHighCurrLX200(1, i / 10);
    }
  }
  v = server.arg("mhc2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      writeHighCurrLX200(2, i / 10);
    }
  }

  // Backlash Limits
  v = server.arg("b1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      writeBacklashLX200(1, i);
    }
  }
  v = server.arg("b2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      writeBacklashLX200(2, i);
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
  // Overhead and Horizon Limits
  v = server.arg("ol");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 60) && (i <= 91)))
    {
      sprintf(temp, ":SXLO,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("hl");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -30) && (i <= 30)))
    {
      sprintf(temp, ":SXLH,%d#", i);
      SetLX200(temp);
    }
  }

  // Meridian Limits
  v = server.arg("el");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = round((i*60.0) / 15.0);
      sprintf(temp, ":SXLE,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("wl");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = round((i*60.0) / 15.0);
      sprintf(temp, ":SXLW,%d#", i);
      SetLX200(temp);
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

