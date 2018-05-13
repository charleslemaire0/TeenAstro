#pragma once
#include <OneButton.h>

enum Button { B_SHIFT, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_F, B_f };
enum ButtonEvent { E_NONE, E_CLICK, E_DOUBLECLICK, E_LONGPRESSTART, E_LONGPRESS, E_LONGPRESSSTOP };
byte eventbuttons[7] = { E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE ,E_NONE };


class SmartHandController
{
public:
  OneButton *m_buttons[7];
  void setup(int pin[7], int active[7]);
  void tickButtons();
  void update();


private:
  void setupButton();
};
