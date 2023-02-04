#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
// -----------------------------------------------------------------------------------
// configuration_telescope

const char html_configEncoders_1[] PROGMEM =
"<div class='bt'> Encoders Sync Mode: <br/> </div>"
"<form action='/configuration_mount.htm'>"
"<select name='mount' onchange='this.form.submit()' >";
const char html_configEncoders_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configPPDAxis1[] PROGMEM =
"<div class='bt'> Encoders of Instrument Axis 1: <br/> </div>"
"<form method='get' action='/configuration_encoders.htm'>"
" <input value='%.01f' type='number' name='ppda1' min='0' max='3600' step='0.01'>"
"<button type='submit'>Upload</button>"
" (Pulse per degree axis 1 from 0 to 3600)"
"</form>"
"\r\n";

const char html_configPPDAxis2[] PROGMEM =
"<div class='bt'> Encoders of Instrument Axis 2: <br/> </div>"
"<form method='get' action='/configuration_encoders.htm'>"
" <input value='%.01f' type='number' name='ppda2' min='0' max='3600' step='0.01'>"
"<button type='submit'>Upload</button>"
" (Pulse per degree axis 2 from 0 to 3600)"
"</form>"
"\r\n";

const char html_configRotEAxis_1[] PROGMEM =
"<form action='/configuration_mount.htm'>"
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


//const char html_reboot_t[] PROGMEM =
//"<br/><form method='get' action='/configuration_mount.htm'>"
//"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
//"<button type='submit'>Continue</button>"
//"</form><br/><br/><br/><br/>"
//"\r\n";
//bool restartRequired_t = false;

void TeenAstroWifi::handleConfigurationEncoders()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;

  processConfigurationLimitsGet();
  preparePage(data, ServerPage::Limits);
  sendHtml(data);
  //if (restartRequired_t)
  //{
  //  data += FPSTR(html_reboot_t);
  //  data += "</div></div></body></html>";
  //  sendHtml(data);
  //  sendHtmlDone(data);
  //  restartRequired_t = false;
  //  delay(1000);
  //  return;
  //}
  //update
  ta_MountStatus.updateMount();



  // Overhead and Horizon Encoders
  //if (GetLX200(":GXLH#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); int minAlt = (int)strtol(&temp1[0], NULL, 10);
  //sprintf_P(temp, html_configMinAlt, minAlt);
  //data += temp;
  //sendHtml(data);
  //if (GetLX200(":GXLO#", temp1, sizeof(temp1)) == LX200_GETVALUEFAILED) strcpy(temp1, "0"); int maxAlt = (int)strtol(&temp1[0], NULL, 10);
  //sprintf_P(temp, html_configMaxAlt, maxAlt);
  //data += temp;
  //sendHtml(data);
  //// Meridian Encoders
  //if (ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM)
  //{
  //  if (GetLX200(":GXLU#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  //  {
  //    float angle = (float)strtol(&temp1[0], NULL, 10) / 10;
  //    sprintf_P(temp, html_configUnderPole, angle);
  //    data += temp;
  //  }
  //  if (GetLX200(":GXLE#", temp1, sizeof(temp1)) == LX200_VALUEGET && GetLX200(":GXLW#", temp2, sizeof(temp2)) == LX200_VALUEGET)
  //  {
  //    int degPastMerE = (int)strtol(&temp1[0], NULL, 10);
  //    degPastMerE = round((degPastMerE * 15.0) / 60.0);
  //    sprintf_P(temp, html_configPastMerE, degPastMerE);
  //    data += temp;
  //    int degPastMerW = (int)strtol(&temp2[0], NULL, 10);
  //    degPastMerW = round((degPastMerW * 15.0) / 60.0);
  //    sprintf_P(temp, html_configPastMerW, degPastMerW);
  //    data += temp;
  //  }

  //}

  //if (GetLX200(":GXLA#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  //{
  //  float angle = -(float)strtol(&temp1[0], NULL, 10) / 10;
  //  sprintf_P(temp, html_configMinAxis1, angle);
  //  data += temp;
  //}
  //if (GetLX200(":GXLB#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  //{
  //  float angle = (float)strtol(&temp1[0], NULL, 10) / 10;
  //  sprintf_P(temp, html_configMaxAxis1, angle);
  //  data += temp;
  //}
  //if (GetLX200(":GXLC#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  //{
  //  float angle = -(float)strtol(&temp1[0], NULL, 10) / 10;
  //  sprintf_P(temp, html_configMinAxis2, angle);
  //  data += temp;
  //}
  //if (GetLX200(":GXLD#", temp1, sizeof(temp1)) == LX200_VALUEGET)
  //{
  //  float angle = (float)strtol(&temp1[0], NULL, 10) / 10;
  //  sprintf_P(temp, html_configMaxAxis2, angle);
  //  data += temp;
  //}

  //else data += "<br />\r\n";
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
  char temp[20] = "";

 
  // Overhead and Horizon Encoders
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

  // Meridian Encoders
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

