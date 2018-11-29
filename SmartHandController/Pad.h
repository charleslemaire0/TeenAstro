#pragma once
// Add activate wifi function // default is ON
#define WIFI_ON
#define ALIGN_OFF
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
  bool m_shiftPressed;
  OneButton *m_buttons[7];
public:
  void setup(const int pin[7], const bool active[7]);
  void setMenuMode();
  void setControlerMode();
  void attachEvent();
  void tickButtons();
  bool buttonPressed();
  bool shiftPressed();
#ifdef WIFI_ON
  bool isWifiOn();
  bool isWifiRunning();
  bool turnWifiOn(bool turnOn);
  void getIP(uint8_t* ip);
  bool setWifiMode(int k);
  int getWifiMode();
  void getStationName(int k, char* SSID);
#endif
};