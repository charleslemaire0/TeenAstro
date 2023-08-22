#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_telescope

const char html_configMinAlt[] PROGMEM =
"<div class='bt'> Limits Altitude: <br/> </div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='hl' min='-30' max='30'>"
"<button type='submit'>Upload</button>"
" (Minimum Altitude, in degrees +/- 30)"
"</form>"
"\r\n";
const char html_configMaxAlt[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='ol' min='60' max='91'>"
"<button type='submit'>Upload</button>"
" (Maximum Altitude, in degrees 60 to 90, set 91 to deactivate)"
"</form>"
"\r\n";
const char html_configUnderPole[] PROGMEM =
"<div class='bt'> Limits German Equatorial Mount: <br/> </div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='up' min='9' max='12' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Under pole limite, in hours  from +/-9 to +/-12)"
"</form>"
"\r\n";
const char html_configPastMerE[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='el' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
" (Past Meridian when East of the pier, in degrees +/-45)"
"</form>"
"\r\n";
const char html_configPastMerW[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='wl' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
" (Past Meridian when West of the pier, in degrees +/-45)"
"</form>"
"\r\n";
#ifdef keepTrackingOnWhenFarFromPole
const char html_configMiDistanceFromPole[] PROGMEM =
"<div class='bt'> Tracking safety override when far from Pole: <br/> </div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='miDistanceFromPole' min='0' max='181'>"
"<button type='submit'>Upload</button>"
" (Minimum distance from Pole to keep tracking on for 6 hours after transit, 181 to disable)"
"</form>"
"<br />\r\n";
#endif
const char html_configMinAxis1[] PROGMEM =
"<div class='bt'> Limits of Instrument Axis 1: <br/> </div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='mia1' min='-180' max='0' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Minimum value for instrument axis 1, in degrees from -180 to 0)"
"</form>"
"\r\n";
const char html_configMaxAxis1[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='maa1' min='0' max='180' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Maximum value for instrument axis 1, in degrees from 0 to 180)"
"</form>"
"\r\n";
const char html_configMinAxis2[] PROGMEM =
"<div class='bt'> Limits of Instrument Axis 2: <br/> </div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='mia2' min='-360' max='0' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Minimum value for instrument axis 2, in degrees from -360 to 0)"
"</form>"
"\r\n";
const char html_configMaxAxis2[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='maa2' min='0' max='360' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Maximum value for instrument axis 2, in degrees from 0 to 360)"
"</form>"
"\r\n";



//const char html_reboot_t[] PROGMEM =
//"<br/><form method='get' action='/configuration_mount.htm'>"
//"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
//"<button type='submit'>Continue</button>"
//"</form><br/><br/><br/><br/>"
//"\r\n";
//bool restartRequired_t = false;

void TeenAstroWifi::handleConfigurationLimits()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[350] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;

  processConfigurationLimitsGet();
  preparePage(data, ServerPage::Limits);
  sendHtml(data);
  ta_MountStatus.updateMount();

  // Overhead and Horizon Limits
  if (GetLX200(":GXLH#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); int minAlt = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMinAlt, minAlt);
  data += temp;
  sendHtml(data);
  if (GetLX200(":GXLO#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); int maxAlt = (int)strtol(&temp1[0], NULL, 10);
  sprintf_P(temp, html_configMaxAlt, maxAlt);
  data += temp;
  sendHtml(data);
  // Meridian Limits
  if (ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM)
  {
    if (GetLX200(":GXLU#", temp1, sizeof(temp1)) == LX200_VALUEGET)
    {
      float angle = (float)strtol(&temp1[0], NULL, 10) / 10;
      sprintf_P(temp, html_configUnderPole, angle);
      data += temp;
    }
    if (GetLX200(":GXLE#", temp1, sizeof(temp1)) == LX200_VALUEGET && GetLX200(":GXLW#", temp2, sizeof(temp2)) == LX200_VALUEGET)
    {
      int degPastMerE = (int)strtol(&temp1[0], NULL, 10);
      degPastMerE = round((degPastMerE * 15.0) / 60.0);
      sprintf_P(temp, html_configPastMerE, degPastMerE);
      data += temp;
      int degPastMerW = (int)strtol(&temp2[0], NULL, 10);
      degPastMerW = round((degPastMerW * 15.0) / 60.0);
      sprintf_P(temp, html_configPastMerW, degPastMerW);
      data += temp;
    }
    #ifdef keepTrackingOnWhenFarFromPole
    if (GetLX200(":GXLS#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "181"); int miDistanceFromPole = (int)strtol(&temp1[0], NULL, 10);
    sprintf_P(temp, html_configMiDistanceFromPole, miDistanceFromPole);
    data += temp;
    #endif
  }

  if (GetLX200(":GXLA#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  {
    float angle = -(float)strtol(&temp1[0], NULL, 10) / 10;
    sprintf_P(temp, html_configMinAxis1, angle);
    data += temp;
  }
  if (GetLX200(":GXLB#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  {
    float angle = (float)strtol(&temp1[0], NULL, 10) / 10;
    sprintf_P(temp, html_configMaxAxis1, angle);
    data += temp;
  }
  if (GetLX200(":GXLC#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  {
    float angle = -(float)strtol(&temp1[0], NULL, 10) / 10;
    sprintf_P(temp, html_configMinAxis2, angle);
    data += temp;
  }
  if (GetLX200(":GXLD#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  {
    float angle = (float)strtol(&temp1[0], NULL, 10) / 10;
    sprintf_P(temp, html_configMaxAxis2, angle);
    data += temp;
  }

  else data += "<br />\r\n";
  strcpy(temp, "</div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationLimitsGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";

 
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
  v = server.arg("up");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 9) && (f <= 12)))
    {
      sprintf(temp, ":SXLU,%03d#", (int)(f * 10));
      SetLX200(temp);
    }
  }
  #ifdef keepTrackingOnWhenFarFromPole
  v = server.arg("miDistanceFromPole");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 181)))
    {
      sprintf(temp, ":SXLS,%d#", i);
      SetLX200(temp);
    }
  }
  #endif
  v = server.arg("mia1");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((-f >= 0) && (-f <= 360)))
    {
      sprintf(temp, ":SXLA,%04d#", (int)(-f * 10));
      SetLX200(temp);
    }
  }
  v = server.arg("maa1");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 360)))
    {
      sprintf(temp, ":SXLB,%04d#", (int)(f * 10));
      SetLX200(temp);
    }
  }
  v = server.arg("mia2");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((-f >= 0) && (-f <= 360)))
    {
      sprintf(temp, ":SXLC,%04d#", (int)(-f * 10));
      SetLX200(temp);
    }
  }
  v = server.arg("maa2");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0) && (f <= 360)))
    {
      sprintf(temp, ":SXLD,%04d#", (int)(f * 10));
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

