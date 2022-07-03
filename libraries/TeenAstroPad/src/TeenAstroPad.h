#pragma once
// Add activate wifi function // default is ON
#define ALIGN_OFF
#include <Arduino.h>
#include <OneButton.h>
#include <TeenAstroWifi.h>


enum Button { B_SHIFT, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_F, B_f };
enum ButtonEvent { E_NONE, E_CLICK, E_DOUBLECLICK, E_LONGPRESSTART, E_LONGPRESS, E_LONGPRESSSTOP };
extern volatile byte eventbuttons[7];
class Pad
{
public:
  enum ButtonSpeed { BS_SLOW, BS_MEDIUM, BS_FAST };
private:
  TeenAstroWifi m_wbt;
  bool m_buttonPressed;
  bool m_shiftPressed;
  int m_adress;
  OneButton *m_buttons[7];
  ButtonSpeed m_button_speed;
public:
  void setup(const int pin[7], const bool active[7], int adress);
  void setMenuMode();
  void setControlerMode();
  ButtonSpeed getButtonSpeed();
  void readButtonSpeed();
  void setButtonSpeed(ButtonSpeed bs);
  void attachEvent();
  void tickButtons();
  bool buttonPressed();
  bool shiftPressed();
  bool isWifiOn();
  bool isWifiRunning();
  bool turnWifiOn(bool turnOn);
  void getIP(uint8_t* ip);
  const char* getPassword();
  bool setWifiMode(int k);
  int getWifiMode();
  void getStationName(int k, char* SSID);

};