#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
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
"<div class='bct' align='left'>Tracking:</div>"
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
"<div class='bct' align='left'>Align:</div>"
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
"<div class='bct' align='left'>Park & Home:</div>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('pk')\">Park</button>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('pr')\">Sync@Park</button>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('ps')\">Set Park</button><br / >"
"<button type = 'button' class = 'bb' onpointerdown = \"g('qh')\">Go Home</button>"
"<button type = 'button' class = 'bb' onpointerdown = \"g('qr')\">Sync@home</button>"
"</div><br class='clear' />\r\n";

const char html_controlGuide[] PROGMEM =
"<div class='b1' style='width: 27em'>"
"<div class='bct' align='left'>Recenter:</div>"
"<button class='gb' type='button' onpointerdown=\"g('n1')\" onpointerup=\"g('n0')\">" BUTTON_N "</button><br />"
"<button class='gb' type='button' onpointerdown=\"g('e1')\" onpointerup=\"g('e0')\">" BUTTON_E "</button>"
"<button class='gb' type='button' onpointerdown=\"g('q1')\">" BUTTON_Stop "</button>"
"<button class='gb' type='button' onpointerdown=\"g('w1')\" onpointerup=\"g('w0')\">" BUTTON_W "</button><br />"
"<button class='gb' type='button' onpointerdown=\"g('s1')\" onpointerup=\"g('s0')\">" BUTTON_S "</button><br /><br />"
"<button class='bbh' type='button' onpointerdown=\"g('R0')\">Guide</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R1')\">Slow</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R2')\">Medium</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R3')\">Fast</button>"
"<button class='bbh' type='button' onpointerdown=\"g('R4')\">Max</button>"
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


const char html_controlEnd[] =
"<br />\r\n";


void TeenAstroWifi::handleControl()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp1[80] = "";
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

  if (ta_MountStatus.hasFocuser())
  {
    data += FPSTR(html_controlFocus1);
    data += "<div class='bct' style='float: left;'>Focuser:</div><div style='float: right; text-align: right;' id='focuserpos'>?</div><br />";
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


#ifdef ALIGN_ON
  // Get the align mode --------------------------------------
  data += FPSTR(html_controlAlign1);
  if (mountStatus.alignMaxStars() <= 3)
  {
    for (int i = 1; i <= mountStatus.alignMaxStars(); i++)
    {
      char temp2[120] = ""; sprintf_P(temp2, html_controlAlign2, i, i, SIDEREAL_CH); data += temp2;
    }
  }
  else
  {
    char temp2[120] = "";
    sprintf_P(temp2, html_controlAlign2, 1, 1, SIDEREAL_CH); data += temp2;
    sprintf_P(temp2, html_controlAlign2, 4, 4, SIDEREAL_CH); data += temp2;
    sprintf_P(temp2, html_controlAlign2, 6, 6, SIDEREAL_CH); data += temp2;
  }
  data += FPSTR(html_controlAlign3);
#endif


  // Tracking ------------------------------------------------
  data += FPSTR(html_controlTrack10);

  //Goto Sync
  data += FPSTR(html_controlParkHome);

  data += html_controlEnd;
  data += "</div></body></html>";
  //server.send(200, "text/html",data);
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::guideAjax()
{
  processControlGet();
  server.send(200, "text/html", "");
}


void TeenAstroWifi::controlAjax()
{
  String data = "";
  char temp[40] = "";
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

  server.send(200, "text/plain", data);
}

int get_temp_month;
int get_temp_day;
int get_temp_year;
int get_temp_hour;
int get_temp_minute;
int get_temp_second;

void TeenAstroWifi::processControlGet()
{
  String v;
  int i;
  char temp[20] = "";
  // Align
#ifdef ALIGN_ON
  v = server.arg("al");
  if (v != "")
  {
    if (v == "1") SetLX200(":A1#");
    if (v == "2") SetLX200(":A2#");
    if (v == "3") SetLX200(":A3#");
    if (v == "4") SetLX200(":A4#");
    if (v == "5") SetLX200(":A5#");
    if (v == "6") SetLX200(":A6#");
    if (v == "7") SetLX200(":A7#");
    if (v == "8") SetLX200(":A8#");
    if (v == "9") SetLX200(":A9#");
    if (v == "n") SetLX200(":A+#");
    if (v == "q") SetLX200(":Q#");
  }
#endif

  // Set DATE/TIME
  v = server.arg("dm");
  if (v != "")
  {
    if ((atoi2((char *)v.c_str(), &i)) && ((i >= 0) && (i <= 11)))
    {
      get_temp_month = i + 1;
    }
  }
  v = server.arg("dd");
  if (v != "")
  {
    if ((atoi2((char *)v.c_str(), &i)) && ((i >= 1) && (i <= 31)))
    {
      get_temp_day = i;
    }
  }
  v = server.arg("dy");
  if (v != "")
  {
    if ((atoi2((char *)v.c_str(), &i)) && ((i >= 2016) && (i <= 9999)))
    {
      get_temp_year = i - 2000;
      char temp[10];
      sprintf(temp, ":SXT1%02d/%02d/%02d#", get_temp_month, get_temp_day, get_temp_year);
      SetLX200(temp);
    }
  }
  v = server.arg("th");
  if (v != "")
  {
    if ((atoi2((char *)v.c_str(), &i)) && ((i >= 0) && (i <= 23)))
    {
      get_temp_hour = i;
    }
  }
  v = server.arg("tm");
  if (v != "")
  {
    if ((atoi2((char *)v.c_str(), &i)) && ((i >= 0) && (i <= 59)))
    {
      get_temp_minute = i;
    }
  }
  v = server.arg("ts");
  if (v != "")
  {
    if ((atoi2((char *)v.c_str(), &i)) && ((i >= 0) && (i <= 59)))
    {
      get_temp_second = i;
      char temp[10];
      sprintf(temp, ":SXT0%02d:%02d:%02d#", get_temp_hour, get_temp_minute, get_temp_second);
      SetLX200(temp);
    }
  }

  v = server.arg("dr");
  if (v != "")
  {
    // Tracking control
    if (v == "on") SetLX200(":Te#");
    else if (v == "off") SetLX200(":Td#");
    else if (v == "f") SetLX200(":T+#"); // 0.02hz faster
    else if (v == "-") SetLX200(":T-#"); // 0.02hz slower
    else if (v == "r") SetLX200(":TR#"); // reset

    // Tracking control
    else if (v == "Ts") SetLX200(":TQ#"); // sidereal
    else if (v == "Tl") SetLX200(":TL#"); // lunar
    else if (v == "Th") SetLX200(":TS#"); // solar

    // quick
    else if (v == "qc") SetLX200(":MF#");  // meridian flip
    else if (v == "qr") SetLX200(":hF#");  // home, reset
    else if (v == "qh") SetLX200(":hC#");  // home, goto
    else if (v == "pr") SetLX200(":hO#");  // park, reset
    else if (v == "ps") SetLX200(":hQ#");  // set park
    else if (v == "pk") SetLX200(":hP#");  // park
    else if (v == "pu") SetLX200(":hR#");  // un-park


// GUIDE control direction
    else if (v == "n1") SetLX200(":Mn#");
    else if (v == "s1") SetLX200(":Ms#");
    else if (v == "e1") SetLX200(":Me#");
    else if (v == "w1") SetLX200(":Mw#");
    else if (v == "q1") SetLX200(":Q#");

    else if (v == "n0") SetLX200(":Qn#");
    else if (v == "s0") SetLX200(":Qs#");
    else if (v == "e0") SetLX200(":Qe#");
    else if (v == "w0") SetLX200(":Qw#");

    // GUIDE control rate
    else if (v == "R0") SetLX200(":R0#");
    else if (v == "R1") SetLX200(":R1#");
    else if (v == "R2") SetLX200(":R2#");
    else if (v == "R3") SetLX200(":R3#");
    else if (v == "R4") SetLX200(":R4#");

    // Focuser
    else if (v == "Fz") SetLX200(":FP#");
    else if (v == "Fh") SetLX200(":FS,0#");
    else if (v == "Fi") SetLX200(":F-#");
    else if (v == "Fo") SetLX200(":F+#");
    else if (v == "Fq") SetLX200(":FQ#");

    // Rotate/De-Rotate
    else if (v == "b2") SetLX200(":r3#:r<#");
    else if (v == "b1") SetLX200(":r1#:r<#");
    else if (v == "f1") SetLX200(":r1#:r>#");
    else if (v == "f2") SetLX200(":r3#:r>#");
    else if (v == "ho") SetLX200(":rC#");
    else if (v == "re") SetLX200(":rF#");
    else if (v == "d0") SetLX200(":r-#");
    else if (v == "d1") SetLX200(":r+#");
    else if (v == "dr") SetLX200(":rR#");
    else if (v == "dp") SetLX200(":rP#");
  }
}
