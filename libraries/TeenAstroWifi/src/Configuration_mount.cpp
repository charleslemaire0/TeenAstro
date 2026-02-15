#include "TeenAstroWifi.h"
#include "HtmlCommon.h"
// -----------------------------------------------------------------------------------
// configuration_telescope

const char html_configMountSelect1[] PROGMEM =
"<div class='bt' align='left'> Selected Mount :<br/> </div>"
"<form method='post' action='/configuration_mount.htm'>"
"<select onchange='this.form.submit()' style='width:11em' name='mount_select'>";
const char html_configMountSelect2[] PROGMEM =
"</select>"
" (Select your predefined mount)"
"</form>"
"<br/>\r\n";

const char html_configMountName1[] PROGMEM =
"<div class='bt' align='left'> Selected Mount definition: <br/> </div>"
"<form method='get' action='/configuration_mount.htm'>";
const char html_configMountName2[] PROGMEM =
" <input value='%s' style='width:10.25em' type='text' name='mount_n' maxlength='14'>";
const char html_configMountName3[] PROGMEM =
"<button type='submit'>Upload</button>"
" (Edit the name of the selected mount)"
"</form>"
"<br/>\r\n";
const char html_configMount_1[] PROGMEM =
"<div class='bt'>Mount Type: <br/> </div>"
"<form action='/configuration_mount.htm'>"
"<select onchange='this.form.submit()' style='width:11em' name='mount'>";
const char html_configMount_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configRefraction[] PROGMEM =
"<div class='bt'> Refraction Options: <br/> </div>";
const char html_Opt_1[] PROGMEM =
"<form action='/configuration_mount.htm'>"
"<select name='%s' onchange='this.form.submit()' >";

const char html_reboot_t[] PROGMEM =
"<br/><form method='get' action='/configuration_mount.htm'>"
"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
"<button type='submit'>Continue</button>"
"</form><br/><br/><br/><br/>"
"\r\n";


bool restartRequired_t = false;

void TeenAstroWifi::handleConfigurationMount()
{
  s_client->setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  String data;
  int selectedmount = 0;
  processConfigurationMountGet();
  preparePage(data, ServerPage::Mount);
  sendHtml(data);
  if (restartRequired_t)
  {
    data += FPSTR(html_reboot_t);
    data += FPSTR(html_pageFooterNested);
    sendHtml(data);
    sendHtmlDone(data);
    restartRequired_t = false;
    delay(1000);
    return;
  }

  if (s_client->getMountIdx(selectedmount) == LX200_VALUEGET)
  {
    char mount0[32]; char mount1[32];
    s_client->getMountName(0, mount0, sizeof(mount0));
    s_client->getMountName(1, mount1, sizeof(mount1));
    data += FPSTR(html_configMountSelect1);
    sendHtml(data);
    selectedmount == 0 ? data += "<option selected value='0'>" : data += "<option value='0'>";
    sprintf(temp, "%s</option>", mount0);
    data += temp;
    selectedmount == 1 ? data += "<option selected value='1'>" : data += "<option value='1'>";
    sprintf(temp, "%s</option>", mount1);
    data += temp;
    data += FPSTR(html_configMountSelect2);
    sendHtml(data);
    // Name
    data += FPSTR(html_configMountName1);
    sprintf_P(temp, html_configMountName2, selectedmount == 0 ? mount0 : mount1);
    data += temp;
    data += FPSTR(html_configMountName3);
    sendHtml(data);

    ta_MountStatus.updateMount();

    data += FPSTR(html_configMount_1);
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM ? data += "<option selected value='1'>German Eq</option>" : data += "<option value='1'>German Eq</option>";
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_FORK ? data += "<option selected value='2'>Fork Eq</option>" : data += "<option value='2'>Fork Eq</option>";
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM ? data += "<option selected value='3'>German Alt Az</option>" : data += "<option value='3'>German Alt Az</option>";
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT ? data += "<option selected value='4'>Fork Alt Az</option>" : data += "<option value='4'>Fork Alt Az</option>";
    data += FPSTR(html_configMount_2);

    sprintf_P(temp, html_Opt_1, "motors");
    data += temp;
    ta_MountStatus.motorsEnable() ? data += FPSTR(html_optOnSel) : data += FPSTR(html_optOnUnsel);
    !ta_MountStatus.motorsEnable() ? data += FPSTR(html_optOffSel) : data += FPSTR(html_optOffUnsel);
    data += "</select> Enable Motors</form><br/>\r\n";
    sendHtml(data);

    sprintf_P(temp, html_Opt_1, "encoders");
    data += temp;
    ta_MountStatus.encodersEnable() ? data += FPSTR(html_optOnSel) : data += FPSTR(html_optOnUnsel);
    !ta_MountStatus.encodersEnable() ? data += FPSTR(html_optOffSel) : data += FPSTR(html_optOffUnsel);
    data += "</select> Enable Encoders</form><br/>\r\n";
    sendHtml(data);

    data += FPSTR(html_configRefraction);
    if (!ta_MountStatus.isAltAz())
    {
      if (s_client->getPolarAlignEnabled(temp1, sizeof(temp1)) != LX200_GETVALUEFAILED)
      {
        sprintf_P(temp, html_Opt_1, "polar");
        data += temp;
        temp1[0] == 'y' ? data += FPSTR(html_optOnSel) : data += FPSTR(html_optOnUnsel);
        temp1[0] == 'n' ? data += FPSTR(html_optOffSel) : data += FPSTR(html_optOffUnsel);
        data += "</select> Consider Refraction for Pole definition</form><br/>\r\n";
        sendHtml(data);
      }
    }
    if (s_client->getGoToEnabled(temp1, sizeof(temp1)) != LX200_GETVALUEFAILED)
    {
      sprintf_P(temp, html_Opt_1, "gotor");
      data += temp;
      temp1[0] == 'y' ? data += FPSTR(html_optOnSel) : data += FPSTR(html_optOnUnsel);
      temp1[0] == 'n' ? data += FPSTR(html_optOffSel) : data += FPSTR(html_optOffUnsel);
      data += "</select> Consider Refraction for Goto and Sync</form><br/>\r\n";
      sendHtml(data);
    }
  }
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationMountGet()
{
  String v;
  int i;

  // selected Mount
  v = server.arg("mount_select");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      if (s_client->setMount(i) == LX200_VALUESET)
        restartRequired_t = true;
    }
  }
  // name
  v = server.arg("mount_n");
  if (v != "")
    s_client->setMountDescription(v.c_str());

  // mount type
  v = server.arg("mount");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 4)))
    {
      if (s_client->setMountType(i) == LX200_VALUESET)
        restartRequired_t = true;
    }
  }

  v = server.arg("motors");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      if ((i == 1 ? s_client->enableMotors(true) : s_client->enableMotors(false)) == LX200_VALUESET)
        restartRequired_t = true;
    }
  }

  v = server.arg("encoders");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      if ((i == 1 ? s_client->enableEncoders(true) : s_client->enableEncoders(false)) == LX200_VALUESET)
        restartRequired_t = true;
    }
  }

  v = server.arg("polar");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
      i == 1 ? s_client->enablePolarAlign(true) : s_client->enablePolarAlign(false);
  }

  v = server.arg("gotor");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
      i == 1 ? s_client->enableGoTo(true) : s_client->enableGoTo(false);
  }
}
