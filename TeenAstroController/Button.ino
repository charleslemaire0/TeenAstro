

void click_s() {
  eventbuttons[B_SHIFT] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button s click.");
#endif // DEBUGBUTTON
} // click2

void doubleclick_s() {
  eventbuttons[B_SHIFT] = E_DOUBLECLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button s doubleclick.");
#endif // DEBUGBUTTON
} // doubleclick2

void longPressStart_s() {
  eventbuttons[B_SHIFT] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button s longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_s() {
  eventbuttons[B_SHIFT] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button s longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_s() {
  eventbuttons[B_SHIFT] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button s longPress stop");
#endif // DEBUGBUTTON
} // longPressStop2


//------------------------------------------------------------------------------

void click_N() {
  eventbuttons[B_NORTH] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button N click.");
#endif // DEBUGBUTTON
} // click2

//void doubleclick_N() {
//  eventbuttons[B_NORTH] = E_DOUBLECLICK;
//#ifdef DEBUGBUTTON
//  Serial.println("Button N doubleclick.");
//#endif // DEBUGBUTTON
//} // doubleclick2

void longPressStart_N() {
  eventbuttons[B_NORTH] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button N longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_N() {
  eventbuttons[B_NORTH] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button N longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_N() {
  eventbuttons[B_NORTH] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button N longPress stop");
#endif // DEBUGBUTTON
} // long

//------------------------------------------------------------------------------
void click_S() {
  eventbuttons[B_SOUTH] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button S click.");
#endif // DEBUGBUTTON
} // click2

//void doubleclick_S() {
//  eventbuttons[B_SOUTH] = E_DOUBLECLICK;
//#ifdef DEBUGBUTTON
//  Serial.println("Button S doubleclick.");
//#endif // DEBUGBUTTON
//} // doubleclick2

void longPressStart_S() {
  eventbuttons[B_SOUTH] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button S longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_S() {
  eventbuttons[B_SOUTH] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button S longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_S() {
  eventbuttons[B_SOUTH] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button S longPress stop");
#endif // DEBUGBUTTON
} // long


//------------------------------------------------------------------------------
void click_E() {
  eventbuttons[B_EAST] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button E click.");
#endif // DEBUGBUTTON
} // click2

//void doubleclick_E() {
//  eventbuttons[B_EAST] = E_DOUBLECLICK;
//#ifdef DEBUGBUTTON
//  Serial.println("Button E doubleclick.");
//#endif // DEBUGBUTTON
//} // doubleclick2

void longPressStart_E() {
  eventbuttons[B_EAST] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button E longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_E() {
  eventbuttons[B_EAST] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button E longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_E() {
  eventbuttons[B_EAST] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button E longPress stop");
#endif // DEBUGBUTTON
} // long

//------------------------------------------------------------------------------
void click_W() {
  eventbuttons[B_WEST] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button W click.");
#endif // DEBUGBUTTON
} // click2

//void doubleclick_W() {
//  eventbuttons[B_WEST] = E_DOUBLECLICK;
//#ifdef DEBUGBUTTON
//  Serial.println("Button W doubleclick.");
//#endif // DEBUGBUTTON
//} // doubleclick2

void longPressStart_W() {
  eventbuttons[B_WEST] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button W longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_W() {
  eventbuttons[B_WEST] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button W longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_W() {
  eventbuttons[B_WEST] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button W longPress stop");
#endif // DEBUGBUTTON
} // long

//------------------------------------------------------------------------------
void click_F(){
  eventbuttons[B_F] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button F click.");
#endif // DEBUGBUTTON
} // click2

void doubleclick_F() {
  eventbuttons[B_F] = E_DOUBLECLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button F doubleclick.");
#endif // DEBUGBUTTON
} // doubleclick2

void longPressStart_F() {
  eventbuttons[B_F] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button F longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_F() {
  eventbuttons[B_F] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button F longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_F() {
  eventbuttons[B_F] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button F longPress stop");
#endif // DEBUGBUTTON
} // long

//------------------------------------------------------------------------------
void click_f() {
  eventbuttons[B_f] = E_CLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button f click.");
#endif // DEBUGBUTTON
} // click2

void doubleclick_f() {
  eventbuttons[B_f] = E_DOUBLECLICK;
#ifdef DEBUGBUTTON
  Serial.println("Button f doubleclick.");
#endif // DEBUGBUTTON
} // doubleclick2

void longPressStart_f() {
  eventbuttons[B_f] = E_LONGPRESSTART;
#ifdef DEBUGBUTTON
  Serial.println("Button f longPress start");
#endif // DEBUGBUTTON
} // longPressStart2

void longPress_f() {
  eventbuttons[B_f] = E_LONGPRESS;
#ifdef DEBUGBUTTON
  Serial.println("Button f longPress...");
#endif // DEBUGBUTTON
} // longPress2

void longPressStop_f() {
  eventbuttons[B_f] = E_LONGPRESSSTOP;
#ifdef DEBUGBUTTON
  Serial.println("Button f longPress stop");
#endif // DEBUGBUTTON
} // long



void SmartHandController::setupButton()
{
  m_buttons[B_SHIFT]->attachClick(click_s);
  m_buttons[B_SHIFT]->attachDoubleClick(doubleclick_s);
  m_buttons[B_SHIFT]->attachLongPressStart(longPressStart_s);
  m_buttons[B_SHIFT]->attachLongPressStop(longPressStop_s);
  m_buttons[B_SHIFT]->attachDuringLongPress(longPress_s);

  //m_buttons[B_NORTH]->attachDoubleClick(doubleclick_N);
  m_buttons[B_NORTH]->attachLongPressStart(longPressStart_N);
  m_buttons[B_NORTH]->attachLongPressStop(longPressStop_N);
  m_buttons[B_NORTH]->attachDuringLongPress(longPress_N);

  m_buttons[B_SOUTH]->attachClick(click_S);
  //m_buttons[B_SOUTH]->attachDoubleClick(doubleclick_S);
  m_buttons[B_SOUTH]->attachLongPressStart(longPressStart_S);
  m_buttons[B_SOUTH]->attachLongPressStop(longPressStop_S);
  m_buttons[B_SOUTH]->attachDuringLongPress(longPress_S);

  m_buttons[B_EAST]->attachClick(click_E);
  //m_buttons[B_EAST]->attachDoubleClick(doubleclick_E);
  m_buttons[B_EAST]->attachLongPressStart(longPressStart_E);
  m_buttons[B_EAST]->attachLongPressStop(longPressStop_E);
  m_buttons[B_EAST]->attachDuringLongPress(longPress_E);

  m_buttons[B_WEST]->attachClick(click_W);
  //m_buttons[B_WEST]->attachDoubleClick(doubleclick_W);
  m_buttons[B_WEST]->attachLongPressStart(longPressStart_W);
  m_buttons[B_WEST]->attachLongPressStop(longPressStop_W);
  m_buttons[B_WEST]->attachDuringLongPress(longPress_W);
  m_buttons[B_WEST]->attachClick(click_W);

  m_buttons[B_F]->attachClick(click_F);
  m_buttons[B_F]->attachDoubleClick(doubleclick_F);
  m_buttons[B_F]->attachLongPressStart(longPressStart_F);
  m_buttons[B_F]->attachLongPressStop(longPressStop_F);
  m_buttons[B_F]->attachDuringLongPress(longPress_F);

  m_buttons[B_f]->attachClick(click_f);
  m_buttons[B_f]->attachDoubleClick(doubleclick_f);
  m_buttons[B_f]->attachLongPressStart(longPressStart_f);
  m_buttons[B_f]->attachLongPressStop(longPressStop_f);
  m_buttons[B_f]->attachDuringLongPress(longPress_f);

}
void SmartHandController::setup(int pin[7], int active[7])
{
  for (int k = 0; k < 7; k++)
  {
    m_buttons[k] = new OneButton(pin[k], active[k]);
    m_buttons[k]->setClickTicks(300);
    m_buttons[k]->setDebounceTicks(30);
    m_buttons[k]->setPressTicks(300);
  }
  setupButton();

}
void SmartHandController::tickButtons()
{
  delay(10);
  buttonPressed = false;
  for (int k = 0; k < 7; k++)
  {
    delay(1);
    eventbuttons[k] = E_NONE;
    m_buttons[k]->tick();
  }
  for (int k = 0; k < 7; k++)
  {
    if (eventbuttons[k] != 0)
    {
      buttonPressed = true;
    }
  }
  for (int k = 1; k < 6; k += 2)
  {
    if (eventbuttons[k] == eventbuttons[k + 1])
    {
      eventbuttons[k] = E_NONE;
      eventbuttons[k + 1] = E_NONE;
    }
  }
}
void SmartHandController::update()
{
  tickButtons();
  unsigned long top = millis();
  if (buttonPressed)
  {
    if (sleepDisplay)
    {
      display.setContrast(maxContrast);
      display.sleepOff();
      sleepDisplay = false;
      lowContrast = false;
      time_last_action = millis();
    }
    if (lowContrast)
    {
      display.setContrast(maxContrast);
      lowContrast = false;
      time_last_action = top;
    }
  }
  else if (sleepDisplay)
  {
    return;
  }
  else if (top - time_last_action > 120000)
  {
    display.sleepOn();
    sleepDisplay = true;
    return;
  }
  else if (top - time_last_action > 30000 && !lowContrast)
  {
    display.setContrast(0);
    lowContrast = true;
    return;
  }
  if (powerCylceRequired)
  {
    display.setFont(u8g2_font_helvR12_tr);
    DisplayMessage("REBOOT", "DEVICE", 1000);
    return;
  }
  if (telInfo.align == Telescope::ALI_SELECT_STAR_1 || telInfo.align == Telescope::ALI_SELECT_STAR_2 || telInfo.align == Telescope::ALI_SELECT_STAR_3)
  {
    if (telInfo.align == Telescope::ALI_SELECT_STAR_1)
      DisplayLongMessage("Select a Star", "near the Meridian", "& the Celestial Equ.", "in the Western Sky", -1);
    else if (telInfo.align == Telescope::ALI_SELECT_STAR_2)
      DisplayLongMessage("Select a Star", "near the Meridian", "& the Celestial Equ.", "in the Eastern Sky", -1);
    else if (telInfo.align == Telescope::ALI_SELECT_STAR_3)
      DisplayLongMessage("Select a Star", "HA = -3 hour", "Dec = +- 45 degree", "in the Eastern Sky", -1);
    if (!SelectStarAlign())
    {
      DisplayMessage("Alignment", "Aborted", -1);
      telInfo.align = Telescope::ALI_OFF;
      return;
    }
    telInfo.align = static_cast<Telescope::AlignState>(telInfo.align + 1);
  }
  else if (top - lastpageupdate > 100)
  {
    update_main(display.getU8g2(), page);
  }
  if (telInfo.connected == false)
  {
    DisplayMessage("Hand controler", "not connected", -1);
  }
  if (telInfo.connected && (telInfo.getTrackingState() == Telescope::TRK_SLEWING || telInfo.getParkState() == Telescope::PRK_PARKING))
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
      Serial.print(":Q#");
      Serial.flush();
      time_last_action = millis();
      display.sleepOff();
      if (telInfo.align != Telescope::ALI_OFF)
      {
        telInfo.align = static_cast<Telescope::AlignState>(telInfo.align-1);
      }
      return;
    }

  }
  else
  {
    buttonCommand = false;
    for (int k = 1; k < 5; k++)
    {
      if (Move[k - 1] && (eventbuttons[k] == E_LONGPRESSSTOP || eventbuttons[k] == E_NONE))
      {
        buttonCommand = true;
        Move[k - 1] = false;
        Serial.print(BreakRC[k - 1]);
        Serial.flush();
        continue;
      }
      else if (!Move[k - 1] && (eventbuttons[k] == E_LONGPRESS || eventbuttons[k] == E_CLICK || eventbuttons[k] == E_LONGPRESSTART))
      {
        buttonCommand = true;
        Move[k - 1] = true;
        Serial.print(RC[k - 1]);
        Serial.flush();
        continue;
      }
    }
    if (buttonCommand)
    {
      time_last_action = millis();
      return;
    }

  }
  if (eventbuttons[0] == E_DOUBLECLICK /*|| eventbuttons[0] == E_CLICK)  && eventbuttons[1] != E_NONE*/)
  {
    menuSpeedRate();
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_CLICK && telInfo.align == Telescope::ALI_OFF)
  {
    page++;
    if (page > 2) page = 0;
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_LONGPRESS && telInfo.align == Telescope::ALI_OFF)
  {
    menuMain();
    time_last_action = millis();
  }
  else if (eventbuttons[0] == E_CLICK && (telInfo.align == Telescope::ALI_RECENTER_1 || telInfo.align == Telescope::ALI_RECENTER_2 || telInfo.align == Telescope::ALI_RECENTER_3))
  {
    telInfo.addStar();
  }

}
