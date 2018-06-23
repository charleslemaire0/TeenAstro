#pragma once
// Add activate wifi function // default is ON
#define WIFI_ON
#include <Arduino.h>
#include <OneButton.h>
#ifdef WIFI_ON
#include <WifiBluetooth.h>
#endif // 

enum Button { B_SHIFT, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_F, B_f };
enum ButtonEvent { E_NONE, E_CLICK, E_DOUBLECLICK, E_LONGPRESSTART, E_LONGPRESS, E_LONGPRESSSTOP };
extern byte eventbuttons[7];
class Pad
{
#ifdef WIFI_ON
  wifibluetooth m_wbt;
#endif
  bool m_buttonPressed;
  OneButton *m_buttons[7];
public:
  void setup(const int pin[7], const bool active[7]);
  void attachEvent();
  void tickButtons();
  bool buttonPressed();
#ifdef WIFI_ON
  bool isWifiOn();
  bool turnWifiOn(bool turnOn);
#endif
};