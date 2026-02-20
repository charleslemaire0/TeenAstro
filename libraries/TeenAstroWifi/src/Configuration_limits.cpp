#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_limits

const char html_configMinAlt[] PROGMEM =
"<div class='bt'>Limits Altitude</div>"
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
"<div class='bt'>Limits German Equatorial Mount</div>"
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
"<div class='bt'>Tracking Safety Override (Far from Pole)</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='miDistanceFromPole' min='0' max='181'>"
"<button type='submit'>Upload</button>"
" (Minimum distance from Pole to keep tracking on for 6 hours after transit, 181 to disable)"
"</form>"
"<br />\r\n";
#endif
const char html_configMinAxis1[] PROGMEM =
"<div class='bt'>Limits of Instrument Axis 1</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='mia1' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Minimum value for instrument axis 1, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";
const char html_configMaxAxis1[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='maa1' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Maximum value for instrument axis 1, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";
const char html_configMinAxis2[] PROGMEM =
"<div class='bt'>Limits of Instrument Axis 2</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='mia2' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Minimum value for instrument axis 2, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";
const char html_configMaxAxis2[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='maa2' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
" (Maximum value for instrument axis 2, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";


void TeenAstroWifi::handleConfigurationLimits()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[350] = "";
  String data;

  processConfigurationLimitsGet();
  preparePage(data, ServerPage::Limits);
  sendHtml(data);
  ta_MountStatus.updateMount();
  data += "<div class='card'>";

  // Overhead and Horizon Limits
  int minAlt = 0;
  if (s_client->getMinAltitude(minAlt) != LX200_VALUEGET) minAlt = 0;
  sprintf_P(temp, html_configMinAlt, minAlt);
  data += temp;
  sendHtml(data);

  int maxAlt = 0;
  if (s_client->getMaxAltitude(maxAlt) != LX200_VALUEGET) maxAlt = 0;
  sprintf_P(temp, html_configMaxAlt, maxAlt);
  data += temp;
  sendHtml(data);

  // Meridian Limits (GEM only)
  if (ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM)
  {
    char underPole[20];
    if (s_client->getUnderPoleLimit(underPole, sizeof(underPole)) == LX200_VALUEGET)
    {
      float angle = (float)strtol(underPole, NULL, 10) / 10;
      sprintf_P(temp, html_configUnderPole, angle);
      data += temp;
    }
    char merE[20], merW[20];
    if (s_client->getLimitEast(merE, sizeof(merE)) == LX200_VALUEGET &&
        s_client->getLimitWest(merW, sizeof(merW)) == LX200_VALUEGET)
    {
      int degPastMerE = (int)round(strtol(merE, NULL, 10) * 15.0 / 60.0);
      sprintf_P(temp, html_configPastMerE, degPastMerE);
      data += temp;
      int degPastMerW = (int)round(strtol(merW, NULL, 10) * 15.0 / 60.0);
      sprintf_P(temp, html_configPastMerW, degPastMerW);
      data += temp;
    }
    #ifdef keepTrackingOnWhenFarFromPole
    int miDist = 181;
    if (s_client->getMinDistFromPole(miDist) != LX200_VALUEGET) miDist = 181;
    sprintf_P(temp, html_configMiDistanceFromPole, miDist);
    data += temp;
    #endif
    sendHtml(data);
  }

  // Axis limits
  bool ok = true;
  short anglemin, anglemax, angle_i_min, angle_i_max;

  ok =  s_client->getShort(":GXLA#", &anglemin)  == LX200_VALUEGET;
  ok &= s_client->getShort(":GXLB#", &anglemax)  == LX200_VALUEGET;
  ok &= s_client->getShort(":GXlA#", &angle_i_min) == LX200_VALUEGET;
  ok &= s_client->getShort(":GXlB#", &angle_i_max) == LX200_VALUEGET;

  if (ok)
  {
    sprintf_P(temp, html_configMinAxis1, (float)anglemin / 10.0, (float)angle_i_min, (float)anglemax / 10.0, (float)angle_i_min, (float)anglemax / 10.0);
    data += temp;
    sprintf_P(temp, html_configMaxAxis1, (float)anglemax / 10.0, (float)anglemin / 10.0, (float)angle_i_max, (float)anglemin / 10.0, (float)angle_i_max);
    data += temp;
    sendHtml(data);
  }

  ok =  s_client->getShort(":GXLC#", &anglemin)  == LX200_VALUEGET;
  ok &= s_client->getShort(":GXLD#", &anglemax)  == LX200_VALUEGET;
  ok &= s_client->getShort(":GXlC#", &angle_i_min) == LX200_VALUEGET;
  ok &= s_client->getShort(":GXlD#", &angle_i_max) == LX200_VALUEGET;

  if (ok)
  {
    sprintf_P(temp, html_configMinAxis2, (float)anglemin / 10.0, (float)angle_i_min, (float)anglemax / 10.0, (float)angle_i_min, (float)anglemax / 10.0);
    data += temp;
    sprintf_P(temp, html_configMaxAxis2, (float)anglemax / 10.0, (float)anglemin / 10.0, (float)angle_i_max, (float)anglemin / 10.0, (float)angle_i_max);
    data += temp;
    sendHtml(data);
  }
  else
    data += "<br />\r\n";

  data += "</div>"; // close card
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}

void TeenAstroWifi::processConfigurationLimitsGet()
{
  String v;
  int i;
  float f;

  // Overhead and Horizon Limits
  v = server.arg("ol");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 60) && (i <= 91)))
      s_client->setMaxAltitude(i);
  }
  v = server.arg("hl");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -30) && (i <= 30)))
      s_client->setMinAltitude(i);
  }

  // Meridian Limits
  v = server.arg("el");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = (int)round((i * 60.0) / 15.0);
      s_client->setLimitEast(i);
    }
  }
  v = server.arg("wl");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = (int)round((i * 60.0) / 15.0);
      s_client->setLimitWest(i);
    }
  }
  v = server.arg("up");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 9) && (f <= 12)))
      s_client->setUnderPoleLimit(f);
  }
  #ifdef keepTrackingOnWhenFarFromPole
  v = server.arg("miDistanceFromPole");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 181)))
      s_client->setMinDistFromPole(i);
  }
  #endif

  // Axis limits
  v = server.arg("mia1");
  if (v != "")
  {
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('A', f);
  }
  v = server.arg("maa1");
  if (v != "")
  {
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('B', f);
  }
  v = server.arg("mia2");
  if (v != "")
  {
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('C', f);
  }
  v = server.arg("maa2");
  if (v != "")
  {
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('D', f);
  }

  // Time zone (shared handler)
  int ut_hrs = -999;
  v = server.arg("u1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -13) && (i <= 13)))
      ut_hrs = i;
  }
  v = server.arg("u2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i == 00) || (i == 30) || (i == 45)))
    {
      if ((ut_hrs >= -13) && (ut_hrs <= 13))
        s_client->setTimeZone((float)(-(ut_hrs * 60 + i) / 60.0));
    }
  }
}
