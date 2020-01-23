#include <TeenAstroLX200io.h>
#include "WifiBluetooth.h"
#include "config.h"
#include "Ajax.h"
// -----------------------------------------------------------------------------------
// Telescope control related functions

#define ARROW_DR "&#x27A5;"
#define ARROW_UR "&#x27A6;"
#define ARROW_R2 "&#x27A4;"
#define CAUTION_CH "&#9888;"
#define CLOCK_CH "&#x1F565;"
#define PLUS_CH "+"
#define MINUS_CH "-"
#define ARROW_LL "&lt;&lt;"
#define ARROW_L "&lt;"
#define ARROW_R "&gt;"
#define ARROW_RR "&gt;&gt;"
#define ARROW_DD "&lt;&lt;"
#define ARROW_D "&lt;"
#define ARROW_U "&gt;"
#define ARROW_UU "&gt;&gt;"
#define SIDEREAL_CH "&#9733;"
#define LUNAR_CH "&#9790;"
#define SOLAR_CH "&#9737;"
#define BUTTON_N "N"
#define BUTTON_S "S"
#define BUTTON_E "E"
#define BUTTON_W "W"
#define BUTTON_Stop "stop"
#define BUTTON_SYNC "@"

const char  html_controlScript1[] = "<script>\n"
"function s(key,v1) {\n"
"var xhttp = new XMLHttpRequest();\n"
"xhttp.open('GET', 'guide.txt?'+key+'='+v1+'&x='+new Date().getTime(), true);\n"
"xhttp.send();\n"
"}\n"
"function g(v1){s('dr',v1);}\n"
"function gf(v1){s('dr',v1);autoFastRun();}\n"
"function sf(key,v1){s(key,v1);autoFastRun();}\n"
"</script>\n";

const char html_controlScript2[] PROGMEM =
"<script>\r\n"
"function SetDateTime() {"
#ifdef TIMEZONE_ON
"var d1 = new Date();"
"var jan = new Date(d1.getFullYear(), 0, 1);"
"var d = new Date(d1.getTime()-(jan.getTimezoneOffset()-d1.getTimezoneOffset())*60*1000);";
const char html_controlScript3[] =
"document.getElementById('dd').value = d.getDate();"
"document.getElementById('dm').value = d.getMonth();"
"document.getElementById('dy').value = d.getFullYear();";
const char html_controlScript4[] =
"document.getElementById('th').value = d.getHours();"
"document.getElementById('tm').value = d.getMinutes();"
"document.getElementById('ts').value = d.getSeconds();"
#else
"var d1 = new Date();";
const char html_controlScript3[] PROGMEM =
"document.getElementById('dd').value = d1.getUTCDate();"
"document.getElementById('dm').value = d1.getUTCMonth();"
"document.getElementById('dy').value = d1.getUTCFullYear();";
const char html_controlScript4[] PROGMEM =
"document.getElementById('th').value = d1.getUTCHours();"
"document.getElementById('tm').value = d1.getUTCMinutes();"
"document.getElementById('ts').value = d1.getUTCSeconds();"
#endif
"}\r\n"
"</script>\r\n";

const char html_controlQuick0[] PROGMEM =
"<div style='text-align: center; width: 30em'>"
"<form style='display: inline;' method='get' action='/control.htm'>";
const char html_controlQuick1[] PROGMEM =
"<button name='qb' class='bb' value='st' type='submit' onpointerdown='SetDateTime();'>" CLOCK_CH "</button>"
"&nbsp;&nbsp;";
const char html_controlQuick1a[] PROGMEM =
"<input id='dm' type='hidden' name='dm'><input id='dd' type='hidden' name='dd'><input id='dy' type='hidden' name='dy'>"
"<input id='th' type='hidden' name='th'><input id='tm' type='hidden' name='tm'><input id='ts' type='hidden' name='ts'>";

const char html_controlQuick2[] PROGMEM =
"&nbsp;&nbsp;"
"<button type='button' class='bb' onpointerdown=\"g('pu')\">Unpark</button>";

const char html_controlQuick3[]  PROGMEM =
"</div><br class='clear' />\r\n";

const char html_controlTrack1[]  PROGMEM =
"<div class='b1' style='width: 27em'>"
"<div align='left'>Tracking:</div>"
"<button type='button' class='bbh' onpointerdown=\"g('Ts')\" type='submit'>" SIDEREAL_CH "</button>";
const char html_controlTrack2[]  PROGMEM =
"<button type='button' class='bbh' onpointerdown=\"g('Tl')\" type='submit'>" LUNAR_CH "</button>";
const char html_controlTrack3[]  PROGMEM =
"<button type='button' class='bbh' onpointerdown=\"g('Th')\" type='submit'>" SOLAR_CH "</button>";
const char html_controlTrack4[] PROGMEM =
"<button type='button' class='bbh' onpointerdown=\"g('on')\" type='submit'>On</button>"
"<button type='button' class='bbh' onpointerdown=\"g('off')\" type='submit'>Off</button><br/></div>";
//const char html_controlTrack5[] PROGMEM =
//"<button type='button' class='bbh' style='width: 2.6em' onpointerdown=\"g('-')\" type='submit'>" MINUS_CH "</button>"
//"<button type='button' class='bbh' style='width: 2.6em' onpointerdown=\"g('f')\" type='submit'>" PLUS_CH " </button>"
//"<button type='button' class='bbh' onpointerdown=\"g('r')\" type='submit'>Reset</button>""</div>";

#ifdef ALIGN_ON
const char html_controlAlign1[]  PROGMEM =
"<div class='b1' style='width: 16.2em'>"
"<div align='left'>Align:</div>"
"<form method='get' action='/control.htm'>";
const char html_controlAlign2[]  PROGMEM =
"<button class='bbh' type='submit' name=\"al\" value='%d'>%d%s</button>";
const char html_controlAlign3[]  PROGMEM =
"&nbsp;&nbsp;&nbsp;<button class='bbh' type='submit' name=\"al\" value=\"n\">Accept</button></form>";
const char html_controlTrack4[]  PROGMEM =
"</div><br class='clear' />\r\n";
#else 
const char html_controlTrack10[]  PROGMEM = "<br class='clear' />\r\n";
#endif
const char html_controlParkHome[] PROGMEM =
"<div class='b1' style='width: 27em'>"
"<div align='left'>Park & Home:</div>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('pk')\">Park</button>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('pr')\">Sync@Park</button>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('ps')\">Set Park</button><br / >"
"<button type = 'button' class = 'bb' onpointerdown = \"g('qh')\">Go Home</button>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('qr')\">Sync@home</button>"
"</div><br class='clear' />\r\n";

const char html_controlGuide[] PROGMEM =
"<div class='b1' style='width: 27em'>"
"<div align='left'>Recenter:</div>"
"<button class='gb' type='button' onpointerdown=\"g('n1')\" onpointerup=\"g('n0')\">" BUTTON_N "</button><br />"
"<button class='gb' type='button' onpointerdown=\"g('e1')\" onpointerup=\"g('e0')\">" BUTTON_E "</button>"
"<button class='gb' type='button' onpointerdown=\"g('q1')\">" BUTTON_Stop "</button>"
"<button class='gb' type='button' onpointerdown=\"g('w1')\" onpointerup=\"g('w0')\">" BUTTON_W "</button><br />"
"<button class='gb' type='button' onpointerdown=\"g('s1')\" onpointerup=\"g('s0')\">" BUTTON_S "</button><br /><br />"
"<button class='bbh' type='button' onpointerdown=\"g('R2')\">1x</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R4')\">4x</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R5')\">16x</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R7')\">64x</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R9')\">VMax</button>"
"</div><br class='clear' />\r\n";

const char html_controlFocus1[] PROGMEM =
"<div class='b1' style='width: 27em'>";
const char html_controlFocus3[] PROGMEM =
"<button class='bb' type='button' onpointerdown=\"gf('Fz')\" >Park</button>"
"<button class='bb' type='button' onpointerdown=\"gf('Fh')\" >Set 0</button>&nbsp;&nbsp;&nbsp;&nbsp;";
const char html_controlFocus4[] PROGMEM =
"<button class='bbh' type='button' onpointerdown=\"gf('Fi')\" onpointerup=\"g('Fq')\" >" MINUS_CH "</button>";
const char html_controlFocus5[] PROGMEM =
"<button class='bbh' type='button' onpointerdown=\"gf('Fo')\" onpointerup=\"g('Fq')\" >" PLUS_CH "</button>";

const char html_controlFocus6[] PROGMEM =
"</div><br class='clear' />\r\n";

#ifdef ROTATOR_ON
const char html_controlRotate0[] PROGMEM =
"<div class='b1' style='width: 27em'>";
const char html_controlRotate1[] PROGMEM =
"<button class='bbh' type='button' style='height: 2.1em' onpointerdown=\"gf('re')\" >Reset</button>"
"<button class='bbh' type='button' style='height: 2.1em' onpointerdown=\"gf('ho')\" >Go Home</button>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
const char html_controlRotate2[] PROGMEM =
"<button class='bbh' type='button' style='height: 2.1em' onpointerdown=\"gf('b2')\" >" ARROW_LL "</button>"
"<button class='bbh' type='button' style='width: 2em' onpointerdown=\"gf('b1')\" >" ARROW_L "</button>";
const char html_controlRotate3[] PROGMEM =
"<button class='bbh' type='button' style='width: 2em' onpointerdown=\"gf('f1')\" >" ARROW_R "</button>"
"<button class='bbh' type='button' style='height: 2.1em' onpointerdown=\"gf('f2')\" >" ARROW_RR "</button><br />";
const char html_controlDeRotate1[] PROGMEM =
"<button type='button' onpointerdown=\"gf('d1')\" >De-Rotate On</button>&nbsp;&nbsp;&nbsp;"
"<button type='button' onpointerdown=\"gf('dr')\" >Rev</button>";
const char html_controlDeRotate2[] PROGMEM =
"<button type='button' onpointerdown=\"gf('dp')\" >P</button>&nbsp;&nbsp;&nbsp;"
"<button type='button' onpointerdown=\"gf('d0')\" >De-Rotate Off</button>";
const char html_controlRotate4[] PROGMEM =
"</div><br class='clear' />\r\n";
#endif


const char html_controlEnd[] =
"<br />\r\n";


void wifibluetooth::handleControl() {
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp1[80]="";
  String data;
  processControlGet();
  preparePage(data, 2);
  sendHtml(data);
  // guide (etc) script
  data += FPSTR(html_controlScript1);
  // clock script
  data += FPSTR(html_controlScript2);
  data += FPSTR(html_controlScript3);
  data += FPSTR(html_controlScript4);
  sendHtml(data);
  // active ajax page is: controlAjax();
  data += "<script>var ajaxPage='control.txt';</script>\n";
  data += FPSTR(html_ajax_active);
  sendHtml(data);


  data += FPSTR(html_controlQuick0);
  sendHtml(data);
  // Quick controls ------------------------------------------
  if (!ta_MountStatus.Parking())
  {
    if (ta_MountStatus.Parked() || ta_MountStatus.atHome())
    {
      data += FPSTR(html_controlQuick1);
      data += FPSTR(html_controlQuick1a);
      sendHtml(data);
      if (ta_MountStatus.Parked())
      {
        data += FPSTR(html_controlQuick2);
        data += "</form>";
        data += FPSTR(html_controlQuick3);
        data += html_controlEnd;
        data += "</div></body></html>";

        sendHtml(data);
        sendHtmlDone(data);

        return;
      }
      data += "</form>";
    }
  }
  data += FPSTR(html_controlQuick3);
  sendHtml(data);



  // Guiding -------------------------------------------------
  data += FPSTR(html_controlGuide);


  // Focusing ------------------------------------------------
  boolean Focuser1; if (GetLX200(":FV#", temp1, sizeof(temp1)) == LX200VALUEGET) Focuser1 = true; else Focuser1 = false;
  //Focuser1 = true;
  boolean Focuser2 = false;
  /* boolean Focuser2; if (sendCommand(":fA#",temp1,R_BOOL)) Focuser2=true; else Focuser2=false;*/
  if (Focuser1) {
    data += FPSTR(html_controlFocus1);
    data += "<div style='float: left;'>Focuser:</div><div style='float: right; text-align: right;' id='focuserpos'>?</div><br />";
    data += FPSTR(html_controlFocus3);
    data += FPSTR(html_controlFocus4);
    data += FPSTR(html_controlFocus5);
    data += FPSTR(html_controlFocus6);
    sendHtml(data);

  }

  // Tracking control ----------------------------------------
  data += FPSTR(html_controlTrack1);
  data += FPSTR(html_controlTrack2);
  data += FPSTR(html_controlTrack3);
  data += FPSTR(html_controlTrack4);
  sendHtml(data);
#ifdef OETHS
  client->print(data); data="";
#endif

#ifdef ALIGN_ON
  // Get the align mode --------------------------------------
  data += FPSTR(html_controlAlign1);
  if (mountStatus.alignMaxStars()<=3) {
    for (int i=1; i<=mountStatus.alignMaxStars(); i++) { char temp2[120]=""; sprintf_P(temp2,html_controlAlign2,i,i,SIDEREAL_CH); data+=temp2; }
  } else {
    char temp2[120]="";
    sprintf_P(temp2,html_controlAlign2,1,1,SIDEREAL_CH); data+=temp2;
    sprintf_P(temp2,html_controlAlign2,4,4,SIDEREAL_CH); data+=temp2;
    sprintf_P(temp2,html_controlAlign2,6,6,SIDEREAL_CH); data+=temp2;
  }
  data += FPSTR(html_controlAlign3);
#endif
#ifdef OETHS
  client->print(data); data="";
#endif

  // Tracking ------------------------------------------------
  data += FPSTR(html_controlTrack10);

  //Goto Sync
  data += FPSTR(html_controlParkHome);

  // Rotate/De-Rotate ----------------------------------------
#ifdef ROTATOR_ON
  boolean Rotate=false;
  boolean DeRotate=false;
  if (sendCommand(":GX98#",temp1)) {
    if (temp1[0]=='R') Rotate=true;
    if (temp1[0]=='D') { Rotate=true; DeRotate=true; }
  }
  if (Rotate) {
    data += FPSTR(html_controlRotate0);
    data += "<div style='float: left;'>Rotator:</div><div style='float: right; text-align: right;' id='rotatorpos'>?</div><br />";
    data += FPSTR(html_controlRotate1);
    data += FPSTR(html_controlRotate2);
    data += FPSTR(html_controlRotate3);
  }
  if (DeRotate) {
    data += FPSTR(html_controlDeRotate1);
    data += FPSTR(html_controlDeRotate2);
  }
  if (Rotate) {
    data += FPSTR(html_controlRotate4);

#ifdef OETHS
    client->print(data); data="";
#endif
  }
#endif
  // Aux -----------------------------------------------------
  #if defined(SW0) || defined(SW1) || defined(SW2) || defined(SW3) || defined(SW4) || defined(SW5) || defined(SW6) || defined(SW7) || defined(SW8) || defined(SW9) || defined(SW10) || defined(SW11) || defined(SW12) || defined(SW13) || defined(SW14) || defined(SW15) || defined(AN3) || defined(AN4) || defined(AN5) || defined(AN6) || defined(AN7) || defined(AN8)
    data += html_controlAuxB;
    // Digital Control
    int c=0;
    #ifdef SW0
    data += html_controlSwitch0; c++;
    #endif
    #ifdef SW1
    data += html_controlSwitch1; c++;
    #endif
    #ifdef SW2
    data += html_controlSwitch2; c++;
    #endif
    #ifdef SW3
    data += html_controlSwitch3; c++;
    #endif
    #ifdef SW4
    data += html_controlSwitch4; c++;
    #endif
    #ifdef SW5
    data += html_controlSwitch5; c++;
    #endif
    #ifdef SW6
    data += html_controlSwitch6; c++;
    #endif
    #ifdef SW7
    data += html_controlSwitch7; c++;
    #endif
    #ifdef SW8
    data += html_controlSwitch8; c++;
    #endif
    #ifdef SW9
    data += html_controlSwitch9; c++;
    #endif
    #ifdef SW10
    data += html_controlSwitch10; c++;
    #endif
    #ifdef SW11
    data += html_controlSwitch11; c++;
    #endif
    #ifdef SW12
    data += html_controlSwitch12; c++;
    #endif
    #ifdef SW13
    data += html_controlSwitch13; c++;
    #endif
    #ifdef SW14
    data += html_controlSwitch14; c++;
    #endif
    #ifdef SW15
    data += html_controlSwitch15; c++;
    #endif
    if (c>0) data+="<br />";

#ifdef OETHS
    client->print(data); data="";
#endif

    // Analog Control
    #ifdef AN3
    if (sendCommand(":GXG3#",temp1)) { data += html_controlAnalog3A; data += temp1; data += html_controlAnalog3B; data += temp1; data += html_controlAnalog3C; }
    #endif
    #ifdef AN4
    if (sendCommand(":GXG4#",temp1)) { data += html_controlAnalog4A; data += temp1; data += html_controlAnalog4B; data += temp1; data += html_controlAnalog4C; }
    #endif
    #ifdef AN5
    if (sendCommand(":GXG5#",temp1)) { data += html_controlAnalog5A; data += temp1; data += html_controlAnalog5B; data += temp1; data += html_controlAnalog5C; }
    #endif
    #ifdef AN6
    if (sendCommand(":GXG6#",temp1)) { data += html_controlAnalog6A; data += temp1; data += html_controlAnalog6B; data += temp1; data += html_controlAnalog6C; }
    #endif
    #ifdef AN7
    if (sendCommand(":GXG7#",temp1)) { data += html_controlAnalog7A; data += temp1; data += html_controlAnalog7B; data += temp1; data += html_controlAnalog7C; }
    #endif
    #ifdef AN8
    if (sendCommand(":GXG8#",temp1)) { data += html_controlAnalog8A; data += temp1; data += html_controlAnalog8B; data += temp1; data += html_controlAnalog8C; }
    #endif

    data += html_controlAuxE;
  #endif

  data += html_controlEnd;

  data += "</div></body></html>";

#ifdef OETHS
  client->print(data);
#else
  //server.send(200, "text/html",data);
  sendHtml(data);
  sendHtmlDone(data);
#endif
}

#ifdef OETHS
void guideAjax(EthernetClient *client) {
#else
void wifibluetooth::guideAjax() {
#endif
  processControlGet();
#ifdef OETHS
  client->print("");
#else
  server.send(200, "text/html","");
#endif
}

#ifdef OETHS
void wifibluetooth::controlAjax(EthernetClient *client) {
#else
void wifibluetooth::controlAjax() {
#endif
  String data="";
  char temp[40]="";
  data += "focuserpos|";
  ta_MountStatus.updateFocuser();
  strcpy(temp, ta_MountStatus.getFocuser());
  temp[6] = 0;
  data += &temp[1];
  data += " steps, ";
  temp[10] = 0;
  temp[16] = 0;
  data += &temp[11];
  data += "&deg C";
#ifdef ROTATOR_ON
  data += "rotatorpos|";
  if (sendCommand(":rG#",temp)) { temp[9]=temp[5]; temp[10]=temp[6]; temp[11]=0; temp[4]='&'; temp[5]='d'; temp[6]='e'; temp[7]='g'; temp[8]=';'; data += temp; data += "&#39;\n"; } else { data += "?\n"; }
#endif

  #ifdef AN3
    data += "an3v|"; if (sendCommand(":GXG3#",temp)) { data += temp; data += "\n"; } else { data += "?\n"; }
  #endif
  #ifdef AN4
    data += "an4v|"; if (sendCommand(":GXG4#",temp)) { data += temp; data += "\n"; } else { data += "?\n"; }
  #endif
  #ifdef AN5
    data += "an5v|"; if (sendCommand(":GXG5#",temp)) { data += temp; data += "\n"; } else { data += "?\n"; }
  #endif
  #ifdef AN6
    data += "an6v|"; if (sendCommand(":GXG6#",temp)) { data += temp; data += "\n"; } else { data += "?\n"; }
  #endif
  #ifdef AN7
    data += "an7v|"; if (sendCommand(":GXG7#",temp)) { data += temp; data += "\n"; } else { data += "?\n"; }
  #endif
  #ifdef AN8
    data += "an8v|"; if (sendCommand(":GXG8#",temp)) { data += temp; data += "\n"; } else { data += "?\n"; }
  #endif

#ifdef OETHS
  client->print(data);
#else
  server.send(200, "text/plain",data);

#endif
}

int get_temp_month;
int get_temp_day;
int get_temp_year;
int get_temp_hour;
int get_temp_minute;
int get_temp_second;

void wifibluetooth::processControlGet() {
  String v;
  int i;
  char temp[20]="";
  // Align
#ifdef ALIGN_ON
  v=server.arg("al");
  if (v!="") {
    if (v=="1") SetLX200(":A1#");
    if (v=="2") SetLX200(":A2#");
    if (v=="3") SetLX200(":A3#");
    if (v=="4") SetLX200(":A4#");
    if (v=="5") SetLX200(":A5#");
    if (v=="6") SetLX200(":A6#");
    if (v=="7") SetLX200(":A7#");
    if (v=="8") SetLX200(":A8#");
    if (v=="9") SetLX200(":A9#");
    if (v=="n") SetLX200(":A+#");
    if (v=="q") SetLX200(":Q#");
  }
#endif

  // Set DATE/TIME
  v=server.arg("dm");
  if (v!="") {
    if ( (atoi2((char *)v.c_str(),&i)) && ((i>=0) && (i<=11))) { get_temp_month=i+1; }
  }
  v=server.arg("dd");
  if (v!="") {
    if ( (atoi2((char *)v.c_str(),&i)) && ((i>=1) && (i<=31))) { get_temp_day=i; }
  }
  v=server.arg("dy");
  if (v!="") {
    if ( (atoi2((char *)v.c_str(),&i)) && ((i>=2016) && (i<=9999))) {
      get_temp_year=i-2000;
      char temp[10];
      sprintf(temp,":SX81%02d/%02d/%02d#",get_temp_month,get_temp_day,get_temp_year);
      SetLX200(temp);
    }
  }
  v=server.arg("th");
  if (v!="") {
    if ( (atoi2((char *)v.c_str(),&i)) && ((i>=0) && (i<=23))) { get_temp_hour=i; }
  }
  v=server.arg("tm");
  if (v!="") {
    if ( (atoi2((char *)v.c_str(),&i)) && ((i>=0) && (i<=59))) { get_temp_minute=i; }
  }
  v=server.arg("ts");
  if (v!="") {
    if ( (atoi2((char *)v.c_str(),&i)) && ((i>=0) && (i<=59))) {
      get_temp_second=i;
      char temp[10];
      sprintf(temp,":SX80%02d:%02d:%02d#",get_temp_hour,get_temp_minute,get_temp_second);
      SetLX200(temp);
    }
  }

  v=server.arg("dr");
  if (v!="") {
    // Tracking control


    if (v == "on") SetLX200(":Te#");
    if (v == "off") SetLX200(":Td#");
    if (v == "f") SetLX200(":T+#"); // 0.02hz faster
    if (v == "-") SetLX200(":T-#"); // 0.02hz slower
    if (v == "r") SetLX200(":TR#"); // reset
    
    // Tracking control
    if (v=="Ts") SetLX200(":TQ#"); // sidereal
    if (v=="Tl") SetLX200(":TL#"); // lunar
    if (v=="Th") SetLX200(":TS#"); // solar

    // quick
    if (v=="qc") { SetLX200(":SX99,1#");} // meridian flip, pause->continue
    if (v=="qr") { SetLX200(":hF#");}     // home, reset
    if (v=="qh") { SetLX200(":hC#");}     // home, goto
    if (v=="pr") { SetLX200(":hO#");}     // park, reset
    if (v=="ps") { SetLX200(":hQ#");}     // set park
    if (v=="pk") { SetLX200(":hP#");}     // park
    if (v=="pu") { SetLX200(":hR#");}     // un-park


    // GUIDE control direction
    if (v=="n1") SetLX200(":Mn#");
    if (v=="s1") SetLX200(":Ms#");
    if (v=="e1") SetLX200(":Me#");
    if (v=="w1") SetLX200(":Mw#");
    if (v=="q1") SetLX200(":Q#");

    if (v=="n0") SetLX200(":Qn#");
    if (v=="s0") SetLX200(":Qs#");
    if (v=="e0") SetLX200(":Qe#");
    if (v=="w0") SetLX200(":Qw#");

    // GUIDE control rate
    if (v=="R0") SetLX200(":R0#");
    if (v=="R1") SetLX200(":R1#");
    if (v=="R2") SetLX200(":R2#");
    if (v=="R3") SetLX200(":R3#");
    if (v=="R4") SetLX200(":R4#");
    if (v=="R5") SetLX200(":R5#");
    if (v=="R6") SetLX200(":R6#");
    if (v=="R7") SetLX200(":R7#");
    if (v=="R8") SetLX200(":R8#");
    if (v=="R9") SetLX200(":R9#");

    // Focuser
    if (v=="Fz") SetLX200(":FP#");
    if (v=="Fh") SetLX200(":FS0#");
    if (v=="Fi") SetLX200(":F-#");
    if (v=="Fo") SetLX200(":F+#");
    if (v=="Fq") SetLX200(":FQ#");

    // Rotate/De-Rotate
    if (v=="b2") SetLX200(":r3#:r<#");
    if (v=="b1") SetLX200(":r1#:r<#");
    if (v=="f1") SetLX200(":r1#:r>#");
    if (v=="f2") SetLX200(":r3#:r>#");
    if (v=="ho") SetLX200(":rC#");
    if (v=="re") SetLX200(":rF#");
    if (v=="d0") SetLX200(":r-#");
    if (v=="d1") SetLX200(":r+#");
    if (v=="dr") SetLX200(":rR#");
    if (v=="dp") SetLX200(":rP#");
    Ser.flush();
  }

  // General purpose switches
  #ifdef SW0
  v=server.arg("sw0"); if (v!="") { SetLX200(":SXG0,"+v+"#");}
  #endif
  #ifdef SW1
  v=server.arg("sw1"); if (v!="") { SetLX200(":SXG1,"+v+"#");}
  #endif
  #ifdef SW2
  v=server.arg("sw2"); if (v!="") { SetLX200(":SXG2,"+v+"#");}
  #endif
  #ifdef SW3
  v=server.arg("sw3"); if (v!="") { SetLX200(":SXG3,"+v+"#");}
  #endif
  #ifdef SW4
  v=server.arg("sw4"); if (v!="") { SetLX200(":SXG4,"+v+"#");}
  #endif
  #ifdef SW5
  v=server.arg("sw5"); if (v!="") { SetLX200(":SXG5,"+v+"#");}
  #endif
  #ifdef SW6
  v=server.arg("sw6"); if (v!="") { SetLX200(":SXG6,"+v+"#");}
  #endif
  #ifdef SW7
  v=server.arg("sw7"); if (v!="") { SetLX200(":SXG7,"+v+"#");}
  #endif
  #ifdef SW8
  v=server.arg("sw8"); if (v!="") { SetLX200(":SXG8,"+v+"#");}
  #endif
  #ifdef SW9
  v=server.arg("sw9"); if (v!="") { SetLX200(":SXG9,"+v+"#");}
  #endif
  #ifdef SW10
  v=server.arg("swA"); if (v!="") { SetLX200(":SXGA,"+v+"#");}
  #endif
  #ifdef SW11
  v=server.arg("swB"); if (v!="") { SetLX200(":SXGB,"+v+"#");}
  #endif
  #ifdef SW12
  v=server.arg("swC"); if (v!="") { SetLX200(":SXGC,"+v+"#");}
  #endif
  #ifdef SW13
  v=server.arg("swD"); if (v!="") { SetLX200(":SXGD,"+v+"#");}
  #endif
  #ifdef SW14
  v=server.arg("swE"); if (v!="") { SetLX200(":SXGE,"+v+"#");}
  #endif
  #ifdef SW15
  v=server.arg("swF"); if (v!="") { SetLX200(":SXGF,"+v+"#");}
  #endif

  // General purpose analog
  #ifdef AN3
  v=server.arg("an3"); if (v!="") { SetLX200f(":SXG3,%ld#",(v.toInt()*255L)/100L);}
  #endif
  #ifdef AN4
  v=server.arg("an4"); if (v!="") { SetLX200f(":SXG4,%ld#",(v.toInt()*255L)/100L);}
  #endif
  #ifdef AN5
  v=server.arg("an5"); if (v!="") { SetLX200f(":SXG5,%ld#",(v.toInt()*255L)/100L);}
  #endif
  #ifdef AN6
  v=server.arg("an6"); if (v!="") { SetLX200f(":SXG6,%ld#",(v.toInt()*255L)/100L);}
  #endif
  #ifdef AN7
  v=server.arg("an7"); if (v!="") { SetLX200f(":SXG7,%ld#",(v.toInt()*255L)/100L);}
  #endif
  #ifdef AN8
  v=server.arg("an8"); if (v!="") { SetLX200f(":SXG8,%ld#",(v.toInt()*255L)/100L);}
  #endif

}
