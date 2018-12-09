#include "config.h"
#include "WifiBluetooth.h"
// -----------------------------------------------------------------------------------
// The home page, status information

#ifdef ADVANCED_CHARS_ON
  #define Axis1 "&alpha;"
  #define Axis1A "&alpha;"
  #define Axis2 "&delta;"
#else
  #define Axis1 "RA"
  #define Axis1A "RA"
  #define Axis2 "DEC"
#endif

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

#ifdef AMBIENT_CONDITIONS_ON
const char* html_indexTPHD = "&nbsp;&nbsp;%s <font class='c'>%s</font>%s<br />";
#endif

const char* html_indexLastError PROGMEM = "&nbsp;&nbsp;Last Error: <font class='c'>%s</font><br />";
const char* html_indexWorkload PROGMEM = "&nbsp;&nbsp;Workload: <font class='c'>%s</font><br />";

#ifdef OETHS
void wifibluetooth::handleRoot(EthernetClient *client) {
#else
void wifibluetooth::handleRoot() {
#endif
  Ser.setTimeout(WebTimeout);
  serialRecvFlush();

  char temp[300]="";
  char temp1[80]="";
  char temp2[80]="";

  String data;
  preparePage(data, 1);

  data+="<div style='width: 27em;'>";

  data+="<b>Time and Date:</b><br />";
  // Browser time
  data += html_settingsBrowserTime;

  // UTC Date
  if (!sendCommand(":GX81#",temp1)) strcpy(temp1,"?");
  sprintf(temp,html_indexDate,temp1);
  data += temp;

  // UTC Time
  if (!sendCommand(":GX80#",temp1)) strcpy(temp1,"?");
  sprintf(temp,html_indexTime,temp1);
  data += temp;

  // LST
  if (!sendCommand(":GS#",temp1)) strcpy(temp1,"?");
  sprintf(temp,html_indexSidereal,temp1);
  data += temp;

#ifdef OETHS
  client->print(data); data="";
#endif

#ifdef AMBIENT_CONDITIONS_ON
  if (!sendCommand(":GX9A#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Temperature:",temp1,"&deg;C"); data+=temp;
  if (!sendCommand(":GX9B#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Barometric Pressure:",temp1,"mb"); data+=temp;
  if (!sendCommand(":GX9C#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Relative Humidity:",temp1,"%"); data+=temp;
  if (!sendCommand(":GX9E#",temp1)) strcpy(temp1,"?"); sprintf(temp,html_indexTPHD,"Dew Point Temperature:",temp1,"&deg;C"); data+=temp;
#endif

  data+="<br /><b>Current Jnow Coordinates:</b><br />";

  // RA,Dec current
  if (!sendCommand(":GR#",temp1)) strcpy(temp1,"?");
  if (!sendCommand(":GD#",temp2)) strcpy(temp2,"?");
  sprintf(temp,html_indexPosition,temp1,temp2); 
  data += temp;
  // pier side and meridian flips
  if ((mountStatus.pierSide() == PierSideFlipWE1) || (mountStatus.pierSide() == PierSideFlipWE2) || (mountStatus.pierSide() == PierSideFlipWE3)) strcpy(temp1, "Meridian Flip, West to East"); else
    if ((mountStatus.pierSide() == PierSideFlipEW1) || (mountStatus.pierSide() == PierSideFlipEW2) || (mountStatus.pierSide() == PierSideFlipEW3)) strcpy(temp1, "Meridian Flip, East to West"); else
      if (mountStatus.pierSide() == PierSideWest) strcpy(temp1, "West"); else
        if (mountStatus.pierSide() == PierSideEast) strcpy(temp1, "East"); else
          if (mountStatus.pierSide() == PierSideNone) strcpy(temp1, "None"); else strcpy(temp1, "Unknown");
  if (!mountStatus.valid()) strcpy(temp1, "?");
  if (mountStatus.meridianFlips()) {
    strcpy(temp2, "On");
    if (mountStatus.autoMeridianFlips()) strcat(temp2, "</font>, <font class=\"c\">Auto");
  }
  else strcpy(temp2, "Off");
  if (!mountStatus.valid()) strcpy(temp2, "?");
  sprintf(temp, html_indexPier, temp1, temp2);
  data += temp;

  // RA,Dec target
  data += "<br /><b>Last Jnow Target Coordinates:</b><br />";
  if (!sendCommand(":Gr#",temp1)) strcpy(temp1,"?");
  if (!sendCommand(":Gd#",temp2)) strcpy(temp2,"?");
  sprintf(temp,html_indexPosition,temp1,temp2); 
  data += temp;

#ifdef ENCODERS_ON
  // RA,Dec OnStep position
  double f;
  f=encoders.getOnStepAxis1(); doubleToDms(temp1,&f,true,true);
  f=encoders.getOnStepAxis2(); doubleToDms(temp2,&f,true,true);
  sprintf(temp,html_indexEncoder1,temp1,temp2);
  data += temp;

  // RA,Dec encoder position
  f=encoders.getAxis1(); doubleToDms(temp1,&f,true,true);
  f=encoders.getAxis2(); doubleToDms(temp2,&f,true,true);
  sprintf(temp,html_indexEncoder2,temp1,temp2);
  data += temp;
#endif



#ifdef OETHS
  client->print(data); data="";
#endif

  //data+="<br /><b>Alignment:</b><br />";

  //if ((mountStatus.mountType()== MountStatus::MT_GEM) || (mountStatus.mountType()== MountStatus::MT_FORK)) {
  //  long altCor=0; if (sendCommand(":GX02#",temp1)) { altCor=strtol(&temp1[0],NULL,10); }
  //  long azmCor=0; if (sendCommand(":GX03#",temp1)) { azmCor=strtol(&temp1[0],NULL,10); }
  //  sprintf(temp,html_indexCorPolar,(long)(altCor),(long)(azmCor));
  //  data += temp;
  //}
#ifdef OETHS
  client->print(data); data="";
#endif

  data+="<br /><b>Operations:</b><br />";

  // Park
  if (mountStatus.parked()) strcpy(temp1,"Parked"); else strcpy(temp1,"Not Parked");
  if (mountStatus.parking()) strcpy(temp1,"Parking"); else
  if (mountStatus.parkFail()) strcpy(temp1,"Park Failed");
  if (mountStatus.atHome()) strcat(temp1," </font>(<font class=\"c\">At Home</font>)<font class=\"c\">");
  if (!mountStatus.valid()) strcpy(temp1,"?");
  sprintf(temp,html_indexPark,temp1);
  data += temp;

  // Tracking
  if (mountStatus.tracking()) strcpy(temp1,"On"); else strcpy(temp1,"Off");
  if (mountStatus.slewing()) strcpy(temp1,"Slewing");
  if (!mountStatus.valid()) strcpy(temp1,"?");
  
  strcpy(temp2,"</font>(<font class=\"c\">");
  if (mountStatus.ppsSync()) strcat(temp2,"PPS Sync, ");
  if (mountStatus.rateCompensation()== MountStatus::RC_REFR_RA)   strcat(temp2,"Refr Comp RA Axis, ");
  if (mountStatus.rateCompensation()== MountStatus::RC_REFR_BOTH) strcat(temp2,"Refr Comp Both Axis, ");
  if (mountStatus.rateCompensation()== MountStatus::RC_FULL_RA)   strcat(temp2,"Full Comp RA Axis, ");
  if (mountStatus.rateCompensation()== MountStatus::RC_FULL_BOTH) strcat(temp2,"Full Comp Both Axis, ");
  if (!mountStatus.valid()) strcpy(temp2,"?");
  if (temp2[strlen(temp2)-2]==',') { temp2[strlen(temp2)-2]=0; strcat(temp2,"</font>)<font class=\"c\">"); } else strcpy(temp2,"");
  sprintf_P(temp,html_indexTracking,temp1,temp2);
  data += temp;

  // Tracking rate
  if ((sendCommand(":GT#",temp1)) && (strlen(temp1)>6)) {
    double tr=atof(temp1);
    sprintf(temp,"&nbsp;&nbsp;Tracking Rate: <font class=\"c\">%5.3f</font>Hz<br />",tr);
    data += temp;
  }

  
#ifdef OETHS
  client->print(data); data="";
#endif

  data+="<br /><b>State:</b><br />";

  // Last Error
  if (mountStatus.lastError()!=MountStatus::ERR_NONE) strcpy(temp1,"</font><font class=\"y\">"); else strcpy(temp1,"");
  mountStatus.getLastErrorMessage(temp2);
  strcat(temp1,temp2);
  if (!mountStatus.valid()) strcpy(temp1,"?");
  sprintf(temp,html_indexLastError,temp1);
  data += temp;

  // Loop time
  if (!sendCommand(":GXFA#",temp1)) strcpy(temp1,"?%");
  sprintf(temp,html_indexWorkload,temp1);
  data += temp;

  data += "</div><br class=\"clear\" />\r\n";
  data += "</div></body></html>";

#ifdef OETHS
  client->print(data);
#else
  server.send(200, "text/html",data);
#endif
}

