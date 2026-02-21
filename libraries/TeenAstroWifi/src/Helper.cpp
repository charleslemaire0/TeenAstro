#include "TeenAstroWifi.h"

const char* html_TrackingState PROGMEM =        "<b>Tracking State:</b><br/>\n";
const char* html_Tracking PROGMEM =             "Tracking: <span class='c'>%s</span><br/>\n";
const char* html_TrackingCorrections PROGMEM =  "Corrections: <span class='c'>%s</span><br/>\n";
const char* html_TrackingSpeed PROGMEM =        "Speed: <span class='c'>%s</span><br/>\n";
const char* html_TrackingRate PROGMEM =         "Rate: <span class='c'>%f</span> Hz<br/>\n";
const char* html_TrackingSpeedRA PROGMEM =     "Drift RA: <span class='c'>%.5f sec/SI</span><br/>\n";
const char* html_TrackingSpeedDEC PROGMEM =    "Drift Dec: <span class='c'>%.5f arcsec/SI</span><br/><br/>\n";

void TeenAstroWifi::addTrackingInfo(String &data )
{
  char temp[128] = "";
  char temp1[24] = "";
  data += FPSTR(html_TrackingState);

  switch (ta_MountStatus.getTrackingState())
  {
  case TeenAstroMountStatus::TRK_SLEWING: strcpy(temp1, "Slewing"); break;
  case TeenAstroMountStatus::TRK_ON:      strcpy(temp1, "On"); break;
  case TeenAstroMountStatus::TRK_OFF:     strcpy(temp1, "Off"); break;
  default: strcpy(temp1, "?");
  }
  sprintf_P(temp, html_Tracking, temp1);
  data += temp;

  switch (ta_MountStatus.getRateCompensation())
  {
  case TeenAstroMountStatus::RateCompensation::RC_RA:   strcpy(temp1, "Comp RA Axis"); break;
  case TeenAstroMountStatus::RateCompensation::RC_BOTH: strcpy(temp1, "Comp Both Axis"); break;
  default: strcpy(temp1, "get Comp Axis failed"); break;
  }
  sprintf_P(temp, html_TrackingCorrections, temp1);
  data += temp;

  double rate;
  ta_MountStatus.getTrackingRate(rate);
  sprintf_P(temp, html_TrackingRate, rate);
  data += temp;

  bool showRates = false;
  switch (ta_MountStatus.getSiderealMode())
  {
  case TeenAstroMountStatus::SID_STAR:   strcpy(temp1, "Sidereal"); break;
  case TeenAstroMountStatus::SID_SUN:    strcpy(temp1, "Solar"); break;
  case TeenAstroMountStatus::SID_MOON:   strcpy(temp1, "Lunar"); break;
  case TeenAstroMountStatus::SID_TARGET: strcpy(temp1, "Target"); showRates = true; break;
  default: strcpy(temp1, "Unknown"); break;
  }
  sprintf_P(temp, html_TrackingSpeed, temp1);
  data += temp;

  if (showRates)
  {
    // Use only cached rates (e.g. from :GXAS#) — no extra serial round-trip on index/status.
    if (ta_MountStatus.hasInfoTrackingRate())
    {
      float RateRa = ta_MountStatus.getTrackingRateRa() / 10000.0;
      sprintf_P(temp, html_TrackingSpeedRA, RateRa);
      data += temp;
      float RateDec = ta_MountStatus.getTrackingRateDec() / 10000.0;
      sprintf_P(temp, html_TrackingSpeedDEC, RateDec);
      data += temp;
    }
  }
}
