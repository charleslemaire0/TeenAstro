#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuMotor(const uint8_t axis)
{
  static uint8_t s_sel = 1;
  uint8_t tmp_sel = s_sel;
  while (tmp_sel)
  {
    const char *string_list_Motor = T_SHOWSETTINGS "\n" T_ROTATION "\n" T_GEAR "\n" T_STEPSPERROT "\n"
      T_MICROSTEP "\n" T_BACKLASH "\n" T_BACKLASHSPEED  "\n" T_LOWCURR "\n" T_HIGHCURR "\n" "Silent" ;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, axis == 1 ? T_MOTOR " 1" : T_MOTOR " 2", tmp_sel, string_list_Motor);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    switch (tmp_sel)
    {
    case 1:
      DisplayMotorSettings(axis);
      break;
    case 2:
      menuSetReverse(axis);
      break;
    case 3:
      menuSetTotGear(axis);
      break;
    case 4:
      menuSetStepPerRot(axis);
      break;
    case 5:
      menuSetMicro(axis);
      break;
    case 6:
      menuSetBacklash(axis);
      break;
    case 7:
      menuSetBacklashRate(axis);
      break;
    case 8:
      menuSetCurrent(axis,0);
      break;
    case 9:
      menuSetCurrent(axis,1);
      break;
    case 10:
      menuSetSilentStep(axis);
      break;
    default:
      break;
    }
  }

}
void SmartHandController::DisplayMotorSettings(const uint8_t &axis)
{
  char line1[32] = "";
  char line2[32] = "";
  char line3[32] = "";
  char line4[32] = "";
  bool reverse;
  float backlash, totGear, stepPerRot;
  uint8_t microStep;
  unsigned int lowCurr, highCurr;
  sprintf(line1, T_MOTORSETTINGS, axis);
  if (DisplayMessageLX200(readReverseLX200(axis, reverse)))
  {
    reverse ? sprintf(line3, T_REVERSEDROTATION) : sprintf(line3, T_DIRECTROTATION);
  }
  if (DisplayMessageLX200(readTotGearLX200(axis, totGear)))
  {
    sprintf(line4, T_RATIO": %u", (unsigned int)totGear);
  }

  DisplayLongMessage(line1, NULL, line3, line4, -1);

  line2[0] = 0;
  line3[0] = 0;
  line4[0] = 0;

  if (DisplayMessageLX200(readStepPerRotLX200(axis, stepPerRot)))
  {
    sprintf(line2, "%u " T_STEPSPERROT, (unsigned int)stepPerRot);
  }
  if (DisplayMessageLX200(readMicroLX200(axis, microStep)))
  {
    sprintf(line3, T_MICROSTEP ": %u", (unsigned int)pow(2, microStep));
  }
  if (DisplayMessageLX200(readBacklashLX200(axis, backlash)))
  {
    sprintf(line4, T_BACKLASH": %u sec.", (unsigned int)backlash);
  }
  DisplayLongMessage(line1, line2, line3, line4, -1);
  line2[0] = 0;
  line3[0] = 0;
  line4[0] = 0;
  if (DisplayMessageLX200(readLowCurrLX200(axis, lowCurr)))
  {
    sprintf(line3, T_LOWCURR " %u mA", lowCurr);
  }
  if (DisplayMessageLX200(readHighCurrLX200(axis, highCurr)))
  {
    sprintf(line4, T_HIGHCURR " %u mA", highCurr);
  }

  DisplayLongMessage(line1, NULL, line3, line4, -1);
}
bool SmartHandController::menuSetReverse(const uint8_t &axis)
{
  bool reverse;
  if (!DisplayMessageLX200(readReverseLX200(axis, reverse)))
    return false;
  char text[20];
  char * string_list = T_DIRECT "\n" T_REVERSE;
  sprintf(text, T_ROTATION " M%u", axis);
  uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, text, (uint8_t)reverse + 1, string_list);
  if (choice)
  {
    reverse = (bool)(choice - 1);
    return DisplayMessageLX200(writeReverseLX200(axis, reverse), false);
  }
  return true;
}
bool SmartHandController::menuSetBacklash(const uint8_t &axis)
{
  float backlash;
  if (!DisplayMessageLX200(readBacklashLX200(axis, backlash)))
    return false;
  char text[20];
  sprintf(text, T_BACKLASH " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &backlash, 0, 999, 4, 0, " " T_INSECONDS))
  {
    return DisplayMessageLX200(writeBacklashLX200(axis, backlash), false);
  }
  return true;
}
bool SmartHandController::menuSetBacklashRate(const uint8_t& axis)
{
  float backlash;
  if (!DisplayMessageLX200(readBacklashRateLX200(axis, backlash)))
    return false;
  char text[20];
  sprintf(text, T_BACKLASHSPEED " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &backlash, 16, 64, 2, 0, ""))
  {
    return DisplayMessageLX200(writeBacklashRateLX200(axis, backlash), false);
  }
  return true;
}
bool SmartHandController::menuSetTotGear(const uint8_t &axis)
{
  float totGear;
  if (!DisplayMessageLX200(readTotGearLX200(axis, totGear)))
    return false;
  char text[20];
  sprintf(text, T_GEAR " M%u", axis);
  if (display->UserInterfaceInputValueFloat(&buttonPad, text, "", &totGear, 1, 60000, 8, 3, ""))
  {
    return DisplayMessageLX200(writeTotGearLX200(axis, totGear), false);
  }
  return true;
}
bool SmartHandController::menuSetStepPerRot(const uint8_t &axis)
{
  float stepPerRot;
  if (!DisplayMessageLX200(readStepPerRotLX200(axis, stepPerRot)))
    return false;
  char text[20];
  sprintf(text, T_STEPPER " M%u", axis);
  if (display->UserInterfaceInputValueFloatIncr(&buttonPad, text, "", &stepPerRot, 20, 400, 3, 0, 1, " " T_STEPS))
  {
    return DisplayMessageLX200(writeStepPerRotLX200(axis, stepPerRot), false);
  }
  return true;
}
bool SmartHandController::menuSetMicro(const uint8_t &axis)
{
  uint8_t microStep;
  if (!DisplayMessageLX200(readMicroLX200(axis, microStep)))
    return false;
  char text[20];
  char * string_list_micro = "2\n4\n8\n16 (~256)\n32\n64\n128\n256";
  sprintf(text, T_STEPPER " M%u", axis);
  uint8_t choice = microStep - 1 + 1;
  choice = display->UserInterfaceSelectionList(&buttonPad, text, choice, string_list_micro);
  if (choice)
  {
    microStep = choice - 1 + 1;
    return DisplayMessageLX200(writeMicroLX200(axis, microStep), false);
  }
  return true;
}
bool SmartHandController::menuSetSilentStep(const uint8_t &axis)
{
  uint8_t silent;
  if (!DisplayMessageLX200(readSilentStepLX200(axis, silent)))
    return false;
  char text[20];
  char * string_list_mode = T_OFF "\n" T_ON;
  sprintf(text, T_STEPPER " M%u", axis);
  uint8_t choice = silent + 1;
  choice = display->UserInterfaceSelectionList(&buttonPad, text, choice, string_list_mode);
  if (choice)
  {
    silent = choice - 1;
    return DisplayMessageLX200(writeSilentStepLX200(axis, silent), false);
  }
  return true;
}
bool SmartHandController::menuSetCurrent(const uint8_t &axis, bool high)
{
  unsigned int curr;
  uint8_t curr8;
  char text[20];
  if (high)
  {
    if (!DisplayMessageLX200(readHighCurrLX200(axis, curr)))
    {
      return false;
    }
    sprintf(text, T_HIGHCURR " M%u", axis);
  }
  else
  {
    if (!DisplayMessageLX200(readLowCurrLX200(axis, curr)))
    {
      return false;
    }
    sprintf(text, T_LOWCURR " M%u", axis);
  }

  curr8 = curr / 100;

  
  if (display->UserInterfaceInputValueInteger(&buttonPad, text, "", &curr8, 2, 28, 3, "00 mA " T_PEAK))
  {
    curr = (unsigned int)curr8 * 100;
    if (high)
      return DisplayMessageLX200(writeHighCurrLX200(axis, curr), false);
    else
      return DisplayMessageLX200(writeLowCurrLX200(axis, curr), false);
  }
  return true;
}
bool SmartHandController::menuSetEncoderReverse(const uint8_t& axis)
{
  bool reverse;
  if (!DisplayMessageLX200(readEncoderReverseLX200(axis, reverse)))
    return false;
  char text[20];
  char* string_list = T_DIRECT "\n" T_REVERSE;
  sprintf(text, T_ROTATION " M%u", axis);
  uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, text, (uint8_t)reverse + 1, string_list );
  if (choice)
  {
    reverse = (bool)(choice - 1);
    return DisplayMessageLX200(writeEncoderReverseLX200(axis, reverse), false);
  }
  return true;
}
bool SmartHandController::menuSetEncoderPulsePerDegree(const uint8_t& axis)
{
  float ppd;
  if (!DisplayMessageLX200(readPulsePerDegreeLX200(axis, ppd)))
    return false;
  char text[20];
  ppd /= 100;
  sprintf(text, T_PULSEPERDEGREE " E%u", axis);
  if (display->UserInterfaceInputValueFloatIncr( & buttonPad, text, "", &ppd, 0.5, 3600.00, 5, 2, 0.01, ""))
  {
    ppd *= 100;
    return DisplayMessageLX200(writePulsePerDegreeLX200(axis, ppd), false);
  }
  return true;
}