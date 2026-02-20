#include "SmartController.h"
#include "SHC_text.h"


//----------------------------------//
//           ENCODERS               //
//----------------------------------//
void SmartHandController::menuEncoders()
{

  if (ta_MountStatus.encodersEnable())
  {
    const char* string_list = T_AUTO_SYNC "\n" T_CALIBRATION "\n" T_PULSEPERDEGREE " E1\n" T_REVERSE " E1\n" T_PULSEPERDEGREE " E2\n" T_REVERSE " E2\n" T_DISABLE;
    static uint8_t s_sel = 1;
    uint8_t tmp_sel = s_sel;
    while (!exitMenu)
    {
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_ENCODERS, s_sel, string_list);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 1:
        menuAutoSyncEncoder();
        break;
      case 2:
        menuCalibrationEncoder();
        break;
      case 3:
        menuSetEncoderPulsePerDegree(1);
        break;
      case 4:
        menuSetEncoderReverse(1);
        break;
      case 5:
        menuSetEncoderPulsePerDegree(2);
        break;
      case 6:
        menuSetEncoderReverse(2);
        break;
      case 7:
        if (display->UserInterfaceMessage(&buttonPad, T_DISABLE, T_ENCODERS, "", T_NO "\n" T_YES) == 2)
        {
          if (m_client->enableEncoders(false) == LX200_VALUESET)
          {
            DisplayMessage(T_TELESCOPE, T_REBOOT, 500);
            powerCycleRequired = true;
            exitMenu = true;
          }
          else
          {
            DisplayMessage(T_DISABLE, T_FAILED, 500);
          }
        }
        break;
      default:
        return;
        break;
      }
    }
  }
  else
  {
    const char* string_list = T_ENABLE;
    static uint8_t s_sel = 1;
    uint8_t tmp_sel = s_sel;
    while (!exitMenu)
    {
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_ENCODERS, s_sel, string_list);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        return;
      case 1:
        if (display->UserInterfaceMessage(&buttonPad, T_ENABLE, T_ENCODERS, "", T_NO "\n" T_YES) == 2)
        {
          if (m_client->enableEncoders(true) == LX200_VALUESET)
          {
            DisplayMessage(T_TELESCOPE, T_REBOOT, 500);
            powerCycleRequired = true;
            exitMenu = true;
          }
          else
          {
            DisplayMessage(T_ENABLE, T_FAILED, 500);
          }
        }
        break;
      default:
        break;
      }
    }
  }
}

void SmartHandController::menuAutoSyncEncoder()
{
  const char* string_list = T_OFF "\n60'\n30'\n15'\n8'\n4'\n2'\n" T_ON;
  static uint8_t s_sel = 1;
  DisplayMessageLX200(m_client->readEncoderAutoSync(s_sel), true);
  uint8_t tmp_sel = s_sel + 1;
  while (tmp_sel)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_AUTO_SYNC, tmp_sel, string_list);
    if (tmp_sel && tmp_sel != s_sel + 1)
    {
      DisplayMessageLX200(m_client->writeEncoderAutoSync(tmp_sel - 1), false);
      return;
    }
  }
}

void SmartHandController::menuCalibrationEncoder()
{
  const char* string_list = T_STAR "\n" T_CANCEL "\n" T_COMPLETE;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (!exitMenu)
  {
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_CALIBRATION, s_sel, string_list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      DisplayMessageLX200(m_client->startEncoderCalibration(), false);
      exitMenu = true;
      break;
    case 2:
      DisplayMessageLX200(m_client->cancelEncoderCalibration(), false);
      exitMenu = true;
      break;
    case 3:
      DisplayMessageLX200(m_client->completeEncoderCalibration(), false);
      exitMenu = true;
      break;
    default:
      return;
      break;
    }
  }
}