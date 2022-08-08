#include "TeenAstroWifi.h"

const char* html_TrackingState PROGMEM =        "<b> Tracking State: </b><br/>\n";
const char* html_Tracking PROGMEM =             "&nbsp;&nbsp;Tracking: <font class=\"c\">%s</font><br/>\n";
const char* html_TrackingCorrections PROGMEM =  "&nbsp;&nbsp;Tracking Corrections: <font class=\"c\">%s</font><br/>\n";
const char* html_TrackingSpeed PROGMEM =        "&nbsp;&nbsp;Tracking Speed: <font class=\"c\">%s</font><br/>\n";
const char* html_TrackingRate PROGMEM =         "&nbsp;&nbsp;Tracking Rate: <font class=\"c\">%f</font>Hz<br />\n";
const char* html_TrackingSpeedRA PROGMEM =     "&nbsp;&nbsp;drift Ra: <font class=\"c\"> %.5f sec/SI</font><br/>\n";
const char* html_TrackingSpeedDEC PROGMEM =    "&nbsp;&nbsp;drift Dec: <font class=\"c\"> %.5f arcsec/SI</font><br/><br/>\n";

void TeenAstroWifi::addTrackingInfo(String &data )
{
  char temp[300] = "";
  char temp1[80] = "";
  ta_MountStatus.updateMount();
  data += FPSTR(html_TrackingState);

  switch (ta_MountStatus.getTrackingState())
  {
  case TeenAstroMountStatus::TRK_SLEWING:
    strcpy(temp1, "Slewing");
    break;
  case TeenAstroMountStatus::TRK_ON:
    strcpy(temp1, "On");
    break;
  case  TeenAstroMountStatus::TRK_OFF:
    strcpy(temp1, "Off");
    break;
  default:
    strcpy(temp1, "?");
  }
  sprintf_P(temp, html_Tracking, temp1);
  data += temp;

  switch (ta_MountStatus.getRateCompensation())
  {
  case TeenAstroMountStatus::RateCompensation::RC_ALIGN_RA:
    strcpy(temp1, "Align Comp RA Axis");
    break;
  case TeenAstroMountStatus::RateCompensation::RC_ALIGN_BOTH:
    strcpy(temp1, "Align Comp Both Axis");
    break;
  case TeenAstroMountStatus::RateCompensation::RC_FULL_RA:
    strcpy(temp1, "Full Comp RA Axis");
    break;
  case TeenAstroMountStatus::RateCompensation::RC_FULL_BOTH:
    strcpy(temp1, "Full Comp Both Axis");
    break;
  default:
    strcpy(temp1, "No Comp Axis");
    break;
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
  case TeenAstroMountStatus::SID_STAR:
    strcpy(temp1, "Sidereal");
    break;
  case TeenAstroMountStatus::SID_SUN:
    strcpy(temp1, "Solar");
    break;
  case TeenAstroMountStatus::SID_MOON:
    strcpy(temp1, "Lunar");
    break;
  case TeenAstroMountStatus::SID_TARGET:
  {
    strcpy(temp1, "Target");
    showRates = true;
    break;
  }
  default:
    strcpy(temp1, "Unkown");
    break;
  }
  sprintf_P(temp, html_TrackingSpeed, temp1);
  data += temp;

  if (showRates)
  {
    ta_MountStatus.updateTrackingRate();
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