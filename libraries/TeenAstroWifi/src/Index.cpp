#include "TeenAstroWifi.h"
#include "HtmlCommon.h"

// -----------------------------------------------------------------------------------
// The home page, status information

#define Axis1 "&alpha;"
#define Axis2 "&delta;"

const char* html_settingsBrowserTime PROGMEM =
"<span id=\"datetime\"></span> UT (web browser)"
"<script> "
"function pad(num, size) { var s = '000000000' + num; return s.substr(s.length-size); }"
"var now = new Date(); document.getElementById('datetime').innerHTML = (now.getUTCMonth()+1).toString()+'/'+"
"pad(now.getUTCDate().toString(),2)+'/'+pad(now.getUTCFullYear().toString(),4)+"
"' '+pad(now.getUTCHours().toString(),2)+':'+pad(now.getUTCMinutes().toString(),2)+':'+pad(now.getUTCSeconds().toString(),2); "
"</script><br />\r\n";

const char* html_indexDate PROGMEM = "<span class='c'>%s</span>";
const char* html_indexTime PROGMEM = " <span class='c'>%s</span> UT";
const char* html_indexSidereal PROGMEM = " (<span class='c'>%s</span> LST)<br />";

const char* html_indexPosition PROGMEM = Axis1 " = <span class='c'>%s</span> &nbsp; " Axis2 " = <span class='c'>%s</span><br />";
const char* html_indexTarget PROGMEM = "Target: " Axis1 " = <span class='c'>%s</span> &nbsp; " Axis2 " = <span class='c'>%s</span><br />";

const char* html_indexPier PROGMEM = "Pier Side: <span class='c'>%s</span><br />";

const char* html_indexPark PROGMEM = "Parking: <span class='c'>%s</span><br />";
const char* html_indexHasFocuser PROGMEM = "Has Focuser: <span class='c'>%s</span><br />";
const char* html_indexGNSS PROGMEM = "GNSS: <span class='c'>%s</span><br />";
const char* html_indexPowerMode PROGMEM = "Power mode: <span class='c'>%s</span><br />";
const char* html_indexAligned PROGMEM = "Aligned: <span class='c'>%s</span><br />";

const char* html_indexGuiding PROGMEM = "Guiding: <span class='c'>%s</span><br />";
const char* html_indexSpiral PROGMEM = "Spiral running: <span class='c'>%s</span><br />";

const char* html_indexLastError PROGMEM = "Last Error: <span class='c'>%s</span><br />";
const char* html_indexWorkload PROGMEM = "Workload: <span class='c'>%s</span><br />";


void TeenAstroWifi::handleRoot()
{
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[300] = "";
  char temp1[80] = "";
  char temp2[80] = "";

  //updates
  ta_MountStatus.updateTime();
  ta_MountStatus.updateRaDec();
  ta_MountStatus.updateMount();
  ta_MountStatus.updateRaDecT();

  String data;
  preparePage(data, ServerPage::Index);
  sendHtml(data);

  // --- Time and Date card ---
  data += "<div class='card'><div class='bt'>Time &amp; Date</div>";
  data += FPSTR(html_settingsBrowserTime);
  sendHtml(data);
  sprintf_P(temp, html_indexDate, ta_MountStatus.getUTCdate());
  data += temp;
  sprintf_P(temp, html_indexTime, ta_MountStatus.getUTC());
  data += temp;
  sendHtml(data);
  sprintf_P(temp, html_indexSidereal, ta_MountStatus.getSidereal());
  data += temp;
  data += "</div>";
  sendHtml(data);

  // --- Current Coordinates card ---
  data += "<div class='card'><div class='bt'>Current Jnow Coordinates</div>";
  sprintf_P(temp, html_indexPosition, ta_MountStatus.getRa(), ta_MountStatus.getDec());
  data += temp;
  sendHtml(data);

  // pier side
  switch (ta_MountStatus.getPierState())
  {
  case TeenAstroMountStatus::PIER_W: strcpy(temp1, "West"); break;
  case TeenAstroMountStatus::PIER_E: strcpy(temp1, "East"); break;
  case TeenAstroMountStatus::PIER_UNKNOW: strcpy(temp1, "None"); break;
  default: strcpy(temp1, "Unknown"); break;
  }
  sprintf_P(temp, html_indexPier, temp1);
  data += temp;
  data += "</div>";
  sendHtml(data);

  // --- Target card ---
  data += "<div class='card'><div class='bt'>Last Jnow Target</div>";
  sprintf_P(temp, html_indexPosition, ta_MountStatus.getRaT(), ta_MountStatus.getDecT());
  data += temp;
  data += "</div>";
  sendHtml(data);

  // --- Alignment card ---
  data += "<div class='card'><div class='bt'>Alignment</div>";
  ta_MountStatus.isAligned() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexAligned, temp1);
  data += temp;
  data += "</div>";
  sendHtml(data);

  // --- Operations card ---
  data += "<div class='card'><div class='bt'>Operations</div>";
  switch (ta_MountStatus.getParkState())
  {
  case TeenAstroMountStatus::PRK_UNPARKED: strcpy(temp1, "Not Parked"); break;
  case TeenAstroMountStatus::PRK_PARKED: strcpy(temp1, "Parked"); break;
  case TeenAstroMountStatus::PRK_FAILED: strcpy(temp1, "Park Failed"); break;
  case TeenAstroMountStatus::PRK_PARKING: strcpy(temp1, "Parking"); break;
  default: strcpy(temp1, "?"); break;
  }
  sprintf_P(temp, html_indexPark, temp1);
  data += temp;
  sendHtml(data);

  ta_MountStatus.isSpiralRunning() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexSpiral, temp1);
  data += temp;

  ta_MountStatus.hasFocuser() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexHasFocuser, temp1);
  data += temp;
  sendHtml(data);

  // GNSS
  if (ta_MountStatus.hasGNSSBoard())
  {
    if (!ta_MountStatus.atHome() && ta_MountStatus.getParkState() != TeenAstroMountStatus::PRK_PARKED)
      strcpy(temp1, "GNSS board detected, goto park or home to track satellite");
    else if (ta_MountStatus.isGNSSValid())
    {
      strcpy(temp1, "Tracking Satellite, ");
      ta_MountStatus.isGNSSTimeSync() ? strcpy(temp2, "Time Sync, ") : strcpy(temp2, "Time Not Sync, ");
      strcat(temp1, temp2);
      ta_MountStatus.isGNSSLocationSync() ? strcpy(temp2, "Loc. Sync") : strcpy(temp2, "Loc. Not Sync");
      strcat(temp1, temp2);
    }
    else
      strcpy(temp1, "GNSS board detected, no signal");
  }
  else
    strcpy(temp1, "No GNSS board detected");

  sprintf_P(temp, html_indexGNSS, temp1);
  data += temp;
  data += "</div>";
  sendHtml(data);

  // --- State card ---
  data += "<div class='card'><div class='bt'>State</div>";
  if (ta_MountStatus.getError() != TeenAstroMountStatus::ERR_NONE) strcpy(temp1, "</span><span class='y'>"); else strcpy(temp1, "");
  ta_MountStatus.getLastErrorMessage(temp2);
  strcat(temp1, temp2);
  sprintf_P(temp, html_indexLastError, temp1);
  data += temp;
  data += "</div>";
  sendHtml(data);

  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
}
