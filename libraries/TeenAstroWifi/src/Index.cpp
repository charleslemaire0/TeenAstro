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
const char* html_indexAltAz PROGMEM = "Alt = <span class='c'>%s</span> &nbsp; Az = <span class='c'>%s</span><br />";
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
const char* html_indexAtHome PROGMEM = "At Home: <span class='c'>%s</span><br />";
const char* html_indexGuidingRate PROGMEM = "Guiding rate: <span class='c'>%s</span><br />";
const char* html_indexPulseGuiding PROGMEM = "Pulse guiding: <span class='c'>%s</span><br />";
const char* html_indexGuidingDir PROGMEM = "Guiding: <span class='c'>%s</span><br />";
const char* html_indexTrackCorrected PROGMEM = "Track corrected: <span class='c'>%s</span><br />";
const char* html_indexMountType PROGMEM = "Mount: <span class='c'>%s</span><br />";
const char* html_indexMotors PROGMEM = "Motors: <span class='c'>%s</span><br />";
const char* html_indexEncoders PROGMEM = "Encoders: <span class='c'>%s</span><br />";

// Builds status card inner HTML. Only reads cached mount state (no update* or serial).
// Call ensureStatusFetched() once before building so cache is fresh.
void TeenAstroWifi::buildStatusCardContent(String& data)
{
  char temp[128] = "";
  char temp1[64] = "";
  char temp2[32] = "";

  // --- Last Error (on top) ---
  data += "<div class='bt'>Last Error</div>";
  if (ta_MountStatus.getError() != TeenAstroMountStatus::ERR_NONE) strcpy(temp1, "</span><span class='y'>"); else strcpy(temp1, "");
  ta_MountStatus.getLastErrorMessage(temp2);
  strcat(temp1, temp2);
  sprintf_P(temp, html_indexLastError, temp1);
  data += temp;
  sendHtml(data);

  // --- Time & Date (mount + sidereal) ---
  data += "<div class='bt'>Time &amp; Date</div>";
  sprintf_P(temp, html_indexDate, ta_MountStatus.getUTCdate());
  data += temp;
  sprintf_P(temp, html_indexTime, ta_MountStatus.getUTC());
  data += temp;
  sprintf_P(temp, html_indexSidereal, ta_MountStatus.getSidereal());
  data += temp;
  data += FPSTR(html_settingsBrowserTime);
  sendHtml(data);

  // --- Current Jnow (RA, Dec, Alt, Az, Pier) ---
  data += "<div class='bt'>Current Jnow</div>";
  sprintf_P(temp, html_indexPosition, ta_MountStatus.getRa(), ta_MountStatus.getDec());
  data += temp;
  if (ta_MountStatus.hasInfoAlt() && ta_MountStatus.hasInfoAz())
  {
    sprintf_P(temp, html_indexAltAz, ta_MountStatus.getAlt(), ta_MountStatus.getAz());
    data += temp;
  }
  switch (ta_MountStatus.getPierState())
  {
  case TeenAstroMountStatus::PIER_W: strcpy(temp1, "West"); break;
  case TeenAstroMountStatus::PIER_E: strcpy(temp1, "East"); break;
  case TeenAstroMountStatus::PIER_UNKNOW: strcpy(temp1, "None"); break;
  default: strcpy(temp1, "?"); break;
  }
  sprintf_P(temp, html_indexPier, temp1);
  data += temp;
  sendHtml(data);

  // --- Target ---
  data += "<div class='bt'>Target</div>";
  sprintf_P(temp, html_indexPosition, ta_MountStatus.getRaT(), ta_MountStatus.getDecT());
  data += temp;

  // --- Alignment ---
  data += "<div class='bt'>Alignment</div>";
  ta_MountStatus.isAligned() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexAligned, temp1);
  data += temp;
  sendHtml(data);

  // --- Tracking ---
  data += "<div class='bt'>Tracking</div>";
  addTrackingInfo(data);
  sendHtml(data);

  // --- Operations ---
  data += "<div class='bt'>Operations</div>";
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
  ta_MountStatus.atHome() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexAtHome, temp1);
  data += temp;
  ta_MountStatus.isSpiralRunning() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexSpiral, temp1);
  data += temp;
  ta_MountStatus.isPulseGuiding() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexPulseGuiding, temp1);
  data += temp;
  sendHtml(data);
  temp1[0] = ta_MountStatus.isGuidingE() ? 'E' : (ta_MountStatus.isGuidingW() ? 'W' : '-');
  temp1[1] = ' ';
  temp1[2] = ta_MountStatus.isGuidingN() ? 'N' : (ta_MountStatus.isGuidingS() ? 'S' : '-');
  temp1[3] = '\0';
  sprintf_P(temp, html_indexGuidingDir, temp1);
  data += temp;
  switch (ta_MountStatus.getGuidingRate())
  {
  case TeenAstroMountStatus::GUIDING: strcpy(temp1, "Guiding"); break;
  case TeenAstroMountStatus::SLOW:    strcpy(temp1, "Slow"); break;
  case TeenAstroMountStatus::MEDIUM: strcpy(temp1, "Medium"); break;
  case TeenAstroMountStatus::FAST:   strcpy(temp1, "Fast"); break;
  case TeenAstroMountStatus::MAX:    strcpy(temp1, "Max"); break;
  default: strcpy(temp1, "?"); break;
  }
  sprintf_P(temp, html_indexGuidingRate, temp1);
  data += temp;
  ta_MountStatus.hasFocuser() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexHasFocuser, temp1);
  data += temp;
  sendHtml(data);
  if (ta_MountStatus.hasGNSSBoard())
  {
    if (!ta_MountStatus.atHome() && ta_MountStatus.getParkState() != TeenAstroMountStatus::PRK_PARKED)
      strcpy(temp1, "Park/Home to track");
    else if (ta_MountStatus.isGNSSValid())
    {
      strcpy(temp1, "Sat, ");
      strcat(temp1, ta_MountStatus.isGNSSTimeSync() ? "Tsync, " : "Tnosync, ");
      strcat(temp1, ta_MountStatus.isGNSSLocationSync() ? "Lsync" : "Lnosync");
    }
    else
      strcpy(temp1, "No signal");
  }
  else
    strcpy(temp1, "No board");
  sprintf_P(temp, html_indexGNSS, temp1);
  data += temp;

  // --- Mount ---
  data += "<div class='bt'>Mount</div>";
  switch (ta_MountStatus.getMount())
  {
  case TeenAstroMountStatus::MOUNT_TYPE_GEM:      strcpy(temp1, "GEM"); break;
  case TeenAstroMountStatus::MOUNT_TYPE_FORK:     strcpy(temp1, "Fork"); break;
  case TeenAstroMountStatus::MOUNT_TYPE_ALTAZM:   strcpy(temp1, "Alt-Az"); break;
  case TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT: strcpy(temp1, "Fork Alt-Az"); break;
  default: strcpy(temp1, "?"); break;
  }
  sprintf_P(temp, html_indexMountType, temp1);
  data += temp;
  ta_MountStatus.motorsEnableCached() ? strcpy(temp1, "On") : strcpy(temp1, "Off");
  sprintf_P(temp, html_indexMotors, temp1);
  data += temp;
  ta_MountStatus.encodersEnableCached() ? strcpy(temp1, "On") : strcpy(temp1, "Off");
  sprintf_P(temp, html_indexEncoders, temp1);
  data += temp;
  ta_MountStatus.isTrackingCorrected() ? strcpy(temp1, "Yes") : strcpy(temp1, "No");
  sprintf_P(temp, html_indexTrackCorrected, temp1);
  data += temp;
  sendHtml(data);
}

// Single bulk status update (:GXAS#) for the index page — avoids a long list of
// separate serial round-trips and keeps the page stable.
static void ensureStatusFetched()
{
  ta_MountStatus.updateAllState();
}

void TeenAstroWifi::statusAjax()
{
  ensureStatusFetched();
  sendHtmlStart();
  String data;
  buildStatusCardContent(data);
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::handleRoot()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  String data;
  // Single :GXAS# update; then index build does not modify mount state (read-only cache).
  ensureStatusFetched();
  preparePage(data, ServerPage::Index);
  sendHtml(data);

  data += "<div class='card' id='StatusCard'>";
  data += "<div id='StatusContent'>";
  sendHtml(data);
  buildStatusCardContent(data);
  data += "</div>";
  data += "</div>";
  sendHtml(data);
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}
