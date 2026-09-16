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
  if (processConfigurationLimitsGet())
  {
    ta_MountStatus.invalidateAllConfig();
    sendRedirectAfterMutation("/configuration_limits.htm");
    return;
  }
  sendHtmlStart();
  char temp[448] = "";
  String data;

  preparePage(data, ServerPage::Limits);
  sendHtml(data);

  // Mount type (GEM meridian UI) from :GXAS#; limit values from :GXCS#.
  ta_MountStatus.updateMount();
  ta_MountStatus.updateAllConfig();
  data += "<div class='card'>";

  if (!ta_MountStatus.hasConfig())
  {
    data += "<p>Mount config unavailable</p></div>";
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    return;
  }

  // Overhead and Horizon Limits
  snprintf_P(temp, sizeof(temp), html_configMinAlt, (int)ta_MountStatus.getCfgMinAlt());
  data += temp;
  sendHtml(data);

  snprintf_P(temp, sizeof(temp), html_configMaxAlt, (int)ta_MountStatus.getCfgMaxAlt());
  data += temp;
  sendHtml(data);

  // Meridian Limits (GEM only)
  if (ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM)
  {
    const float underPole = ta_MountStatus.getCfgUnderPole10() / 10.0f;
    snprintf_P(temp, sizeof(temp), html_configUnderPole, underPole);
    data += temp;

    // :GXCS# meridian values match :GXLE#/:GXLW# (arcmin×4) → degrees via /4
    const int degPastMerE = (int)round(ta_MountStatus.getCfgMeridianE() / 4.0);
    const int degPastMerW = (int)round(ta_MountStatus.getCfgMeridianW() / 4.0);
    snprintf_P(temp, sizeof(temp), html_configPastMerE, degPastMerE);
    data += temp;
    snprintf_P(temp, sizeof(temp), html_configPastMerW, degPastMerW);
    data += temp;
    #ifdef keepTrackingOnWhenFarFromPole
    snprintf_P(temp, sizeof(temp), html_configMiDistanceFromPole, (int)ta_MountStatus.getCfgMinDistPole());
    data += temp;
    #endif
    sendHtml(data);
  }

  // Axis limits: user values from :GXCS#; mount-type bounds still need :GXlA#–D#
  bool ok = true;
  int angle_i_min = 0, angle_i_max = 0;

  ok =  s_client->getMountTypeAxisLimit('A', angle_i_min) == LX200_VALUEGET;
  ok &= s_client->getMountTypeAxisLimit('B', angle_i_max) == LX200_VALUEGET;

  if (ok)
  {
    const float anglemin = ta_MountStatus.getCfgAxis1Min() / 10.0f;
    const float anglemax = ta_MountStatus.getCfgAxis1Max() / 10.0f;
    snprintf_P(temp, sizeof(temp), html_configMinAxis1, anglemin, (float)angle_i_min, anglemax, (float)angle_i_min, anglemax);
    data += temp;
    snprintf_P(temp, sizeof(temp), html_configMaxAxis1, anglemax, anglemin, (float)angle_i_max, anglemin, (float)angle_i_max);
    data += temp;
    sendHtml(data);
  }

  ok =  s_client->getMountTypeAxisLimit('C', angle_i_min) == LX200_VALUEGET;
  ok &= s_client->getMountTypeAxisLimit('D', angle_i_max) == LX200_VALUEGET;

  if (ok)
  {
    const float anglemin = ta_MountStatus.getCfgAxis2Min() / 10.0f;
    const float anglemax = ta_MountStatus.getCfgAxis2Max() / 10.0f;
    snprintf_P(temp, sizeof(temp), html_configMinAxis2, anglemin, (float)angle_i_min, anglemax, (float)angle_i_min, anglemax);
    data += temp;
    snprintf_P(temp, sizeof(temp), html_configMaxAxis2, anglemax, anglemin, (float)angle_i_max, anglemin, (float)angle_i_max);
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

bool TeenAstroWifi::processConfigurationLimitsGet()
{
  bool any = false;
  String v;
  int i;
  float f;

  // Overhead and Horizon Limits
  v = server.arg("ol");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 60) && (i <= 91)))
      s_client->setMaxAltitude(i);
  }
  v = server.arg("hl");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -30) && (i <= 30)))
      s_client->setMinAltitude(i);
  }

  // Meridian Limits
  v = server.arg("el");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = (int)round((i * 60.0) / 15.0);
      s_client->setLimitEast(i);
    }
  }
  v = server.arg("wl");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -45) && (i <= 45)))
    {
      i = (int)round((i * 60.0) / 15.0);
      s_client->setLimitWest(i);
    }
  }
  v = server.arg("up");
  if (v != "")
  {
    any = true;
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 9) && (f <= 12)))
      s_client->setUnderPoleLimit(f);
  }
  #ifdef keepTrackingOnWhenFarFromPole
  v = server.arg("miDistanceFromPole");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 181)))
      s_client->setMinDistFromPole(i);
  }
  #endif

  // Axis limits
  v = server.arg("mia1");
  if (v != "")
  {
    any = true;
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('A', f);
  }
  v = server.arg("maa1");
  if (v != "")
  {
    any = true;
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('B', f);
  }
  v = server.arg("mia2");
  if (v != "")
  {
    any = true;
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('C', f);
  }
  v = server.arg("maa2");
  if (v != "")
  {
    any = true;
    if (atof2((char*)v.c_str(), &f))
      s_client->setAxisLimit('D', f);
  }

  // Time zone (shared handler)
  int ut_hrs = -999;
  v = server.arg("u1");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -13) && (i <= 13)))
      ut_hrs = i;
  }
  v = server.arg("u2");
  if (v != "")
  {
    any = true;
    if ((atoi2((char*)v.c_str(), &i)) && ((i == 00) || (i == 30) || (i == 45)))
    {
      if ((ut_hrs >= -13) && (ut_hrs <= 13))
        s_client->setTimeZone((float)(-(ut_hrs * 60 + i) / 60.0));
    }
  }

  return any;
}
