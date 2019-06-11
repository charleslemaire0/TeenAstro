/*
* Title       TeenAstro Smart Hand Controller
*
* Copyright (C) 2019 Charles Lemaire, Howard Dutton
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
* Author: Charles Lemaire, https://pixelstelescopes.wordpress.com/teenastro/
* Author: Howard Dutton, http://www.stellarjourney.com, hjd1964@gmail.com
*
*/


#define Product "Teenastro SHC"
#define SHCFirmwareDate          __DATE__
#define SHCFirmwareTime          __TIME__
#define SHCFirmwareVersionMajor  "1"
#define SHCFirmwareVersionMinor  "2"
#define SHCFirmwareVersionPatch  "b"

#include "Config.h"
#include "SmartController.h"

const char SHCVersion[] = SHCFirmwareVersionMajor "." SHCFirmwareVersionMinor "." SHCFirmwareVersionPatch;
const int pin[7] = { B_PIN0,B_PIN1,B_PIN2,B_PIN3,B_PIN4,B_PIN5,B_PIN6 };
const bool active[7] = { B_PIN_UP_0,B_PIN_UP_1,B_PIN_UP_2,B_PIN_UP_3,B_PIN_UP_4,B_PIN_UP_5,B_PIN_UP_6 };

SmartHandController HdCrtlr;
TeenAstroMountStatus ta_MountStatus;

void setup(void)
{
  int value = analogRead(A0);
  
  if (value< 191)       //0.616129032V
    HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SH1106);
  else if (value < 319) //1.029032258V
  {
    //empty
  }
  else if (value < 447) //1.441935484V
  {
    //empty
  }
  else if (value < 575) //1.85483871V
  {
    HdCrtlr.setup(SHCVersion, pin, active, SERIAL_BAUD, SmartHandController::OLED::OLED_SSD1309);
  }
  else if (value < 703) //2.267741935V
  {
    //empty
  }
  else if (value < 831) //2.680645161V
  {
    //empty
  }
  else
  {
    //empty
  }
    
}

void loop()
{
  HdCrtlr.update();
}

