/*
* Title  TeenAstro Smart Hand Controller
*
* Copyright (C) 2020 Charles Lemaire, Howard Dutton
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*
*
* Revision History, see GitHub
*
*
* Author: Charles Lemaire, https://groups.io/g/TeenAstro
* Author: Howard Dutton
*
*/
#include "SmartConfig.h"
#include "SmartController.h"
#include <TeenAstroMountStatus.h>

const char SHCVersion[] = SHCFirmwareVersionMajor "." SHCFirmwareVersionMinor "." SHCFirmwareVersionPatch;
const int pin[7] = { B_PIN0,B_PIN1,B_PIN2,B_PIN3,B_PIN4,B_PIN5,B_PIN6 };
const bool active[7] = { B_PIN_UP_0,B_PIN_UP_1,B_PIN_UP_2,B_PIN_UP_3,B_PIN_UP_4,B_PIN_UP_5,B_PIN_UP_6 };

LX200Client lx200(Ser);
SmartHandController HdCrtlr;
TeenAstroMountStatus ta_MountStatus;

// MainUnit-style startup blink on boards with an onboard RGB LED (e.g. LOLIN S3 Mini).
static void shcStartupLedBlink()
{
#if defined(RGB_BUILTIN)
  for (int k = 0; k < 20; k++)
  {
    neopixelWrite(RGB_BUILTIN, 0, 100, 0);
    delay(10);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    delay(50);
  }
#endif
}

void setup(void)
{
  shcStartupLedBlink();
  ta_MountStatus.setClient(lx200);
  TeenAstroWifi::setClient(lx200);
  HdCrtlr.setClient(lx200);
#ifdef SHC_TDISPLAY_S3
  // Color TFT via U8g2 blit; OLED model arg unused.
  HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SSD1306, 1);
  return;
#else
#ifdef ARDUINO_TTGO_LoRa32_V1
  HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SSD1309, 2);
  return;
#else
#ifdef ARDUINO_LOLIN_S3_MINI
  // A0 shares the D1 socket on the S3 Mini; auto-detect is unreliable.
  // 1.3" 128x64 modules on the SHC use SH1106 (same as Wemos A0 < 200).
  // Submodel 0 = noname, 1 = winstar (Settings > Display > Submodel).
  HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SH1106, 2);
#else
#ifdef ARDUINO_ARCH_ESP32
  analogReadResolution(10);   // match Wemos 10-bit OLED-select thresholds
#endif
  int value = analogRead(A_SCREEN);
  if (value < 200)       //0.616129032V
  {
    HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SH1106, 1);
  }
  else if (value < 400) 
  {
    HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SSD1306, 1);
  }
  else
  {
    HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SSD1309, 2);
  }
#endif
#endif
#endif
}

void loop()
{
  HdCrtlr.update();
}

