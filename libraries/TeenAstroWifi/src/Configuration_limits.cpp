#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_limits

// Factory defaults from writeDefaultMount() / initCelestialPole() in MainUnit.
namespace {
const int kDefaultMinAltDeg = -10;
const int kDefaultMaxAltDeg = 91;
const float kDefaultUnderPoleHours = 12.0f;
const int kDefaultMeridianDeg = 15;       // EE invalid → 60 arcmin → 15°
const int kDefaultMinDistPoleDeg = 181;
}

const char html_configMinAlt[] PROGMEM =
"<div class='bt'>Limits Altitude</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='hl' min='-30' max='30'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='hl_d' value='1'>Default</button>"
" (Minimum Altitude, in degrees +/- 30)"
"</form>"
"\r\n";
const char html_configMaxAlt[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='ol' min='60' max='91'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='ol_d' value='1'>Default</button>"
" (Maximum Altitude, in degrees 60 to 90, set 91 to deactivate)"
"</form>"
"\r\n";
const char html_configUnderPole[] PROGMEM =
"<div class='bt'>Limits German Equatorial Mount</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='up' min='9' max='12' step='0.1'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='up_d' value='1'>Default</button>"
" (Under pole limite, in hours  from +/-9 to +/-12)"
"</form>"
"\r\n";
const char html_configPastMerE[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='el' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='el_d' value='1'>Default</button>"
" (Past Meridian when East of the pier, in degrees +/-45)"
"</form>"
"\r\n";
const char html_configPastMerW[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='wl' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='wl_d' value='1'>Default</button>"
" (Past Meridian when West of the pier, in degrees +/-45)"
"</form>"
"\r\n";
#ifdef keepTrackingOnWhenFarFromPole
const char html_configMiDistanceFromPole[] PROGMEM =
"<div class='bt'>Tracking Safety Override (Far from Pole)</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%d' type='number' name='miDistanceFromPole' min='0' max='181'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='miDistanceFromPole_d' value='1'>Default</button>"
" (Minimum distance from Pole to keep tracking on for 6 hours after transit, 181 to disable)"
"</form>"
"<br />\r\n";
#endif
const char html_configMinAxis1[] PROGMEM =
"<div class='bt'>Limits of Instrument Axis 1</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='mia1' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='mia1_d' value='1'>Default</button>"
" (Minimum value for instrument axis 1, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";
const char html_configMaxAxis1[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='maa1' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='maa1_d' value='1'>Default</button>"
" (Maximum value for instrument axis 1, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";
const char html_configMinAxis2[] PROGMEM =
"<div class='bt'>Limits of Instrument Axis 2</div>"
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='mia2' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='mia2_d' value='1'>Default</button>"
" (Minimum value for instrument axis 2, in degrees from %.1f to %.1f)"
"</form>"
"\r\n";
const char html_configMaxAxis2[] PROGMEM =
"<form method='get' action='/configuration_limits.htm'>"
" <input value='%.1f' type='number' name='maa2' min='%.1f' max='%.1f' step='0.1'>"
"<button type='submit'>Upload</button>"
"<button type='submit' name='maa2_d' value='1'>Default</button>"
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
  char temp[480] = "";
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

  // Per-limit Default buttons (factory values). Checked before Upload so a
  // Default click is not overridden by the still-present number field.
  if (server.arg("hl_d") != "")
  {
    any = true;
    s_client->setMinAltitude(kDefaultMinAltDeg);
  }
  else
  {
    v = server.arg("hl");
    if (v != "")
    {
      any = true;
      if ((atoi2((char*)v.c_str(), &i)) && ((i >= -30) && (i <= 30)))
        s_client->setMinAltitude(i);
    }
  }

  if (server.arg("ol_d") != "")
  {
    any = true;
    s_client->setMaxAltitude(kDefaultMaxAltDeg);
  }
  else
  {
    v = server.arg("ol");
    if (v != "")
    {
      any = true;
      if ((atoi2((char*)v.c_str(), &i)) && ((i >= 60) && (i <= 91)))
        s_client->setMaxAltitude(i);
    }
  }

  if (server.arg("el_d") != "")
  {
    any = true;
    s_client->setLimitEast((int)round((kDefaultMeridianDeg * 60.0) / 15.0));
  }
  else
  {
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
  }

  if (server.arg("wl_d") != "")
  {
    any = true;
    s_client->setLimitWest((int)round((kDefaultMeridianDeg * 60.0) / 15.0));
  }
  else
  {
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
  }

  if (server.arg("up_d") != "")
  {
    any = true;
    s_client->setUnderPoleLimit(kDefaultUnderPoleHours);
  }
  else
  {
    v = server.arg("up");
    if (v != "")
    {
      any = true;
      if ((atof2((char*)v.c_str(), &f)) && ((f >= 9) && (f <= 12)))
        s_client->setUnderPoleLimit(f);
    }
  }

  #ifdef keepTrackingOnWhenFarFromPole
  if (server.arg("miDistanceFromPole_d") != "")
  {
    any = true;
    s_client->setMinDistFromPole(kDefaultMinDistPoleDeg);
  }
  else
  {
    v = server.arg("miDistanceFromPole");
    if (v != "")
    {
      any = true;
      if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 181)))
        s_client->setMinDistFromPole(i);
    }
  }
  #endif

  // Axis limits — Default restores the mount-type mechanical bound (:GXlA#–D#).
  const char* axisDefs[4] = { "mia1_d", "maa1_d", "mia2_d", "maa2_d" };
  const char* axisVals[4] = { "mia1", "maa1", "mia2", "maa2" };
  const char axisModes[4] = { 'A', 'B', 'C', 'D' };
  for (int a = 0; a < 4; a++)
  {
    if (server.arg(axisDefs[a]) != "")
    {
      any = true;
      int bound = 0;
      if (s_client->getMountTypeAxisLimit(axisModes[a], bound) == LX200_VALUEGET)
        s_client->setAxisLimit(axisModes[a], (float)bound);
    }
    else
    {
      v = server.arg(axisVals[a]);
      if (v != "")
      {
        any = true;
        if (atof2((char*)v.c_str(), &f))
          s_client->setAxisLimit(axisModes[a], f);
      }
    }
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
