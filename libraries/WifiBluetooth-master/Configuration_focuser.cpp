#include "config.h"
#include "WifiBluetooth.h"
// -----------------------------------------------------------------------------------
// configuration_focuser
const char html_configParkFocuser[] PROGMEM =
"Speed & Acceleration: <br />"
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='Park' min='0' max='65535'>"
"<button type='submit'>Upload</button>"
" (Park position in inpulse unit)"
"</form>"
"\r\n";

const char html_configMaxPositionFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='MaxPos' min='0' max='65535'>"
"<button type='submit'>Upload</button>"
" (Max position in inpulse unit)"
"</form>"
"\r\n";

const char html_configMinSpeedFocuser[] PROGMEM =
"Speed & Acceleration: <br />"
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='MinSpeed' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Minimum Slewing speed from 1 to 999 in inpulse per 0.1sec)"
"</form>"
"\r\n";

const char html_configMaxSpeedFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='MaxSpeed' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Maximum Slewing speed from 1 to 999 in inpulse per 0.1sec)"
"</form>"
"\r\n";

const char html_configGotoAccFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='GotoAcc' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Acceleration for goto command from 1 to 100 in inpulse per 0.1sec*0.1sec)"
"</form>"
"\r\n";

const char html_configManAccFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='ManAcc' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Acceleration for manual movement from 1 to 100 in inpulse per 0.1sec*0.1sec)"
"</form>"
"\r\n";

const char html_configManDecFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='Dcc' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (Deceleration for both manual and goto from 1 to 100 in inpulse per 0.1sec*0.1sec)"
"</form>"
"\r\n";

const char html_configInpulseFocuser[] PROGMEM =
"<form method='get' action='/configuration_focuser.htm'>"
" <input value='%d' type='number' name='Inpulse' min='1' max='999'>"
"<button type='submit'>Upload</button>"
" (inpulse size)"
"</form>"
"\r\n";


#ifdef OETHS
void wifibluetooth::handleConfiguration(EthernetClient *client) {
#else
void wifibluetooth::handleConfigurationFocuser() {
#endif
  Ser.setTimeout(WebTimeout);
  serialRecvFlush();
  
  char temp[320]="";
  char temp1[80]="";
  char temp2[80]="";
  
  processConfigurationFocuserGet();

  // send a standard http response header
  String data=html_headB;
  data += html_main_cssB;
  data += html_main_css1;
  data += html_main_css2;
  data += html_main_css3;
  data += html_main_css4;
  data += html_main_css5;
  data += html_main_css6;
  data += html_main_css7;
  data += html_main_css8;
  data += html_main_cssE;
  data += html_headE;
#ifdef OETHS
  client->print(data); data="";
#endif

  data += html_bodyB;

  // get status
  mountStatus.update();

  // finish the standard http response header
  data += html_onstep_header1;
  if (mountStatus.getId(temp1)) data += temp1; else data += "?";
  data += html_onstep_header2;
  if (mountStatus.getVer(temp1)) data += temp1; else data += "?";
  data += html_onstep_header3;
  data += html_links1N;
  data += html_links2N;
#if PEC_ON
  data += html_links3N;
#endif
  data += html_links4N;
  data += html_links5S;
#ifndef OETHS
  data += html_links6N;
#endif
  data += html_onstep_header4;
  
  sendCommand(":F~#", temp1);
  if (temp1[0] = '~')
  {

    int park = (int)strtol(&temp1[1], NULL, 10);
    int maxPos = (int)strtol(&temp1[7], NULL, 10);
    int minSpeed = (int)strtol(&temp1[13], NULL, 10);
    int maxspeed = (int)strtol(&temp1[17], NULL, 10);
    int gotoAcc = (int)strtol(&temp1[21], NULL, 10);
    int manAcc= (int)strtol(&temp1[25], NULL, 10);
    int dec = (int)strtol(&temp1[29], NULL, 10);
    int reverse = temp1[33] == '1';
    int inpulse = (int)strtol(&temp1[35], NULL, 10);

    sprintf_P(temp, html_configParkFocuser, park);
    data += temp;
    sprintf_P(temp, html_configMaxPositionFocuser, maxPos);
    data += temp;
    sprintf_P(temp, html_configMinSpeedFocuser, minSpeed);
    data += temp;
    sprintf_P(temp, html_configMaxSpeedFocuser, maxspeed);
    data += temp;
    sprintf_P(temp, html_configManAccFocuser, gotoAcc);
    data += temp;
    sprintf_P(temp, html_configManAccFocuser, manAcc);
    data += temp;
    sprintf_P(temp, html_configManDecFocuser, dec);
    data += temp;
    sprintf_P(temp, html_configInpulseFocuser, inpulse);
    data += temp;
  }


#ifdef OETHS
  client->print(data); data="";
#endif

  strcpy(temp,"</div></div></body></html>");
  data += temp;

#ifdef OETHS
  client->print(data); data="";
#else
  server.send(200, "text/html",data);
#endif
}

void wifibluetooth::processConfigurationFocuserGet() {
  String v;
  int i;
  float f;
  char temp[20]="";

  v = server.arg("Park");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1000))) {
      sprintf(temp, ":SX92:%04d#", i);
      Ser.print(temp);
    }
  }

  v = server.arg("Maxpos");
  if (v != "") {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25))) {
      sprintf(temp, ":SXE2:%04d#", (int)(f*10));
      Ser.print(temp);
    }
  }

  v = server.arg("MinSpeed");
  if (v != "") {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.01) && (f <= 100))) {
      sprintf(temp, ":SX90:%03d#", (int)(f * 100));
      Ser.print(temp);
    }
  }

  v = server.arg("MaxSpeed");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) {
      sprintf(temp, ":$RR%d#", i);
      Ser.print(temp);
    }
  }

  v = server.arg("GotoAcc");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) {
      sprintf(temp, ":$RD%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("ManAcc");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000))) {
      sprintf(temp, ":$GR%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("Dcc");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000))) {
      sprintf(temp, ":$GD%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("Inpulse");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400))) {
      sprintf(temp, ":$SR%d#", i);
      Ser.print(temp);
    }
  }

  // clear any possible response
  temp[Ser.readBytesUntil('#',temp,20)]=0;
}

