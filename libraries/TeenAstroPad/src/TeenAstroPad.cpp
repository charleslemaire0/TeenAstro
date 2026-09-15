#include <Arduino.h>
#include <TeenAstroPad.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <driver/gpio.h>
#endif

volatile byte eventbuttons[7] = { E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE };

static void padConfigurePin(int p, bool activeLow)
{
  if (p < 0)
    return;
#if defined(ARDUINO_ARCH_ESP32)
  gpio_reset_pin((gpio_num_t)p);
  if (activeLow)
    pinMode(p, INPUT_PULLUP);     // NEWSShift: idle high, press to GND
  else
    pinMode(p, INPUT_PULLDOWN);   // F/f inverse: idle low, press to VCC
#else
  if (activeLow)
    pinMode(p, INPUT_PULLUP);
  else
    pinMode(p, INPUT);
#endif
}

#if defined(DEBUG_ON)
#define D(x)     DebugSer.print(x)
#define DH(x,y)  DebugSer.print(x,HEX)
#define DL(x)    DebugSer.println(x)
#define DHL(x,y) DebugSer.println(x,HEX)
#else
#define D(x)
#define DH(x,y)
#define DL(x)
#define DHL(x,y)
#endif

void click_s() {
  eventbuttons[B_SHIFT] = E_CLICK;
  DL("Button s click.");
} // click2

void doubleclick_s() {
  eventbuttons[B_SHIFT] = E_DOUBLECLICK;
  DL("Button s doubleclick.");
} // doubleclick2

void longPressStart_s() {
  eventbuttons[B_SHIFT] = E_LONGPRESSTART;
  DL("Button s longPress start");
} // longPressStart2

void longPress_s() {
  eventbuttons[B_SHIFT] = E_LONGPRESS;
  DL("Button s longPress...");
} // longPress2

void longPressStop_s() {
  eventbuttons[B_SHIFT] = E_LONGPRESSSTOP;
  DL("Button s longPress stop");
} // longPressStop2


//------------------------------------------------------------------------------
void click_N() {
  eventbuttons[B_NORTH] = E_CLICK;
  DL("Button N click.");
} // click2

  //void doubleclick_N() {
  //  eventbuttons[B_NORTH] = E_DOUBLECLICK;
  //#ifdef DEBUGBUTTON
  //  DL("Button N doubleclick.");
  //#endif // DEBUGBUTTON
  //} // doubleclick2

void longPressStart_N() {
  eventbuttons[B_NORTH] = E_LONGPRESSTART;
  DL("Button N longPress start");
} // longPressStart2

void longPress_N() {
  eventbuttons[B_NORTH] = E_LONGPRESS;
  DL("Button N longPress...");
} // longPress2

void longPressStop_N() {
  eventbuttons[B_NORTH] = E_LONGPRESSSTOP;
  DL("Button N longPress stop");
} // long

//------------------------------------------------------------------------------
void click_S() {
  eventbuttons[B_SOUTH] = E_CLICK;
  DL("Button S click.");
} // click2

  //void doubleclick_S() {
  //  eventbuttons[B_SOUTH] = E_DOUBLECLICK;
  //  DL("Button S doubleclick.");
  //} // doubleclick2

void longPressStart_S() {
  eventbuttons[B_SOUTH] = E_LONGPRESSTART;
  DL("Button S longPress start");
} // longPressStart2

void longPress_S() {
  eventbuttons[B_SOUTH] = E_LONGPRESS;
  DL("Button S longPress...");
} // longPress2

void longPressStop_S() {
  eventbuttons[B_SOUTH] = E_LONGPRESSSTOP;
  DL("Button S longPress stop");
} // long


//------------------------------------------------------------------------------
void click_E() {
  eventbuttons[B_EAST] = E_CLICK;
  DL("Button E click.");
} // click2

void longPressStart_E() {
  eventbuttons[B_EAST] = E_LONGPRESSTART;
  DL("Button E longPress start");
} // longPressStart2

void longPress_E() {
  eventbuttons[B_EAST] = E_LONGPRESS;
  DL("Button E longPress...");
} // longPress2

void longPressStop_E() {
  eventbuttons[B_EAST] = E_LONGPRESSSTOP;
  DL("Button E longPress stop");
} // long

//------------------------------------------------------------------------------
void click_W() {
  eventbuttons[B_WEST] = E_CLICK;
  DL("Button W click.");
} // click2

void longPressStart_W() {
  eventbuttons[B_WEST] = E_LONGPRESSTART;
  DL("Button W longPress start");
} // longPressStart2

void longPress_W() {
  eventbuttons[B_WEST] = E_LONGPRESS;
  DL("Button W longPress...");
} // longPress2

void longPressStop_W() {
  eventbuttons[B_WEST] = E_LONGPRESSSTOP;
  DL("Button W longPress stop");
} // long

//------------------------------------------------------------------------------
void click_F() {
  eventbuttons[B_F] = E_CLICK;
  DL("Button F click.");
} // click2

void doubleclick_F() {
  eventbuttons[B_F] = E_DOUBLECLICK;
  DL("Button F doubleclick.");
} // doubleclick2

void longPressStart_F() {
  eventbuttons[B_F] = E_LONGPRESSTART;
  DL("Button F longPress start");
} // longPressStart2

void longPress_F() {
  eventbuttons[B_F] = E_LONGPRESS;
  DL("Button F longPress...");
} // longPress2

void longPressStop_F() {
  eventbuttons[B_F] = E_LONGPRESSSTOP;
  DL("Button F longPress stop");
} // long

//------------------------------------------------------------------------------
void click_f() {
  eventbuttons[B_f] = E_CLICK;
  DL("Button f click.");
} // click2

void doubleclick_f() {
  eventbuttons[B_f] = E_DOUBLECLICK;
  DL("Button f doubleclick.");
} // doubleclick2

void longPressStart_f() {
  eventbuttons[B_f] = E_LONGPRESSTART;
  DL("Button f longPress start");
} // longPressStart2

void longPress_f() {
  eventbuttons[B_f] = E_LONGPRESS;
  DL("Button f longPress...");
} // longPress2

void longPressStop_f() {
  eventbuttons[B_f] = E_LONGPRESSSTOP;
  DL("Button f longPress stop");
} // long

//------------------------------------------------------------------------------
void Pad::attachEvent()
{
  m_buttons[B_SHIFT]->attachClick(click_s);
  m_buttons[B_SHIFT]->attachDoubleClick(doubleclick_s);
  m_buttons[B_SHIFT]->attachLongPressStart(longPressStart_s);
  m_buttons[B_SHIFT]->attachLongPressStop(longPressStop_s);
  m_buttons[B_SHIFT]->attachDuringLongPress(longPress_s);

  m_buttons[B_NORTH]->attachClick(click_N);
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

  m_buttons[B_F]->attachClick(click_F);
  //m_buttons[B_F]->attachDoubleClick(doubleclick_F);
  m_buttons[B_F]->attachLongPressStart(longPressStart_F);
  m_buttons[B_F]->attachLongPressStop(longPressStop_F);
  m_buttons[B_F]->attachDuringLongPress(longPress_F);

  m_buttons[B_f]->attachClick(click_f);
  //m_buttons[B_f]->attachDoubleClick(doubleclick_f);
  m_buttons[B_f]->attachLongPressStart(longPressStart_f);
  m_buttons[B_f]->attachLongPressStop(longPressStop_f);
  m_buttons[B_f]->attachDuringLongPress(longPress_f);

}
void Pad::setup(const int pin[7], const bool active[7], const int adress, const bool rotated)
{
  m_adress = adress;
  if (!rotated)
  {
    for (int k = 0; k < 7; k++)
    {
      m_activeLow[k] = active[k];
      m_buttons[k] = new OneButton(pin[k], active[k], active[k]);
    }
  }
  else
  {
    m_activeLow[0] = active[0];
    m_activeLow[1] = active[2];
    m_activeLow[2] = active[1];
    m_activeLow[3] = active[4];
    m_activeLow[4] = active[3];
    m_activeLow[5] = active[6];
    m_activeLow[6] = active[5];
    m_buttons[0] = new OneButton(pin[0], active[0], active[0]);
    m_buttons[1] = new OneButton(pin[2], active[2], active[2]);
    m_buttons[2] = new OneButton(pin[1], active[1], active[1]);
    m_buttons[3] = new OneButton(pin[4], active[4], active[4]);
    m_buttons[4] = new OneButton(pin[3], active[3], active[3]);
    m_buttons[5] = new OneButton(pin[6], active[6], active[6]);
    m_buttons[6] = new OneButton(pin[5], active[5], active[5]);
  }


  attachEvent();

  readButtonSpeed();
  setControlerMode();

  // Apply ESP32-safe biases before WiFi (active-HIGH must not float).
  for (int k = 0; k < 7; k++)
  {
    if (m_buttons[k] == nullptr)
      continue;
    padConfigurePin(m_buttons[k]->pin(), m_activeLow[k]);
  }

  m_wbt.setup();

  // WiFi bring-up can reset GPIO config; restore button input modes.
  for (int k = 0; k < 7; k++)
  {
    if (m_buttons[k] == nullptr)
      continue;
    padConfigurePin(m_buttons[k]->pin(), m_activeLow[k]);
  }
}

void Pad::tickButtons()
{
  // One short yield per poll; per-button delay(1) added ~8 ms of lag and is
  // unnecessary with RC-filtered inputs.
  delay(1);
  m_buttonPressed = false;
  m_shiftPressed = false;
  for (int k = 0; k < 7; k++)
  {
    eventbuttons[k] = E_NONE;
    m_buttons[k]->tick();
  }

  for (int k = 0; k < 7; k++)
  {
    if (eventbuttons[k] != 0)
    {
      m_buttonPressed = true;
      if (k == 0)
      {
        m_shiftPressed= true;
      }
      break;
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
  m_wbt.update();
}

Pad::ButtonSpeed Pad::getButtonSpeed()
{
 return m_button_speed;
}

void Pad::setButtonSpeed(Pad::ButtonSpeed bs)
{
  m_button_speed = bs;
  uint8_t val = static_cast<uint8_t>(m_button_speed);
  EEPROM.put(m_adress, val);
  EEPROM.commit();
}

void Pad::readButtonSpeed()
{
  uint8_t val = 1;
  EEPROM.get(m_adress, val);
  if (val == 0)
  {
    m_button_speed = BS_SLOW;
  }
  else if (val == 1)
  {
    m_button_speed = BS_MEDIUM;
  }
  else if (val == 2)
  {
    m_button_speed = BS_FAST;
  }
  else
  {
    m_button_speed = BS_MEDIUM;
    EEPROM.put(m_adress, (uint8_t)1);
    EEPROM.commit();
  }
}

// PCB buttons are RC-filtered; debounce is a fixed light residual filter.
// Slow/Medium/Fast only change menu click vs long-press feel.
static void padTimingsForSpeed(Pad::ButtonSpeed speed,
                               unsigned& debounceMs,
                               unsigned& clickMs,
                               unsigned& pressMs)
{
  debounceMs = 15;   // same for all speeds (HW RC does the heavy bounce work)
  switch (speed)
  {
  case Pad::BS_SLOW:
    // Former Medium
    clickMs = 280;
    pressMs = 550;
    break;
  case Pad::BS_MEDIUM:
    // Former Fast
    clickMs = 180;
    pressMs = 380;
    break;
  case Pad::BS_FAST:
  default:
    // New Fast — snappier than the old Fast
    clickMs = 110;
    pressMs = 250;
    break;
  }
}

void Pad::setMenuMode()
{
  unsigned debounceMs, clickMs, pressMs;
  padTimingsForSpeed(m_button_speed, debounceMs, clickMs, pressMs);
  for (int k = 0; k < 7; k++)
  {
    m_buttons[k]->setDebounceMs(debounceMs);
    m_buttons[k]->setClickMs(clickMs);
    m_buttons[k]->setPressMs(pressMs);
  }
}

void Pad::setControlerMode()
{
  unsigned debounceMs, clickMs, pressMs;
  padTimingsForSpeed(m_button_speed, debounceMs, clickMs, pressMs);
  // Shift keeps full menu timings (menus / double-click).
  m_buttons[0]->setDebounceMs(debounceMs);
  m_buttons[0]->setClickMs(clickMs);
  m_buttons[0]->setPressMs(pressMs);
  // N/S/E/W/F/f: start motion as soon as the RC-filtered edge is stable.
  for (int k = 1; k < 7; k++)
  {
    m_buttons[k]->setDebounceMs(debounceMs);
    m_buttons[k]->setClickMs(20);
    m_buttons[k]->setPressMs(40);
  }
}

bool Pad::buttonPressed()
{
  return m_buttonPressed;
}

bool Pad::shiftPressed()
{
  return m_shiftPressed;
}

bool Pad::isWifiOn()
{
  return m_wbt.isWifiOn();
}

bool Pad::isWifiRunning()
{
  return m_wbt.isWifiRunning();
}

void Pad::turnWifiOn(bool turnOn)
{
  m_wbt.turnWifiOn(turnOn);
}

void Pad::getIP(uint8_t* ip)
{
  m_wbt.getIP(ip);
}
int Pad::getWifiMode()
{
  return m_wbt.getWifiMode();
}
const char* Pad::getPassword()
{
  return m_wbt.getPassword();
}
bool Pad::setWifiMode(int k)
{
  return m_wbt.setWifiMode(k);
}
void Pad::getStationName(int k, char* SSID)
{
   m_wbt.getStationName(k, SSID);
   return;
}