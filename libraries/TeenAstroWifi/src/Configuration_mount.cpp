#include <TeenAstroLX200io.h>
#include "TeenAstroWifi.h"
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
"<div class='bt'> Equatorial Mount Type: <br/> </div>"
"<form action='/configuration_mount.htm'>"
"<select name='mount' onchange='this.form.submit()' >";
const char html_configMount_2[] PROGMEM =
"</select>"
"</form>"
"<br/>\r\n";

const char html_configRefraction[] PROGMEM =
"<div class='bt'> Refraction Options: <br/> </div>";
const char html_Opt_1[] PROGMEM =
"<form action='/configuration_mount.htm'>"
"<select name='%s' onchange='this.form.submit()' >";

const char html_on_1[] PROGMEM = "<option selected value='1'>On</option>";
const char html_on_2[] PROGMEM = "<option value='1'>On</option>";
const char html_off_1[] PROGMEM = "<option selected value='2'>Off</option>";
const char html_off_2[] PROGMEM = "<option value='2'>Off</option>";



const char html_reboot_t[] PROGMEM =
"<br/><form method='get' action='/configuration_mount.htm'>"
"<b>The main unit will now restart please wait some seconds and then press continue.</b><br/><br/>"
"<button type='submit'>Continue</button>"
"</form><br/><br/><br/><br/>"
"\r\n";



bool restartRequired_t = false;

void TeenAstroWifi::handleConfigurationMount()
{
  Ser.setTimeout(WebTimeout);
  sendHtmlStart();
  char temp[320] = "";
  char temp1[50] = "";
  char temp2[50] = "";
  String data;
  int selectedmount = 0;
  processConfigurationMountGet();
  preparePage(data, ServerPage::Mount);
  sendHtml(data);
  if (restartRequired_t)
  {
    data += FPSTR(html_reboot_t);
    data += "</div></div></body></html>";
    sendHtml(data);
    sendHtmlDone(data);
    restartRequired_t = false;
    delay(1000);
    return;
  }
  //update

  if (GetMountIdxLX200(selectedmount) == LX200_VALUEGET)
  {
    char mount0[32]; char mount1[32];
    GetMountNameLX200(0, mount0, sizeof(mount0));
    GetMountNameLX200(1, mount1, sizeof(mount1));
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
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_GEM ? data += "<option selected value='1'>German</option>" : data += "<option value='1'>German</option>";
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_FORK ? data += "<option selected value='2'>Fork</option>" : data += "<option value='2'>Fork</option>";
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM ? data += "<option selected value='3'>Alt Az</option>" : data += "<option value='3'>Alt Az</option>";
    ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT ? data += "<option selected value='4'>Alt Az Fork</option>" : data += "<option value='4'>Alt Az Fork</option>";
    data += FPSTR(html_configMount_2);
    sendHtml(data);

    data += FPSTR(html_configRefraction);
    if (!ta_MountStatus.isAltAz())
    {
      if (GetLX200(":GXrp#", temp1, sizeof(temp1)) != LX200_GETVALUEFAILED) ;
      {
        sprintf_P(temp, html_Opt_1, "polar");
        data += temp;
        temp1[0] == 'y' ? data += FPSTR(html_on_1) : data += FPSTR(html_on_2);
        temp1[0] == 'n' ? data += FPSTR(html_off_1) : data += FPSTR(html_off_2);
        data += "</select> Consider Refraction for Pole definition</form><br/>\r\n";
        sendHtml(data);
      }
    }
    if (GetLX200(":GXrg#", temp1, sizeof(temp1)) != LX200_GETVALUEFAILED)
    {
      sprintf_P(temp, html_Opt_1, "gotor");
      data += temp;
      temp1[0] == 'y' ? data += FPSTR(html_on_1) : data += FPSTR(html_on_2);
      temp1[0] == 'n' ? data += FPSTR(html_off_1) : data += FPSTR(html_off_2);
      data += "</select> Consider Refraction for Goto and Sync</form><br/>\r\n";
      sendHtml(data);
    }
  }
  strcpy(temp, "</div></body></html>");
  data += temp;
  sendHtml(data);
  sendHtmlDone(data);
}

void TeenAstroWifi::processConfigurationMountGet()
{
  String v;
  int i;
  float f;
  char temp[20] = "";

  // selected Mount
  v = server.arg("mount_select");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 0) && (i <= 1)))
    {
      SetMountLX200(i);
      restartRequired_t = true;
    }
  }
  // name
  v = server.arg("mount_n");
  if (v != "")
  {
    sprintf(temp, ":SXOA,%s#", (char*)v.c_str());
    SetLX200(temp);
  }

  v = server.arg("mount");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 4)))
    {
      sprintf(temp, ":S!X#");
      temp[3] = '0' + i;
      SetLX200(temp);
      restartRequired_t = true;
    }
  }

  v = server.arg("polar");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? SetLX200(":SXrp,y#") : SetLX200(":SXrp,n#");
    }
  }

  v = server.arg("gotor");
  if (v != "")
  {
    if ((atoi2((char*)v.c_str(), &i)) && ((i >= 1) && (i <= 2)))
    {
      i == 1 ? SetLX200(":SXrg,y#") : SetLX200(":SXrg,n#");
    }
  }
}

