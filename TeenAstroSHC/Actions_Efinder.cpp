/**
 * SHC eFinder menu — drives TeenAstroMainUnit :e…# commands.
 */
#include "SmartController.h"
#include "SHC_text.h"
#include <stdio.h>

void SmartHandController::waitPlateAlign()
{
  char out[64];
  const unsigned long deadline = millis() + 20UL * 60UL * 1000UL;
  const unsigned long arm = millis() + 400UL;
  bool stopped = false;
  while ((long)(deadline - millis()) > 0)
  {
    tickButtons();
    if ((long)(millis() - arm) >= 0 && buttonPressed())
    {
      m_client->efinderAbort();
      DisplayMessage(T_EFINDER_ALIGN, T_ABORTED, 1000);
      stopped = true;
      break;
    }
    if (m_client->efinderResult(out, sizeof(out)) != LX200_VALUEGET)
      break;
    if (out[0] != 'B')
    {
      if (out[0] == 'S') DisplayMessage(T_EFINDER_ALIGN, T_SUCESS, 1500);
      else if (out[0] == 'F') DisplayMessage(T_EFINDER_ALIGN, T_FAILED, 1500);
      else DisplayMessage(T_EFINDER_ALIGN, out, 1500);
      break;
    }
    unsigned n = 0, tot = 0;
    char line[12] = "";
    if (sscanf(out, "B%u,%u", &n, &tot) == 2)
      snprintf(line, sizeof(line), "%u/%u", n, tot);
    DisplayMessage(T_EFINDER_ALIGN, line[0] ? line : T_EFINDER_SOLVING, 200);
  }
  if (!stopped && (long)(deadline - millis()) <= 0)
    DisplayMessage(T_EFINDER_ALIGN, T_FAILED, 1500);
}

bool SmartHandController::waitEfinderResult(char* out, int len)
{
  for (int i = 0; i < 60; i++)
  {
    if (m_client->efinderResult(out, len) != LX200_VALUEGET)
      return false;
    if (out[0] != 'B')
      return true;
    DisplayMessage(T_EFINDER, T_EFINDER_SOLVING, 200);
  }
  return false;
}

void SmartHandController::menuEfinder()
{
  if (!ta_MountStatus.hasEfinder())
  {
    DisplayMessage(T_EFINDER, T_NOT_CONNECTED "!", 500);
    return;
  }

  buttonPad.setMenuMode();
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  char out[64];

  while (!exitMenu)
  {
    const char* list =
      T_EFINDER_SOLVE_SYNC "\n"
      T_EFINDER_SOLVE "\n"
      T_EFINDER_ALIGN "\n"
      T_EFINDER_GOTOPP "\n"
      T_EFINDER_CALIBRATE "\n"
      T_EFINDER_AIM "\n"
      T_EFINDER_EXPOSURE "\n"
      T_EFINDER_INFO;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_EFINDER, s_sel, list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 0:
      exitMenu = true;
      break;
    case 1:
      if (DisplayMessageLX200(m_client->efinderSolveSync()))
      {
        if (waitEfinderResult(out, sizeof(out)))
        {
          if (out[0] == 'S') DisplayMessage(T_EFINDER, T_SYNCED, 1000);
          else if (out[0] == 'F') DisplayMessage(T_EFINDER, T_FAILED, 1000);
          else DisplayMessage(T_EFINDER, out, 1000);
        }
      }
      break;
    case 2:
      if (DisplayMessageLX200(m_client->efinderSolve()))
      {
        if (waitEfinderResult(out, sizeof(out)))
          DisplayMessage(T_EFINDER, out, 1500);
      }
      break;
    case 3:
    {
      const TeenAstroMountStatus::Mount mt = ta_MountStatus.getMount();
      const bool showSix = (mt == TeenAstroMountStatus::MOUNT_TYPE_GEM
                            || mt == TeenAstroMountStatus::MOUNT_UNDEFINED);
      char menu[48];
      strcpy(menu, "2 " T_STARS "\n" T_ALIGN_PERP);
      if (showSix)
        strcat(menu, "\n" T_ALIGN_CONE);
      const uint8_t c = display->UserInterfaceSelectionList(&buttonPad, T_EFINDER_ALIGN, 1, menu);
      if (c == 0)
        break;
      const uint8_t stars = (c == 1) ? 2 : (c == 2) ? 4 : 6;
      DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
      const LX200RETURN started = (stars == 6)
          ? m_client->efinderAlign()
          : m_client->efinderAlignFromHome(stars);
      if (DisplayMessageLX200(started))
        waitPlateAlign();
      break;
    }
    case 4:
    {
      if (m_client->efinderGotoPPSettings(out, sizeof(out)) != LX200_VALUEGET)
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
        break;
      }
      bool on = (out[0] == '1');
      char thr[8] = "2";
      char* comma = strchr(out, ',');
      if (comma) {
        strncpy(thr, comma + 1, sizeof(thr) - 1);
        thr[sizeof(thr) - 1] = 0;
        char* hash = strchr(thr, '#');
        if (hash) *hash = 0;
      }
      char menu[80];
      snprintf(menu, sizeof(menu), "%s\n%s\n%s %s'",
               on ? T_OFF : T_ON, T_EFINDER_THRESHOLD, T_EFINDER_THRESHOLD, thr);
      uint8_t c = display->UserInterfaceSelectionList(&buttonPad, T_EFINDER_GOTOPP, 1, menu);
      if (c == 1)
        DisplayMessageLX200(m_client->efinderSetGotoPP(!on));
      else if (c == 2 || c == 3)
      {
        float v = (float)atoi(thr);
        if (display->UserInterfaceInputValueFloat(&buttonPad, T_EFINDER_THRESHOLD, "", &v, 1, 60, 2, 0, "'"))
          DisplayMessageLX200(m_client->efinderSetThreshold((uint8_t)v));
      }
      break;
    }
    case 5:
      if (DisplayMessageLX200(m_client->efinderOffset()))
      {
        if (waitEfinderResult(out, sizeof(out)))
          DisplayMessage(T_EFINDER_CALIBRATE, out, 1500);
      }
      break;
    case 6:
    {
      DisplayLongMessage(T_EFINDER_AIM, T_EFINDER_AIM_1, T_EFINDER_AIM_2, NULL, -1);
      if (DisplayMessageLX200(m_client->efinderAimCal()))
      {
        if (waitEfinderResult(out, sizeof(out)))
        {
          double hz = 0, ve = 0;
          if (out[0] == 'F')
            DisplayMessage(T_EFINDER_AIM, T_FAILED, 1500);
          else if (sscanf(out, "O%lf,%lf", &hz, &ve) == 2)
          {
            char hzLine[32];
            char veLine[32];
            snprintf(hzLine, sizeof(hzLine), "%s %+.2f'", T_EFINDER_HZ, hz);
            snprintf(veLine, sizeof(veLine), "%s %+.2f'", T_EFINDER_VE, ve);
            DisplayLongMessage(T_EFINDER_AIM, hzLine, veLine, NULL, 2500);
          }
          else
            DisplayMessage(T_EFINDER_AIM, out, 1500);
        }
      }
      break;
    }
    case 7:
    {
      if (m_client->efinderParam("Exposure", out, sizeof(out)) != LX200_VALUEGET)
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
        break;
      }
      float expv = (float)atof(out);
      if (display->UserInterfaceInputValueFloat(&buttonPad, T_EFINDER_EXPOSURE, "", &expv, 0.1, 30, 4, 1, "s"))
      {
        char body[24];
        snprintf(body, sizeof(body), "Exposure=%.1f", (double)expv);
        DisplayMessageLX200(m_client->efinderParam(body, out, sizeof(out)));
      }
      if (m_client->efinderParam("Gain", out, sizeof(out)) == LX200_VALUEGET)
      {
        float gain = (float)atof(out);
        if (display->UserInterfaceInputValueFloat(&buttonPad, T_EFINDER_GAIN, "", &gain, 1, 600, 3, 0, ""))
        {
          char body[24];
          snprintf(body, sizeof(body), "Gain=%.0f", (double)gain);
          DisplayMessageLX200(m_client->efinderParam(body, out, sizeof(out)));
        }
      }
      break;
    }
    case 8:
      if (m_client->efinderStatus(out, sizeof(out)) == LX200_VALUEGET)
        DisplayMessage(T_EFINDER_INFO, out, 2000);
      else
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      break;
    default:
      break;
    }
  }
  buttonPad.setControlerMode();
}
