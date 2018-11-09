#include "config.h"
#include "WifiBluetooth.h"
// -----------------------------------------------------------------------------------
// Configuration


const char html_configMount_1[] =
"Equatorial Mount Type: <br />"
"<form action='/configuration.htm'>"
"<select name='mount'>";
//"<option value = "1">German< / option>"
//"<option value = "2">Fork< / option>"
const char html_configMount_2[] =
"</select>"
"<button type='submit'>Upload</button>"
"</form>"
"<br/>\r\n";

const char html_configMaxRate[] =
"Rate & Acceleration: <br />"
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='MaxR' min='32' max='1000'>"
"<button type='submit'>Upload</button>"
" (Maximum Slewing speed from 32x to 1000x)"
"</form>"
"\r\n";
const char html_configGuideRate[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%.2f' type='number' name='GuideR' min='0.01' max='1' step='.01'>"
"<button type='submit'>Upload</button>"
" (Guiding speed from 0.01x to 1x)"
"</form>"
"\r\n";
const char html_configAcceleration[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%.1f' type='number' name='Acc' min='0.1' max='25' step='.1'>"
"<button type='submit'>Upload</button>"
" (Acceleration, number of degrees to reach the Max Rate from 0.1° to 25°)"
"</form>"
"<br />\r\n";
const char html_configRotAxis_1[] =
"<form action='/configuration.htm'>"
"<select name='mrot%d'>";
//"<option value = "0">Direct< / option>"
//"<option value = "1">Reverse< / option>"

const char html_configRotAxis_r[] =
"<option value ='0'>Direct</option>"
"<option selected value='1'>Reverse</option>";
const char html_configRotAxis_d[] =
"<option selected value ='0'>Direct</option>"
"<option value='1'>Reverse</option>";

const char html_configRotAxis_2[] =
"</select>"
"<button type='submit'>Upload</button>"
" (Rotation Axis%d)"
"</form>"
"\r\n";

const char html_configBlAxis[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='mbl%d' min='0' max='999'>"
"<button type='submit'>Upload</button>"
" (Backlash Axis%d, in arc-seconds from 0 to 999)"
"</form>"
"\r\n";
const char html_configGeAxis[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='mge%d' min='1' max='60000'>"
"<button type='submit'>Upload</button>"
" (Gear Axis%d, from 1 to 60000)"
"</form>"
"\r\n";
const char html_configStAxis[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='mst%d' min='1' max='400'>"
"<button type='submit'>Upload</button>"
" (Steps per Rotation Axis%d, from 1 to 400)"
"</form>"
"\r\n";
const char html_configMuAxis[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='mmu%d' min='16' max='256'>"
"<button type='submit'>Upload</button>"
" (Microsteps Axis%d, valid value are 16, 32, 64, 128, 256)"
"</form>"
"\r\n";
const char html_configLCAxis[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='mlc%d' min='100' max='2000' step='10'>"
"<button type='submit'>Upload</button>"
" (Low Current Axis%d, from 100mA to 2000mA)"
"</form>"
"\r\n";
const char html_configHCAxis[] =
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='mhc%d' min='100' max='2000' step='10'>"
"<button type='submit'>Upload</button>"
" (High Current Axis%d, from 100mA to 2000mA)"
"</form>"
"\r\n";


const char html_configMinAlt[] = 
"Limits: <br />"
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='hl' min='-30' max='30'>"
"<button type='submit'>Upload</button>"
" (Horizon, in degrees +/- 30)"
"</form>"
"\r\n";
const char html_configMaxAlt[] = 
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='ol' min='60' max='90'>"
"<button type='submit'>Upload</button>"
" (Overhead, in degrees 60 to 90)"
"</form>"
"\r\n";
const char html_configPastMerE[] = 
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='el' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
" (Past Meridian when East of the pier, in degrees +/-45)"
"</form>"
"\r\n";
const char html_configPastMerW[] = 
"<form method='get' action='/configuration.htm'>"
" <input value='%d' type='number' name='wl' min='-45' max='45'>"
"<button type='submit'>Upload</button>"
" (Past Meridian when West of the pier, in degrees +/-45)"
"</form>"
"<br />\r\n";
const char html_configLongDeg[] = 
"Location: <br />"
"<form method='get' action='/configuration.htm'>"
" <input value='%s' type='number' name='g1' min='-180' max='180'>&nbsp;&deg;&nbsp;";
const char html_configLongMin[] = 
" <input value='%s' type='number' name='g2' min='0' max='60'>&nbsp;'&nbsp;&nbsp;"
"<button type='submit'>Upload</button>"
" (Longitude, in deg. and min. +/- 180)"
"</form>"
"\r\n";
const char html_configLatDeg[] = 
"<form method='get' action='/configuration.htm'>"
" <input value='%s' type='number' name='t1' min='-90' max='90'>&nbsp;&deg;&nbsp;";
const char html_configLatMin[] =
" <input value='%s' type='number' name='t2' min='0' max='60'>&nbsp;'&nbsp;&nbsp;"
"<button type='submit'>Upload</button>"
" (Latitude, in deg. and min. +/- 90)"
"</form>"
"<br />\r\n";

#ifdef OETHS
void wifibluetooth::handleConfiguration(EthernetClient *client) {
#else
void wifibluetooth::handleConfiguration() {
#endif
  Ser.setTimeout(WebTimeout);
  serialRecvFlush();
  
  char temp[320]="";
  char temp1[80]="";
  char temp2[80]="";
  
  processConfigurationGet();

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
  data += html_links5S;
#ifndef OETHS
  data += html_links6N;
#endif
  data += html_onstep_header4;

  if (sendCommand(":GU#", temp1))
  {
    data += html_configMount_1;
    temp1[12] == 'E' ? data += "<option selected value='1'>German</option>" : data += "<option value='1'>German</option>";
    temp1[12] == 'K' ? data += "<option selected value='2'>Fork</option>" : data += "<option value='2'>Fork</option>";
    data += html_configMount_2;
  }

  //if (!sendCommand(":GX90#", temp1)) strcpy(temp1, "0"); int mountType = (int)strtol(&temp1[0], NULL, 10);
  //sprintf(temp, html_configMount, mountType);
  //data += temp;
  if (!sendCommand(":GX92#", temp1)) strcpy(temp1, "0"); int maxRate = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configMaxRate, maxRate);
  data += temp;
  if (!sendCommand(":GX90#", temp1)) strcpy(temp1, "0"); float guideRate = (float)strtof(&temp1[0], NULL);
  sprintf(temp, html_configGuideRate, guideRate);
  data += temp;
  if (!sendCommand(":GXE2#", temp1)) strcpy(temp1, "0"); float acc = (float)strtof(&temp1[0], NULL);
  sprintf(temp, html_configAcceleration, acc);
  data += temp;


  data+="<div style='width: 35em;'>";


#ifdef OETHS
  client->print(data); data="";
#endif
  //Axis1

  data += "Motor Configuration: <br />";
  if (!sendCommand(":%RR#", temp1)) strcpy(temp1, "0"); int reverse = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configRotAxis_1, 1);
  data += temp;
  data += reverse ? html_configRotAxis_r : html_configRotAxis_d;
  sprintf(temp, html_configRotAxis_2, 1);
  data += temp;
  if (!sendCommand(":%RD#", temp1)) strcpy(temp1, "0"); reverse = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configRotAxis_1, 2);
  data += temp;
  data += reverse ? html_configRotAxis_r : html_configRotAxis_d;
  sprintf(temp, html_configRotAxis_2, 2);
  data += temp;
  if (!sendCommand(":%GR#", temp1)) strcpy(temp1, "0"); int gear = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configGeAxis, gear, 1, 1);
  data += temp;
  if (!sendCommand(":%GD#", temp1)) strcpy(temp1, "0"); gear = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configGeAxis, gear, 2, 2);
  data += temp;
  if (!sendCommand(":%SR#", temp1)) strcpy(temp1, "0"); int step = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configStAxis, step, 1, 1);
  data += temp;
  if (!sendCommand(":%SD#", temp1)) strcpy(temp1, "0"); step = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configStAxis, step, 2, 2);
  data += temp;
  if (!sendCommand(":%MR#", temp1)) strcpy(temp1, "0"); int micro = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configMuAxis, (int)pow(2.,micro), 1, 1);
  data += temp;
  if (!sendCommand(":%MD#", temp1)) strcpy(temp1, "0"); micro = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configMuAxis, (int)pow(2.,micro), 2, 2);
  data += temp;
  if (!sendCommand(":%BR#", temp1)) strcpy(temp1, "0"); int backlashAxis = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configBlAxis, backlashAxis, 1, 1);
  data += temp;
  if (!sendCommand(":%BD#", temp1)) strcpy(temp1, "0"); backlashAxis = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configBlAxis, backlashAxis, 2, 2);
  data += temp;
  if (!sendCommand(":%cR#", temp1)) strcpy(temp1, "0"); int lowC = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configLCAxis, lowC*10, 1, 1);
  data += temp;
  if (!sendCommand(":%cD#", temp1)) strcpy(temp1, "0"); lowC = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configLCAxis, lowC*10, 2, 2);
  data += temp;
  if (!sendCommand(":%CR#", temp1)) strcpy(temp1, "0"); int highC = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configHCAxis, highC*10, 1, 1);
  data += temp;
  if (!sendCommand(":%CD#", temp1)) strcpy(temp1, "0"); highC = (int)strtol(&temp1[0], NULL, 10);
  sprintf(temp, html_configHCAxis, highC*10, 2, 2);
  data += temp;
  data += "<br />";

#ifdef OETHS
  client->print(data); data="";
#endif

  // Overhead and Horizon Limits
  if (!sendCommand(":Gh#",temp1)) strcpy(temp1,"0"); int minAlt=(int)strtol(&temp1[0],NULL,10);
  sprintf(temp,html_configMinAlt,minAlt);
  data += temp;
  if (!sendCommand(":Go#",temp1)) strcpy(temp1,"0"); int maxAlt=(int)strtol(&temp1[0],NULL,10);
  sprintf(temp,html_configMaxAlt,maxAlt);
  data += temp;

  // Meridian Limits
  if ((sendCommand(":GXE9#",temp1)) && (sendCommand(":GXEA#",temp2))) {
    int degPastMerE=(int)strtol(&temp1[0],NULL,10);
    degPastMerE=round((degPastMerE*15.0)/60.0);
    sprintf(temp,html_configPastMerE,degPastMerE);
    data += temp;
    int degPastMerW=(int)strtol(&temp2[0],NULL,10);
    degPastMerW=round((degPastMerW*15.0)/60.0);
    sprintf(temp,html_configPastMerW,degPastMerW);
    data += temp;
  } else data += "<br />\r\n";
#ifdef OETHS
  client->print(data); data="";
#endif

  // Longitude
  if (!sendCommand(":Gg#",temp1)) strcpy(temp1,"+000*00");
  temp1[4]=0; // deg. part only
  if (temp1[0]=='+') temp1[0]='0'; // remove +
  sprintf(temp,html_configLongDeg,temp1);
  data += temp;
  sprintf(temp,html_configLongMin,(char*)&temp1[5]);
  data += temp;
#ifdef OETHS
  client->print(data); data="";
#endif

  // Latitude
  if (!sendCommand(":Gt#",temp1)) strcpy(temp1,"+00*00");
  temp1[3]=0; // deg. part only
  if (temp1[0]=='+') temp1[0]='0'; // remove +
  sprintf(temp,html_configLatDeg,temp1);
  data += temp;
  sprintf(temp,html_configLatMin,(char*)&temp1[4]);
  data += temp;
#ifdef OETHS
  client->print(data); data="";
#endif


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

void wifibluetooth::processConfigurationGet() {
  String v;
  int i;
  float f;
  char temp[20]="";

  v = server.arg("MaxR");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1000))) {
      sprintf(temp, ":SX92:%04d#", i);
      Ser.print(temp);
    }
  }

  v = server.arg("Acc");
  if (v != "") {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25))) {
      sprintf(temp, ":SXE2:%04d#", (int)(f*10));
      Ser.print(temp);
    }
  }

  v = server.arg("GuideR");
  if (v != "") {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.01) && (f <= 100))) {
      sprintf(temp, ":SX90:%03d#", (int)(f * 100));
      Ser.print(temp);
    }
  }

  v = server.arg("mrot1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) {
      sprintf(temp, ":$RR%d#", i);
      Ser.print(temp);
    }
  }

  v = server.arg("mrot2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) {
      sprintf(temp, ":$RD%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mge1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000))) {
      sprintf(temp, ":$GR%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mge2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 60000))) {
      sprintf(temp, ":$GD%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mst1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400))) {
      sprintf(temp, ":$SR%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mst2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400))) {
      sprintf(temp, ":$SD%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mmu1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 16) && (i <= 256))) {
      sprintf(temp, ":$MR%d#", (int)log2(i));
      Ser.print(temp);
    }
  }
  v = server.arg("mmu2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 16) && (i <= 256))) {
      sprintf(temp, ":$MD%d#", (int)log2(i));
      Ser.print(temp);
    }
  }
  v = server.arg("mbl1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999))) {
      sprintf(temp, ":$BR%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mbl2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999))) {
      sprintf(temp, ":$BD%d#", i);
      Ser.print(temp);
    }
  }
  v = server.arg("mlc1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000))) {
      sprintf(temp, ":$cR%d#", i/10);
      Ser.print(temp);
    }
  }
  v = server.arg("mlc2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000))) {
      sprintf(temp, ":$cD%d#", i/10);
      Ser.print(temp);
    }
  }
  v = server.arg("mhc1");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000))) {
      sprintf(temp, ":$CR%d#", i/10);
      Ser.print(temp);
    }
  }
  v = server.arg("mhc2");
  if (v != "") {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 100) && (i <= 2000))) {
      sprintf(temp, ":$CD%d#", i/10);
      Ser.print(temp);
    }
  }
  // Overhead and Horizon Limits
  v=server.arg("ol");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=60) && (i<=90))) { 
      sprintf(temp,":So%d#",i);
      Ser.print(temp);
    }
  }
  v=server.arg("hl");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=-30) && (i<=30))) { 
      sprintf(temp,":Sh%d#",i);
      Ser.print(temp);
    }
  }

  // Meridian Limits
  v=server.arg("el");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=-45) && (i<=45))) { 
      i=round((i*60.0)/15.0);
      sprintf(temp,":SXE9,%d#",i);
      Ser.print(temp);
    }
  }
  v=server.arg("wl");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=-45) && (i<=45))) { 
      i=round((i*60.0)/15.0);
      sprintf(temp,":SXEA,%d#",i);
      Ser.print(temp);
    }
  }

  // Backlash Limits
  v=server.arg("b1");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=0) && (i<=999))) { 
      sprintf(temp,":$BR%d#",i);
      Ser.print(temp);
    }
  }
  v=server.arg("b2");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=0) && (i<=999))) { 
      sprintf(temp,":$BD%d#",i);
      Ser.print(temp);
    }
  }

  // Location
  int long_deg=-999;
  v=server.arg("g1");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=-180) && (i<=180))) { long_deg=i; }
  }
  v=server.arg("g2");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=0) && (i<=60))) { 
      if ((long_deg>=-180) && (long_deg<=180)) {
        sprintf(temp,":Sg%+04d*%02d#",long_deg,i);
        Ser.print(temp);
      }
    }
  }
  int lat_deg=-999;
  v=server.arg("t1");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=-90) && (i<=90))) { lat_deg=i; }
  }
  v=server.arg("t2");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=0) && (i<=60))) {
      if ((lat_deg>=-90) && (lat_deg<=90)) {
        sprintf(temp,":St%+03d*%02d#",lat_deg,i);
        Ser.print(temp);
      }
    }
  }
  int ut_hrs=-999;
  v=server.arg("u1");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i>=-13) && (i<=13))) { ut_hrs=i; }
  }
  v=server.arg("u2");
  if (v!="") {
    if ( (atoi2((char*)v.c_str(),&i)) && ((i==00) || (i==30) || (i==45))) {
      if ((ut_hrs>=-13) && (ut_hrs<=13)) {
        sprintf(temp,":SG%+03d:%02d#",ut_hrs,i);
        Ser.print(temp);
      }
    }
  }

  // clear any possible response
  temp[Ser.readBytesUntil('#',temp,20)]=0;
}

