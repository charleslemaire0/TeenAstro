#include <TeenAstroLX200io.h>
#include "config.h"
#include "WifiBluetooth.h"

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
#ifdef ENCODERS_ON
const char* html_indexEncoder1 = "&nbsp;&nbsp;OnStep: Ax1=<font class='c'>%s</font>, Ax2=<font class='c'>%s</font><br />";
const char* html_indexEncoder2 = "&nbsp;&nbsp;Encodr: Ax1=<font class='c'>%s</font>, Ax2=<font class='c'>%s</font><br />";
#endif
const char* html_indexPier PROGMEM = "&nbsp;&nbsp;<font class='c'>%s</font> Pier Side (meridian flips <font class='c'>%s</font>)<br />";

const char* html_indexCorPolar PROGMEM = "&nbsp;&nbsp;Polar Offset: &Delta; Alt=<font class='c'>%ld</font>\", &Delta; Azm=<font class='c'>%ld</font>\"<br />";
const char* html_indexPark PROGMEM = "&nbsp;&nbsp;Parking: <font class='c'>%s</font><br />";
const char* html_indexTracking PROGMEM = "&nbsp;&nbsp;Tracking: <font class='c'>%s %s</font><br />";

const char* html_indexLastError PROGMEM = "&nbsp;&nbsp;Last Error: <font class='c'>%s</font><br />";
const char* html_indexWorkload PROGMEM = "&nbsp;&nbsp;Workload: <font class='c'>%s</font><br />";


void wifibluetooth::handleRoot() {
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[300]="";
  char temp1[80]="";
  char temp2[80]="";
  //updates
  ta_MountStatus.updateV();
  ta_MountStatus.updateTime();
  ta_MountStatus.updateRaDec();
  ta_MountStatus.updateMount();
  ta_MountStatus.updateRaDecT();
  ta_MountStatus.updateTrackingRate();

  String data;
  preparePage(data, 1);
  sendHtml(data);
  data+="<div style='width: 27em;'>";

  data+="<b>Time and Date:</b><br />";
  // Browser time
  data += html_settingsBrowserTime;
  sendHtml(data);
  // UTC Date
  sprintf(temp, html_indexDate, ta_MountStatus.getUTCdate());
  data += temp;
  sendHtml(data);
  // UTC Time
  sprintf(temp,html_indexTime, ta_MountStatus.getUTC());
  data += temp;
  sendHtml(data);
  // LST
  sprintf(temp,html_indexSidereal, ta_MountStatus.getSideral());
  data += temp;
  sendHtml(data);


#ifdef AMBIENT_CONDITIONS_ON
  if (!sendCommand(":GX9A#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Temperature:",temp1,"&deg;C"); data+=temp;
  if (!sendCommand(":GX9B#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Barometric Pressure:",temp1,"mb"); data+=temp;
  if (!sendCommand(":GX9C#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Relative Humidity:",temp1,"%"); data+=temp;
  if (!sendCommand(":GX9E#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Dew Point Temperature:",temp1,"&deg;C"); data+=temp;
#endif

  data+="<br /><b>Current Jnow Coordinates:</b><br />";

  // RA,Dec current

  sprintf(temp,html_indexPosition, ta_MountStatus.getRa(), ta_MountStatus.getDec());
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
  //
  //todo
  //if (mountStatus.meridianFlips()) {
  //  strcpy(temp2, "On");
  //  if (mountStatus.autoMeridianFlips()) strcat(temp2, "</font>, <font class=\"c\">Auto");
  //}
  //else strcpy(temp2, "Off");
  //if (!ta_MountStatus.validConnection()()) strcpy(temp2, "?");
  strcpy(temp2, "?");
  sprintf(temp, html_indexPier, temp1, temp2);
  data += temp;
  sendHtml(data);
  // RA,Dec target

  data += "<br /><b>Last Jnow Target Coordinates:</b><br />";
  sprintf(temp,html_indexPosition, ta_MountStatus.getRaT(), ta_MountStatus.getDecT());
  data += temp;
  sendHtml(data);


  //data+="<br /><b>Alignment:</b><br />";

  //if ((mountStatus.mountType()== MountStatus::MT_GEM) || (mountStatus.mountType()== MountStatus::MT_FORK)) {
  //  long altCor=0; if (sendCommand(":GX02#",temp1)) { altCor=strtol(&temp1[0],NULL,10); }
  //  long azmCor=0; if (sendCommand(":GX03#",temp1)) { azmCor=strtol(&temp1[0],NULL,10); }
  //  sprintf(temp,html_indexCorPolar,(long)(altCor),(long)(azmCor));
  //  data += temp;
  //}


  data+="<br /><b>Operations:</b><br />";

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
  sprintf(temp,html_indexPark,temp1);
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

  strcpy(temp2,"</font>(<font class=\"c\">");
  //if (mountStatus.rateCompensation()== MountStatus::RC_REFR_RA)   strcat(temp2,"Refr Comp RA Axis, ");
  //if (mountStatus.rateCompensation()== MountStatus::RC_REFR_BOTH) strcat(temp2,"Refr Comp Both Axis, ");
  //if (mountStatus.rateCompensation()== MountStatus::RC_FULL_RA)   strcat(temp2,"Full Comp RA Axis, ");
  //if (mountStatus.rateCompensation()== MountStatus::RC_FULL_BOTH) strcat(temp2,"Full Comp Both Axis, ");
  if (temp2[strlen(temp2)-2]==',') { temp2[strlen(temp2)-2]=0; strcat(temp2,"</font>)<font class=\"c\">"); } else strcpy(temp2,"");
  sprintf_P(temp,html_indexTracking,temp1,temp2);
  data += temp;
  sendHtml(data);
  // Tracking rate
  sprintf(temp, "&nbsp;&nbsp;Tracking Rate: <font class=\"c\">%s</font>Hz<br />", ta_MountStatus.getTrackingRate());
  data += temp;
  switch (ta_MountStatus.getSideralMode())
  {
  case TeenAstroMountStatus::SID_STAR:
    strcpy(temp2, "Sideral");
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

  data+="<br /><b>State:</b><br />";

  // Last Error
  if (ta_MountStatus.getError()!= TeenAstroMountStatus::ERR_NONE) strcpy(temp1,"</font><font class=\"y\">"); else strcpy(temp1,"");
  ta_MountStatus.getLastErrorMessage(temp2);
  strcat(temp1,temp2);
  sprintf(temp,html_indexLastError,temp1);
  data += temp;
  sendHtml(data);
  // Loop time
  //if (!sendCommand(":GXFA#",temp1)) strcpy(temp1,"?%");
  //sprintf(temp,html_indexWorkload,temp1);
  //data += temp;
  //sendHtml(data);
  data += "</div><br class=\"clear\" />\r\n";
  data += "</div></body></html>";
  sendHtml(data);
  sendHtmlDone(data);
}

