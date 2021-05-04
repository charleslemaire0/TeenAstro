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
#include <TeenAstroMountStatus.h>
#include <TeenAstroWifi.h>
TeenAstroWifi m_wbt;
TeenAstroMountStatus ta_MountStatus;

void setup(void)
{
  Serial.begin(115200);
  m_wbt.setup();
}

void loop()
{
  m_wbt.update();
}