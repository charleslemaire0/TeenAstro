
#include <TeenAstroMountStatus.h>
#include "SHC_text.h"
#include "SmartController.h"

static char* BreakRC[6] = { ":Qn#" ,":Qs#" ,":Qe#" ,":Qw#", ":Fo#", ":Fi#" };
static char* RC[6] = { ":Mn#" , ":Ms#" ,":Me#" ,":Mw#", ":FO#", ":FI#" };

void SmartHandController::setup(
  const char version[], 
  const int pin[7], 
  const bool active[7], 
  const int SerialBaud, 
  const OLED model,
  const uint8_t nSubmodel)
{
#ifdef ARDUINO_LOLIN_C3_MINI
  Ser.begin(SerialBaud, SERIAL_8N1, RX, TX);
#else
  Ser.begin(SerialBaud);
#endif

  if (EEPROM.length() == 0)
#ifdef ARDUINO_D1_MINI32
    EEPROM.begin(512);
#else
    EEPROM.begin(1024);
#endif

  if (strlen(version) <= 19) strcpy(_version, version);

  //choose a 128x64 display supported by U8G2lib (if not listed below there are many many others in u8g2 library example Sketches)

  num_supported_display = nSubmodel;
  uint8_t submodel = EEPROM.read(EEPROM_DISPLAYSUBMODEL);
  switch (model)
  {
  case OLED_SH1106:
    display = new U8G2_EXT_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0);
    break;
  case OLED_SSD1306:
    display = new U8G2_EXT_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0);
    break;
  case OLED_SSD1309:
    if (!(submodel < num_supported_display))
    {
      submodel = 0;
      EEPROM.write(EEPROM_DISPLAYSUBMODEL, 0);
    }
    if (submodel == 0)
      display = new U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C(U8G2_R0);
    else if (submodel == 1)
      display = new U8G2_EXT_SSD1309_128X64_NONAME2_F_HW_I2C(U8G2_R0);
    else
      display = new U8G2_EXT_SSD1309_128X64_NONAME_F_HW_I2C(U8G2_R0);
    break;
  }
  SHCrotated = EEPROM.read(EEPROM_DISPLAY180) == 255;
  
  if (SHCrotated)
  {
    display->setDisplayRotation(U8G2_R2);
  }

  display->begin();
  drawIntro();
  buttonPad.setup(pin, active, EEPROM_BSPEED, SHCrotated);
  tickButtons();
  maxContrast = EEPROM.read(EEPROM_Contrast);
  display->setContrast(maxContrast);
  displayT1 = EEPROM.read(EEPROM_T1);
  if (displayT1 < 3)
  {
    displayT1 = 3;
    EEPROM.write(EEPROM_T1, displayT1);
    EEPROM.commit();
  }
  displayT2 = EEPROM.read(EEPROM_T2);
  if (displayT2 < displayT1)
  {
    displayT2 = displayT1;
    EEPROM.write(EEPROM_T2, displayT2);
    EEPROM.commit();
  }

#ifdef DEBUG_ON
  DebugSer.begin(9600);
  delay(1000);
#endif
  display->setFont(u8g2_font_helvR12_te);
  int k = 0;
  while (!ta_MountStatus.hasInfoV() && k < 10)
  {
    ta_MountStatus.updateV();
    ta_MountStatus.removeLastConnectionFailure();
    delay(500);
    k++;
  }
  DisplayMessage("SHC " T_VERSION, _version, 1500);
  if (k == 10)
  {
    return;
  }
  DisplayMessage("Main Unit " T_VERSION, ta_MountStatus.getVN(), 1500);
  if (ta_MountStatus.checkConnection(SHCFirmwareVersionMajor, SHCFirmwareVersionMinor))
  {    
    if (ta_MountStatus.findFocuser())
    {
      char out[50];
      if (DisplayMessageLX200(GetLX200(":FV#", out, sizeof(out))))
      {
        out[31] = 0;
        DisplayMessage("Focuser " T_VERSION, &out[26], 1500);
      }
    }
    ta_MountStatus.updateMount();
    if (!ta_MountStatus.hasGNSSBoard())
    {
      ta_MountStatus.updateTime();
      unsigned int hour = 0, minute = 0, second = 0;
      GetLocalTimeLX200(hour, minute, second);
      char date_time[40];
      sprintf(date_time, "%s : %.2d:%.2d:%.2d", T_TIME, hour, minute, second);
      char date_time2[40];
      sprintf(date_time2, "%s : %s", T_DATE, ta_MountStatus.getUTCdate());
      DisplayMessage(date_time, date_time2, 2000);
    }
    else
    {
      DisplayMessage("GNSS", T_CONNECTED, 1500);
    }
  }
#ifdef RADEC_PAGE
    pages[P_RADEC].show = true;
#endif
#ifdef HA_PAGE
    pages[P_HADEC].show = true;
#endif
#ifdef ALTAZ_PAGE
    pages[P_ALTAZ].show = true;
#endif
#ifdef PUSH_PAGE
    pages[P_PUSH].show = true;
#endif
#ifdef TIME_PAGE
    pages[P_TIME].show = true;
#endif
#ifdef AXIS_STEP_PAGE
    pages[P_AXIS_STEP].show = true;
#endif
#ifdef AXIS_DEG_PAGE
    pages[P_AXIS_DEG].show = true;
#endif
#ifdef FOCUSER_PAGE
    pages[P_FOCUSER].show = true;
#endif
#ifdef ALIGN_PAGE
    pages[P_ALIGN].show = true;
#endif

}

void SmartHandController::getNextpage()
{
  for (int k = 1; k < NUMPAGES + 1; k++)
  {
    current_page++;
    if (current_page >= NUMPAGES)
      current_page = 0;
    if (pages[current_page].show)
    {
      if (pages[current_page].p == P_FOCUSER && !ta_MountStatus.hasFocuser())
      {
        pages[current_page].show = false;
        continue;
      }
      break;
    }
  }
}

void SmartHandController::updateAlign(bool moving)
{
  if (ta_MountStatus.isAlignSelect())
  {
    char message[10] = T_STAR "#?";
    message[6] = '0' + ta_MountStatus.getAlignStar();
    DisplayLongMessage(T_SELECTASTAR, T_FROMFOLLOWINGLIST, "", message, -1);
    if (!SelectStarAlign())
    {
      DisplayMessage(T_SELECTION, T_ABORTED, -1);
      ta_MountStatus.backStepAlign();
      return;
    }
    else
    {
      ta_MountStatus.nextStepAlign();
    }
  }
  else if (top - lastpageupdate > 200)
  {
    updateMainDisplay(pages[current_page].p);
  }
  if (moving)
  {
    return;
  }
  else if (eventbuttons[0] == E_CLICK && ta_MountStatus.isAlignRecenter())
  {
    TeenAstroMountStatus::AlignReply reply = ta_MountStatus.addStar();
    switch (reply)
    {
    case TeenAstroMountStatus::AlignReply::ALIR_FAILED1:
      DisplayMessage(T_ALIGNMENT, T_FAILED"!", -1);
      break;
    case TeenAstroMountStatus::AlignReply::ALIR_FAILED2:
      DisplayMessage(T_ALIGNMENT, T_WRONG"!", -1);
      break;
    case TeenAstroMountStatus::AlignReply::ALIR_DONE:
      DisplayMessage(T_ALIGNMENT, T_SUCESS"!", -1);
      break;
    case TeenAstroMountStatus::AlignReply::ALIR_ADDED:
      DisplayMessage(T_STARADDED, "=>", 1000);
      break;
    }
  }
  return;
}

void SmartHandController::updatePushing(bool moving)
{
  if (top - lastpageupdate > 200)
  {
    updateMainDisplay(pages[P_PUSH].p);
  }
  if (moving)
  {
    return;
  }
  else if (eventbuttons[0] == E_CLICK)
  {
    buttonPad.setMenuMode();
    if (display->UserInterfaceMessage(&buttonPad, T_SYNCEDAT, T_TARGET, "", T_YES "\n" T_NO) == 1)
    {
      DisplayMessageLX200(SetLX200(":ECS#"), 0.5);
    }
    buttonPad.setControlerMode();
    DisplayMessageLX200(SetLX200(":EMQ#"));
  }
}


void SmartHandController::update()
{
  bool moving = false;
  tickButtons();
  top = millis();
  if (isSleeping())
    return;

  if (!ta_MountStatus.isConnectionValid() && ta_MountStatus.hasInfoV())
  {
    display->sleepOff();
    buttonPad.setMenuMode();
    DisplayMessage("!! " T_ERROR " !!", T_VERSION, -1);
#ifdef ARDUINO_D1_MINI32
    ESP.restart();
#endif
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
    ESP.reset();
#endif
    return;
  }

  if (powerCycleRequired)
  {
    display->sleepOff();
    buttonPad.setMenuMode();
    DisplayMessage(T_PRESS_KEY, T_TO_REBOOT "...", -1);
    DisplayMessage(T_DEVICE, T_WILL_REBOOT "...", 1000);

#ifdef ARDUINO_D1_MINI32
    ESP.restart();
#endif
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
    ESP.reset();
#endif
    return;
  }
  if (ta_MountStatus.notResponding())
  {
    display->sleepOff();
    buttonPad.setMenuMode();
    DisplayMessage("!! " T_ERROR " !!", T_NOT_CONNECTED, -1);
    DisplayMessage(T_DEVICE, T_WILL_REBOOT "...", 1000);

#ifdef ARDUINO_D1_MINI32
    ESP.restart();
#endif
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
    ESP.reset();
#endif

  }

  manualMove(moving);
  if (ta_MountStatus.isAligning())
  {
    updateAlign(moving);
    return;
  }
  if (ta_MountStatus.isPushingto())
  {
    updatePushing(moving);
    return;
  }

  if (top - lastpageupdate > 200)
  {

    updateMainDisplay(pages[current_page].p);
  }

  if (eventbuttons[0] == E_CLICK )
  {
    getNextpage();
    time_last_action = millis();
  }
  else if (moving)
  {
    return;
  }
  else if (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART)
  {
    if (eventbuttons[3] == E_LONGPRESS || eventbuttons[3] == E_CLICK || eventbuttons[3] == E_LONGPRESSTART)
    {
      menuTelAction();
    }
    else if (eventbuttons[1] == E_LONGPRESS || eventbuttons[1] == E_CLICK || eventbuttons[1] == E_LONGPRESSTART)
    {
    #ifdef NO_SPEED_MENU
      increaseSpeed(true);
    #else
      menuSpeedRate();
    #endif
    }
    #ifdef NO_SPEED_MENU
    else if (eventbuttons[2] == E_LONGPRESS || eventbuttons[2] == E_CLICK || eventbuttons[2] == E_LONGPRESSTART)
    {
      increaseSpeed(false);
    }
    #endif
    else if (eventbuttons[4] == E_LONGPRESS || eventbuttons[4] == E_CLICK || eventbuttons[4] == E_LONGPRESSTART)
    {
      menuTelSettings();
    }
    else if (eventbuttons[6] == E_LONGPRESS || eventbuttons[6] == E_CLICK || eventbuttons[6] == E_LONGPRESSTART)
    {
      menuFocuserAction();
    }
    else if (eventbuttons[5] == E_LONGPRESS || eventbuttons[5] == E_CLICK || eventbuttons[5] == E_LONGPRESSTART)
    {
      menuFocuserSettings();
    }
    exitMenu = false;
    time_last_action = millis();
  }
 
}

void SmartHandController::tickButtons()
{
  buttonPad.tickButtons();
}

bool SmartHandController::buttonPressed()
{
  return buttonPad.buttonPressed();
}

bool SmartHandController::isSleeping()
{
  if (forceDisplayoff)
  {
    if (!buttonPad.shiftPressed())
    {
      bool moving = false;
      manualMove(moving);
      return true;
    }
    else
      forceDisplayoff = false;
  }
  if (buttonPressed())
  {
    time_last_action = millis();
    if (sleepDisplay)
    {
      display->setContrast(maxContrast);
      display->sleepOff();
      sleepDisplay = false;
      lowContrast = false;
      buttonPad.setControlerMode();
      return true;
    }
    if (lowContrast)
    {
      lowContrast = false;
      display->setContrast(maxContrast);
      buttonPad.setControlerMode();
      return true;
    }
  }
  else if (sleepDisplay)
  {
    return true;
  }
  else if ((top - time_last_action) / 10000 > displayT2)
  {
    display->sleepOn();
    sleepDisplay = true;
    buttonPad.setMenuMode();
    return false;
  }
  else if ((top - time_last_action) / 10000 > displayT1 && !lowContrast)
  {
    display->setContrast(0);
    lowContrast = true;
    buttonPad.setMenuMode();
    return true;
  }
  return false;
}

void SmartHandController::manualMove(bool &moving)
{
  moving = ta_MountStatus.getTrackingState() == TeenAstroMountStatus::TRK_SLEWING ||
    ta_MountStatus.getParkState() == TeenAstroMountStatus::PRK_PARKING ||
    ta_MountStatus.isSpiralRunning();
  if (moving)
  {
    bool stop = (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART || eventbuttons[0] == E_DOUBLECLICK) ? true : false;
    int it = 1;
    while (!stop && it < 5)
    {
      stop = (eventbuttons[it] == E_LONGPRESS || eventbuttons[it] == E_CLICK || eventbuttons[it] == E_LONGPRESSTART);
      it++;
    }
    if (stop)
    {
      Ser.print(":Q#");
      Ser.flush();
      time_last_action = millis();
      display->sleepOff();
      ta_MountStatus.backStepAlign();
      return;
    }
  }
  else
  {
    buttonCommand = false;
    for (int k = 1; k < 7; k++)
    {
      if (Move[k - 1] && (eventbuttons[k] == E_LONGPRESSSTOP || eventbuttons[k] == E_NONE))
      {
        buttonCommand = true;
        Move[k - 1] = false;
        if (k < 5)
          SetLX200(BreakRC[k - 1]);
        else
          Move[k - 1] = !(SetLX200(BreakRC[k - 1]) == LX200_VALUESET);
        continue;
      }
      else if (eventbuttons[0] == E_NONE && !Move[k - 1] && (eventbuttons[k] == E_LONGPRESS || eventbuttons[k] == E_CLICK || eventbuttons[k] == E_LONGPRESSTART))
      {
        buttonCommand = true;

        if (k < 5)
        {
          if (!telescoplocked)
          {
            Move[k - 1] = true;
            SetLX200(RC[k - 1]);
          }
        }
        else if (!focuserlocked)
          Move[k - 1] = (SetLX200(RC[k - 1]) == LX200_VALUESET);
        continue;
      }
      moving = moving || Move[k - 1];
    }
    if (buttonCommand)
    {
      time_last_action = millis();
      return;
    }
  }
}
