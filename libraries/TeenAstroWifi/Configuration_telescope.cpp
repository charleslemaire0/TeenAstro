#include <TeenAstroLX200io.h>
#include "config.h"
#include "WifiBluetooth.h"
// -----------------------------------------------------------------------------------
// configuration_telescope


const char html_configMount_1[] PROGMEM =
"Equatorial Mount Type: <br />"
"<form action='/configuration_telescope.htm'>"
"<select name='mount' onchange='this.form.submit()' >";
const char html_configMount_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configMaxRate[] PROGMEM =
"Speed & Acceleration: <br />"
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%d' type='number' name='MaxR' min='32' max='4000'>"
"<button type='submit'>Upload</button>"
" (Maximum Slewing speed from 32x to 4000x)"
"</form>"
"\r\n";
const char html_configGuideRate[] PROGMEM =
"<form method='get' action='/configuration_telescope.htm'>"
" <input value='%.2f' type='number' name='GuideR' min='0.01' max='1' step='.01'>"
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
const char html_configMinAlt[] PROGMEM =
"Limits: <br />"
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

void wifibluetooth::handleConfigurationTelescope()
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

  //if (!sendCommand(":GX90#", temp1, sizeof(temp1))) strcpy(temp1, "0"); int mountType = (int)strtol(&temp1[0], NULL, 10);
  //sprintf(temp, html_configMount, mountType);
  //data += temp;
  if (GetLX200(":GX92#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int maxRate = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMaxRate, maxRate);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GX90#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float guideRate = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configGuideRate, guideRate);
  data += temp;
  sendHtml(data);

  if (GetLX200(":GXE2#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); float acc = (float)strtof(&temp1[0], NULL);
  sprintf_P(temp, html_configAcceleration, acc);
  data += temp;
  sendHtml(data);

  //Axis1
  data += "<div style='width: 35em;'>";
  data += "Motor: <br />";
  if (GetLX200(":%RR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int reverse = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configRotAxis_1, 1);
  data += temp;
  data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
  sprintf_P(temp, html_configRotAxis_2, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%RD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); reverse = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configRotAxis_1, 2);
  data += temp;
  data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
  sprintf_P(temp, html_configRotAxis_2, 2);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%GR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int gear = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configGeAxis, gear, 1, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%GD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); gear = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configGeAxis, gear, 2, 2);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%SR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int step = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configStAxis, step, 1, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%SD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); step = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configStAxis, step, 2, 2);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%MR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int micro = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), 1, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%MD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); micro = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), 2, 2);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%BR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int backlashAxis = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configBlAxis, backlashAxis, 1, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%BD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); backlashAxis = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configBlAxis, backlashAxis, 2, 2);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%cR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int lowC = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configLCAxis, lowC * 10, 1, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%cD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); lowC = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configLCAxis, lowC * 10, 2, 2);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%CR#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int highC = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configHCAxis, highC * 10, 1, 1);
  data += temp;
  sendHtml(data);
  if (GetLX200(":%CD#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); highC = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configHCAxis, highC * 10, 2, 2);
  data += temp;
  data += "<br />";

  // Overhead and Horizon Limits
  if (GetLX200(":Gh#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int minAlt = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMinAlt, minAlt);
  data += temp;
  sendHtml(data);
  if (GetLX200(":Go#", temp1, sizeof(temp1)) == LX200GETVALUEFAILED) strcpy(temp1, "0"); int maxAlt = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMaxAlt, maxAlt);
  data += temp;
  sendHtml(data);
  // Meridian Limits
  if (GetLX200(":GXE9#", temp1, sizeof(temp1)) == LX200VALUEGET && GetLX200(":GXEA#", temp2, sizeof(temp2)) == LX200VALUEGET)
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
  strcpy(temp, "</div></div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void wifibluetooth::processConfigurationTelescopeGet()
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

  v = server.arg("MaxR");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4000)))
    {
      sprintf(temp, ":SX92:%04d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("Acc");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25)))
    {
      sprintf(temp, ":SXE2:%04d#", (int)(f * 10));
      SetLX200(temp);
    }
  }

  v = server.arg("GuideR");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.01) && (f <= 100)))
    {
      sprintf(temp, ":SX90:%03d#", (int)(f * 100));
      SetLX200(temp);
    }
  }

  v = server.arg("mrot1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      sprintf(temp, ":$RR%d#", i);
      SetLX200(temp);
    }
  }

  v = server.arg("mrot2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      sprintf(temp, ":$RD%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mge1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000)))
    {
      sprintf(temp, ":$GR%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mge2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000)))
    {
      sprintf(temp, ":$GD%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mst1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400)))
    {
      sprintf(temp, ":$SR%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mst2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400)))
    {
      sprintf(temp, ":$SD%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mmu1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 8) && (i <= 256)))
    {
      sprintf(temp, ":$MR%d#", (int)log2(i));
      SetLX200(temp);
    }
  }
  v = server.arg("mmu2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 8) && (i <= 256)))
    {
      sprintf(temp, ":$MD%d#", (int)log2(i));
      SetLX200(temp);
    }
  }
  v = server.arg("mbl1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      sprintf(temp, ":$BR%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mbl2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      sprintf(temp, ":$BD%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("mlc1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      sprintf(temp, ":$cR%d#", i / 10);
      SetLX200(temp);
    }
  }
  v = server.arg("mlc2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      sprintf(temp, ":$cD%d#", i / 10);
      SetLX200(temp);
    }
  }
  v = server.arg("mhc1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      sprintf(temp, ":$CR%d#", i / 10);
      SetLX200(temp);
    }
  }
  v = server.arg("mhc2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000)))
    {
      sprintf(temp, ":$CD%d#", i / 10);
      SetLX200(temp);
    }
  }
  // Overhead and Horizon Limits
  v = server.arg("ol");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 60) && (i <= 90)))
    {
      sprintf(temp, ":So%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("hl");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -30) && (i <= 30)))
    {
      sprintf(temp, ":Sh%d#", i);
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
      sprintf(temp, ":SXE9,%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("wl");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = round((i*60.0) / 15.0);
      sprintf(temp, ":SXEA,%d#", i);
      SetLX200(temp);
    }
  }

  // Backlash Limits
  v = server.arg("b1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      sprintf(temp, ":$BR%d#", i);
      SetLX200(temp);
    }
  }
  v = server.arg("b2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999)))
    {
      sprintf(temp, ":$BD%d#", i);
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

