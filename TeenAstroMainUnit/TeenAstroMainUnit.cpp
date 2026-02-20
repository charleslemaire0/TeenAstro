/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2024 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvalee
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
 * Revision History, see GitHub
 *
 * Author: Howard Dutton, Charles Lemaire
 *
 * Description: Arduino Stepper motor controller for Telescope mounts
 * with LX200 derived command set.
 * Main entry (setup/loop) - C++ build.
 */
#include "Command.h"
#include "Application.h"

void reboot()
{
  Serial.end();
  Serial1.end();
  Focus_Serial.end();
  GNSS_Serial.end();
  delay(1000);
#if defined(ARDUINO_TEENSY40)  || defined(ARDUINO_TEENSY_MICROMOD) || defined(ARDUINO_TEENSY32)
#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);
  CPU_RESTART;
#endif
}

void setup()
{
  application.setup();
}

void loop()
{
  application.loop();
}
