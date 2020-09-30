#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"

// -----------------------------------------------------------------------------------
// The home page, status information


#define Axis1 "&alpha;"
#define Axis1A "&alpha;"
#define Axis2 "&delta;"

const char* html_settingsBrowserTime PROGMEM =
"&nbsp;&nbsp;<span id=\"datetime\"></span> UT (web browser)"
"<script> "
"function pad(num, size) { var s = '000000000' + num; return s.substr(s.length-size); }"
"var now = new Date(); document.getElementById('datetime').innerHTML = (now.getUTCMonth()+1).toString()+'/'+"
"pad(now.getUTCDate().toString(),2)+'/'+pad(now.getUTCFullYear().toString(),4)+"
"' '+pad(now.getUTCHours().toString(),2)+':'+pad(now.getUTCMinutes().toString(),2)+':'+pad(now.getUTCSeconds().toString(),2); "
"</script><br />\r\n";

const char* html_indexDate PROGMEM = "&nbsp;&nbsp;<font class='c'>%s</font>";
const char* html_indexTime PROGMEM = "&nbsp;<font class='c'>%s</font>&nbsp;UT";
const char* html_indexSidereal PROGMEM = "&nbsp;(<font class='c'>%s</font>&nbsp; LST)<br />";

const char* html_indexPosition PROGMEM = "&nbsp;&nbsp;" Axis1 "=<font class='c'>%s</font>, " Axis2 "=<font class='c'>%s</font><br />";
const char* html_indexTarget PROGMEM = "&nbsp;&nbsp;Target:&nbsp;&nbsp; " Axis1 "=<font class='c'>%s</font>, " Axis2 "=<font class='c'>%s</font><br />";

const char* html_indexPier PROGMEM = "&nbsp;&nbsp;<font class='c'>%s</font> Pier Side<br />";

const char* html_indexPark PROGMEM = "&nbsp;&nbsp;Parking: <font class='c'>%s</font><br />";
const char* html_indexHasFocuser PROGMEM = "&nbsp;&nbsp;Has Focuser: <font class='c'>%s</font><br />";
const char* html_indexGNSS PROGMEM = "&nbsp;&nbsp;GNSS: <font class='c'>%s</font><br />";
const char* html_indexPowerMode PROGMEM = "&nbsp;&nbsp;Power mode: <font class='c'>%s</font><br />";
const char* html_indexAligned PROGMEM = "&nbsp;&nbsp;Aligned: <font class='c'>%s</font><br />";
const char* html_indexTracking PROGMEM = "&nbsp;&nbsp;Tracking: <font class='c'>%s %s</font><br />";
const char* html_indexGuiding PROGMEM = "&nbsp;&nbsp;Guiding: <font class='c'>%s</font><br />";
const char* html_indexSpiral PROGMEM = "&nbsp;&nbsp;Spiral running: <font class='c'>%s</font><br />";

const char* html_indexLastError PROGMEM = "&nbsp;&nbsp;Last Error: <font class='c'>%s</font><br />";
const char* html_indexWorkload PROGMEM = "&nbsp;&nbsp;Workload: <font class='c'>%s</font><br />";


void TeenAstroWifi::handleRoot()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[300] = "";
  char temp1[80] = "";
  char temp2[80] = "";
  //updates
  ta_MountStatus.updateTime();
  ta_MountStatus.updateRaDec();
  ta_MountStatus.updateMount();
  ta_MountStatus.updateRaDecT();
  //ta_MountStatus.updateTrackingRate();

  String data;
  preparePage(data, 1);
  sendHtml(data);
  data += "<div style='width: 27em;'>";

  data += "<b>Time and Date:</b><br />";
  // Browser time
  data += FPSTR(html_settingsBrowserTime);
  sendHtml(data);
  // UTC Date
  sprintf_P(temp, html_indexDate, ta_MountStatus.getUTCdate());
  data += temp;
  sendHtml(data);
  // UTC Time
  sprintf_P(temp, html_indexTime, ta_MountStatus.getUTC());
  data += temp;
  sendHtml(data);
  // LST
  sprintf_P(temp, html_indexSidereal, ta_MountStatus.getSidereal());
  data += temp;
  sendHtml(data);

  data += "<br /><b>Current Jnow Coordinates:</b><br />";

  // RA,Dec current

  sprintf_P(temp, html_indexPosition, ta_MountStatus.getRa(), ta_MountStatus.getDec());
  data += temp;
  sendHtml(data);

  // pier side and meridian flips
  switch (ta_MountStatus.getPierState())
  {
  case TeenAstroMountStatus::PIER_W:
    strcpy(temp1, "West");
    break;
  case TeenAstroMountStatus::PIER_E:
    strcpy(temp1, "East");
    break;
  case TeenAstroMountStatus::PIER_UNKNOW:
    strcpy(temp1, "None");
    break;
  default:
    strcpy(temp1, "Unknown");
    break;
  }

  sprintf_P(temp, html_indexPier, temp1);
  data += temp;
  sendHtml(data);
  // RA,Dec target

  data += "<br /><b>Last Jnow Target Coordinates:</b><br />";
  sprintf_P(temp, html_indexPosition, ta_MountStatus.getRaT(), ta_MountStatus.getDecT());
  data += temp;
  sendHtml(data);

  data += "<br /><b>Alignment:</b><br />";
  sendHtml(data);
  //Aligned
  ta_MountStatus.isAligned() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexAligned, temp1);
  data += temp;
  sendHtml(data);


  data += "<br /><b>Operations:</b><br />";

  // Park

  switch (ta_MountStatus.getParkState())
  {
  case TeenAstroMountStatus::PRK_UNPARKED:
    strcpy(temp1, "Not Parked");
    break;
  case TeenAstroMountStatus::PRK_PARKED:
    strcpy(temp1, "Parked");
    break;
  case TeenAstroMountStatus::PRK_FAILED:
    strcpy(temp1, "Park Failed");
    break;
  case TeenAstroMountStatus::PRK_PARKING:
    strcpy(temp1, "Parking");
    break;
  case TeenAstroMountStatus::PRK_UNKNOW:
  default:
    strcpy(temp1, "?");
    break;
  }
  // if (ta_MountStatus.atHome()) strcat(temp1," </font>(<font class=\"c\">At Home</font>)<font class=\"c\">");
  sprintf_P(temp, html_indexPark, temp1);
  data += temp;
  sendHtml(data);
  // Tracking
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

  strcpy(temp2, "</font>(<font class=\"c\">");
  //if (mountStatus.rateCompensation()== MountStatus::RC_REFR_RA)   strcat(temp2,"Refr Comp RA Axis, ");
  //if (mountStatus.rateCompensation()== MountStatus::RC_REFR_BOTH) strcat(temp2,"Refr Comp Both Axis, ");
  //if (mountStatus.rateCompensation()== MountStatus::RC_FULL_RA)   strcat(temp2,"Full Comp RA Axis, ");
  //if (mountStatus.rateCompensation()== MountStatus::RC_FULL_BOTH) strcat(temp2,"Full Comp Both Axis, ");
  if (temp2[strlen(temp2) - 2] == ',')
  {
    temp2[strlen(temp2) - 2] = 0; strcat(temp2, "</font>)<font class=\"c\">");
  }
  else strcpy(temp2, "");
  sprintf_P(temp, html_indexTracking, temp1, temp2);
  data += temp;
  sendHtml(data);
  // Tracking rate
  //sprintf(temp, "&nbsp;&nbsp;Tracking Rate: <font class=\"c\">%s</font>Hz<br />", ta_MountStatus.getTrackingRate());
  //data += temp;
  switch (ta_MountStatus.getSiderealMode())
  {
  case TeenAstroMountStatus::SID_STAR:
    strcpy(temp2, "Sidereal");
    break;
  case TeenAstroMountStatus::SID_SUN:
    strcpy(temp2, "Solar");
    break;
  case TeenAstroMountStatus::SID_MOON:
    strcpy(temp2, "Lunar");
    break;
  default:
    strcpy(temp2, "Unkown");
    break;
  }
  sprintf(temp, "&nbsp;&nbsp;Tracking Speed: <font class=\"c\">%s</font><br />", temp2);
  data += temp;
  sendHtml(data);

  ta_MountStatus.isSpiralRunning() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexSpiral, temp1);
  data += temp;
  sendHtml(data);

  //Focuser
  ta_MountStatus.hasFocuser() ? strcpy(temp1, "Yes") :  strcpy(temp1, "No");
  sprintf_P(temp, html_indexHasFocuser, temp1);
  data += temp;
  sendHtml(data);

  //GNSS
  ta_MountStatus.isGNSSValid() ? strcpy(temp1, "Tracking Satellite") : strcpy(temp1, "No Signal");
  sprintf_P(temp, html_indexGNSS, temp1);
  data += temp;
  sendHtml(data);

  //Power
  ta_MountStatus.isLowPower() ? strcpy(temp1, "Low") : strcpy(temp1, "High");
  sprintf_P(temp, html_indexPowerMode, temp1);
  data += temp;
  sendHtml(data);


  data += "<br /><b>State:</b><br />";

  // Last Error
  if (ta_MountStatus.getError() != TeenAstroMountStatus::ERR_NONE) strcpy(temp1, "</font><font class=\"y\">"); else strcpy(temp1, "");
  ta_MountStatus.getLastErrorMessage(temp2);
  strcat(temp1, temp2);
  sprintf_P(temp, html_indexLastError, temp1);
  data += temp;
  sendHtml(data);
  // Loop time
  //if (!sendCommand(":GXFA#",temp1)) strcpy(temp1,"?%");
  //sprinf_P(temp,html_indexWorkload,temp1);
  //data += temp;
  //sendHtml(data);
  data += "</div><br class=\"clear\" />\r\n";
  data += "</div></body></html>";
  sendHtml(data);
  sendHtmlDone(data);
}

