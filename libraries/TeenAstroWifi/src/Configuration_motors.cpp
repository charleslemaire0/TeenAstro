#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_motors

const char html_configRotAxis_1[] PROGMEM =
"<form action='/configuration_motors.htm'>"
"<select name='mrot%d'>";
const char html_configRotAxis_r[] PROGMEM =
"<option value ='0'>Direct</option>"
"<option selected value='1'>Reverse</option>";
const char html_configRotAxis_d[] PROGMEM =
"<option selected value ='0'>Direct</option>"
"<option value='1'>Reverse</option>";
const char html_configRotAxis_2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Rotation Axis%d)"
"</form>"
"\r\n";


const char html_configSettleTime[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mw' min='0' max='20'>"
"<button type='submit'>Upload</button>"
" (Settle Time, in seconds from 0 to 20)"
"</form>"
"\r\n";
const char html_configBlAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mbl%d' min='0' max='999'>"
"<button type='submit'>Upload</button>"
" (Backlash Axis%d, in arc-seconds from 0 to 999)"
"</form>"
"\r\n";
const char html_configBlRateAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mblr%d' min='16' max='64'>"
"<button type='submit'>Upload</button>"
" (Backlash Rate Axis%d, in sideral speed from 16 to 64)"
"</form>"
"\r\n";
const char html_configGeAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%.3f' type='number' name='mge%d' min='1' max='60000' step='.001'>"
"<button type='submit'>Upload</button>"
" (Gear Axis%d, from 1 to 60000)"
"</form>"
"\r\n";
const char html_configStAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mst%d' min='1' max='400'>"
"<button type='submit'>Upload</button>"
" (Steps per Rotation Axis%d, from 1 to 400)"
"</form>"
"\r\n";
const char html_configMuAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%d' type='number' name='mmu%d' min='8' max='256'>"
"<button type='submit'>Upload</button>"
" (Microsteps Axis%d, valid value are 8, 16, 32, 64, 128, 256)"
"</form>"
"\r\n";
const char html_configLCAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%u' type='number' name='mlc%d' min='200' max='2800' step='200'>"
"<button type='submit'>Upload</button>"
" (Low Current Axis%d, from 200mA to 2800mA)"
"</form>"
"\r\n";
const char html_configHCAxis[] PROGMEM =
"<form method='get' action='/configuration_motors.htm'>"
" <input value='%u' type='number' name='mhc%d' min='200' max='2800' step='200'>"
"<button type='submit'>Upload</button>"
" (High Current Axis%d, from 200mA to 2800mA)"
"</form>"
"\r\n";

const char html_configSilentAxis_1[] PROGMEM =
"<form action='/configuration_motors.htm'>"
"<select name='ms%d'>";
const char html_configSilentAxis_r[] PROGMEM =
"<option value ='0'>Off</option>"
"<option selected value='1'>On</option>";
const char html_configSilentAxis_d[] PROGMEM =
"<option selected value ='0'>Off</option>"
"<option value='1'>On</option>";

const char html_configSilentAxis_2[] PROGMEM =
"</select>"
"<button type='submit'>Upload</button>"
" (Silent Axis%d)"
"</form>"
"\r\n";

const char html_reboot_t[] PROGMEM =
"<br/><form method='get' action='/configuration_motors.htm'>"
"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
"<button type='submit'>Continue</button>"
"</form><br/><br/><br/><br/>"
"\r\n";



bool restartRequired_t1 = false;

void TeenAstroWifi::handleConfigurationMotors()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp2[50] = "";
  String data;

  processConfigurationMotorsGet();
  preparePage(data, ServerPage::Motors);
  sendHtml(data);
  if (restartRequired_t1)
  {
    data += FPSTR(html_reboot_t);
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    restartRequired_t1 = false;
    delay(1000);
    return;
  }

  data += "<div class='card'>";

  // Settle time
  if (s_client->getStepsPerSecond(temp2, sizeof(temp2)) == LX200_VALUEGET)
  {
    int wt = (int)strtol(temp2, NULL, 10);
    sprintf_P(temp, html_configSettleTime, wt);
    data += temp;
    sendHtml(data);
  }

  // Per-axis parameters using loops
  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    bool reverse = false;
    if (s_client->readReverse(ax, reverse) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configRotAxis_1, ax);
      data += temp;
      data += reverse ? FPSTR(html_configRotAxis_r) : FPSTR(html_configRotAxis_d);
      sprintf_P(temp, html_configRotAxis_2, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    float gear = 0;
    if (s_client->readTotGear(ax, gear) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configGeAxis, gear, ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    float step;
    if (s_client->readStepPerRot(ax, step) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configStAxis, (int)step, ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    uint8_t micro;
    if (s_client->readMicro(ax, micro) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configMuAxis, (int)pow(2., micro), ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    float backlash;
    if (s_client->readBacklash(ax, backlash) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configBlAxis, (int)backlash, ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    float blRate;
    if (s_client->readBacklashRate(ax, blRate) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configBlRateAxis, (int)blRate, ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    unsigned int lowC;
    if (s_client->readLowCurr(ax, lowC) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configLCAxis, lowC, ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    unsigned int highC;
    if (s_client->readHighCurr(ax, highC) == LX200_VALUEGET)
    {
      sprintf_P(temp, html_configHCAxis, highC, ax, ax);
      data += temp;
      sendHtml(data);
    }
  }

  // Silent mode (only for advanced drivers)
  const char* board = ta_MountStatus.getVb();
  if (board[0] - '0' > 1)
  {
    for (uint8_t ax = 1; ax <= 2; ax++)
    {
      uint8_t silent;
      if (s_client->readSilentStep(ax, silent) == LX200_VALUEGET)
      {
        sprintf_P(temp, html_configSilentAxis_1, ax);
        data += temp;
        data += silent ? FPSTR(html_configSilentAxis_r) : FPSTR(html_configSilentAxis_d);
        sprintf_P(temp, html_configSilentAxis_2, ax);
        data += temp;
        sendHtml(data);
      }
    }
  }

  data += "</div>"; // close card
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}

void TeenAstroWifi::processConfigurationMotorsGet()
{
  String v;
  int i;
  float f;

  // Settle time
  v = server.arg("mw");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 20)))
      s_client->setStepsPerSecond(i);
  }

  // Speed: MaxR, Acc, R0-R3, RD
  v = server.arg("MaxR");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4000)))
      s_client->setMaxRate(i);
  }
  v = server.arg("Acc");
  if (v != "")
  {
    if ((atof2((char*)v.c_str(), &f)) && ((f >= 0.1) && (f <= 25)))
      s_client->setAcceleration(f);
  }
  for (uint8_t idx = 0; idx <= 3; idx++)
  {
    char argName[4];
    sprintf(argName, "R%d", idx);
    v = server.arg(argName);
    if (v != "")
    {
      if (atof2((char*)v.c_str(), &f))
        s_client->setSpeedRate(idx, f);
    }
  }
  v = server.arg("RD");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 4)))
      s_client->setDeadband(i);
  }

  // Per-axis motor params
  for (uint8_t ax = 1; ax <= 2; ax++)
  {
    char argName[8];

    // Rotation
    sprintf(argName, "mrot%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) s_client->writeReverse(ax, i); }

    // Gear
    sprintf(argName, "mge%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atof2((char*)v.c_str(), &f)) && ((f >= 1) && (f <= 60000))) s_client->writeTotGear(ax, f); }

    // Steps per rotation
    sprintf(argName, "mst%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 400))) s_client->writeStepPerRot(ax, i); }

    // Microsteps
    sprintf(argName, "mmu%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 8) && (i <= 256))) s_client->writeMicro(ax, (float)((int)log2(i))); }

    // Backlash
    sprintf(argName, "mbl%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 999))) s_client->writeBacklash(ax, (float)i); }

    // Backlash rate
    sprintf(argName, "mblr%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 16) && (i <= 64))) s_client->writeBacklashRate(ax, (float)i); }

    // Low current
    sprintf(argName, "mlc%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 200) && (i <= 2800))) s_client->writeLowCurr(ax, i); }

    // High current
    sprintf(argName, "mhc%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 200) && (i <= 2800))) s_client->writeHighCurr(ax, i); }

    // Silent mode
    sprintf(argName, "ms%d", ax);
    v = server.arg(argName);
    if (v != "") { if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1))) s_client->writeSilentStep(ax, i); }
  }

  // Time zone
  int ut_hrs = -999;
  v = server.arg("u1");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= -13) && (i <= 13)))
      ut_hrs = i;
  }
  v = server.arg("u2");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i == 00) || (i == 30) || (i == 45)))
    {
      if ((ut_hrs >= -13) && (ut_hrs <= 13))
        s_client->setTimeZone((float)(-(ut_hrs * 60 + i) / 60.0));
    }
  }
}
