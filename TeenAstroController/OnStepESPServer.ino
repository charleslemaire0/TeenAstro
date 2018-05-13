/*
 * Title       OnStepESPServer
 * by          Howard Dutton
 *
 * Copyright (C) 2017 Howard Dutton
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
 * Author: Howard Dutton
 * http://www.stellarjourney.com
 * hjd1964@gmail.com
 *
 * Description
 *
 * ESP8266-01 OnStep control
 *
 */

#include <U8x8lib.h>
#include <U8g2lib.h>
#define Product "OnEsp"
#define Version "1.0a 09 28 17"



#include "Config.h"
#include <EEPROM.h>
#include "Button.h"
#include "Telescope.h"

U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0);


bool Move[4] = { false,false,false,false };
int pin[7] = { D8,D7,D6,D0,D5,D3,D4 };
int active[7] = { 0,0,0,0,0,1,1 };
char* BreakRC[4] = { ":Qn#" ,":Qs#" ,":Qe#" ,":Qw#" };
char* RC[4] = { ":Mn#" , ":Ms#" ,":Me#" ,":Mw#" };



bool buttonCommand = false;
unsigned long lastpageupdate = millis();
unsigned long time_last_action = millis();
uint8_t maxContrast = 255;
bool wifiOn = false;
bool powerCylceRequired = false;
bool sleepDisplay = false;
bool lowContrast = false;
bool buttonPressed = false;
byte page = 0;
SmartHandController HdCrtlr;
Telescope telInfo;

void setup(void) {
  initMotor();
  HdCrtlr.setup(pin,active);
  telInfo.lastState = millis();
  display.begin();
  drawIntro();
  HdCrtlr.tickButtons();
  delay(2000);
  HdCrtlr.tickButtons();
  display.firstPage();
  uint8_t x = 0;
  do {
    display.setFont(u8g2_font_helvR14_tr);
    x = (display.getDisplayWidth() - display.getStrWidth("Loading")) / 2;
    display.drawStr(x, 25, "Loading");
    x = (display.getDisplayWidth() - display.getStrWidth("Version 0.0")) / 2;
    display.drawStr(x, 55, "Version 0.0");
  } while (display.nextPage());


  setupWifi();

  display.firstPage();
  do {
    x = (display.getDisplayWidth() - display.getStrWidth("Ready!")) / 2;
    display.drawStr(x, 40, "Ready!");
  } while (display.nextPage());
  delay(500);

}


void setupWifi()

{


#ifndef DEBUG_ON
  Serial.begin(SERIAL_BAUD_DEFAULT);
#endif
#ifdef SERIAL_SWAP_ON
  Serial.swap();
#endif

Again:
#ifdef LED_PIN
  digitalWrite(LED_PIN, LOW);
#endif
  char c = 0;

  // clear the buffers and any noise on the serial lines
  for (int i = 0; i < 3; i++) {
    Serial.print(":#");
#ifdef LED_PIN
    digitalWrite(LED_PIN, HIGH);
#endif
    delay(500);
    Serial.flush();
    //serialRecvFlush();
#ifdef LED_PIN
    digitalWrite(LED_PIN, LOW);
#endif
    delay(500);
  }

}






void loop()
{
  HdCrtlr.update();
}






