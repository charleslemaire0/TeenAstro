#pragma once
// -------------------------------------------------------------------------
//  HtmlCommon.h — shared PROGMEM HTML fragments for TeenAstroWifi pages
// -------------------------------------------------------------------------
#include <Arduino.h>

// On / Off option strings (used by multiple config pages)
const char html_optOnSel[]  PROGMEM = "<option selected value='1'>On</option>";
const char html_optOnUnsel[] PROGMEM = "<option value='1'>On</option>";
const char html_optOffSel[] PROGMEM = "<option selected value='2'>Off</option>";
const char html_optOffUnsel[] PROGMEM = "<option value='2'>Off</option>";

// Page footer
const char html_pageFooter[] PROGMEM = "</div></body></html>";
const char html_pageFooterNested[] PROGMEM = "</div></div></body></html>";
