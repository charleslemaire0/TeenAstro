/*
* Title  TeenAstro Server
*
* Copyright (C) 2021 Charles Lemaire, 
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
*
*
*/
#include <LX200Client.h>
#include <TeenAstroMountStatus.h>
#include <TeenAstroWifi.h>
#include <TeenAstroAlpaca.h>

TeenAstroWifi m_wbt;
TeenAstroMountStatus ta_MountStatus;
LX200Client lx200(Serial);
TeenAstroAlpaca m_alpaca;

void setup(void)
{
  Serial.begin(115200);
  ta_MountStatus.setClient(lx200);
  TeenAstroWifi::setClient(lx200);
  m_wbt.setup();
  // Expose the mount as an ASCOM Alpaca telescope on TCP/11111 (and the
  // standard UDP/32227 discovery port).  The configuration UI on port 80
  // is unaffected.
  m_alpaca.setup(lx200, ta_MountStatus);
}

void loop()
{
  m_wbt.update();
  m_alpaca.update();
}