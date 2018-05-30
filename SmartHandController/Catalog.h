#pragma once
#include <Arduino.h>
#include "PGMWrap.h"

enum Catalog { STAR, MESSIER, HERSCHEL };

extern const char *constellation_txt[];
extern const char *catalog_txt[];

//Herschel
extern const char *Herschel_info_txt[];
extern const uint16_p Herschel_NGC[400] PROGMEM;
extern const uint8_p Herschel_info[400] PROGMEM;
extern const uint16_p Herschel_ra[400] PROGMEM;
extern const int16_p Herschel_dec[400] PROGMEM;
extern const uint8_p Herschel_obj[400] PROGMEM;
extern const uint8_p Hershel_constellation[400] PROGMEM;
extern const uint8_p Hershel_dMag[400] PROGMEM;

//Messier
extern const uint16_p Messier_ra[110] PROGMEM;
extern const int16_p Messier_dec[110] PROGMEM;
extern const uint8_p Messier_obj[110] PROGMEM;
extern const uint8_p Messier_constellation[110] PROGMEM;
extern const uint8_p Messier_dMag[110] PROGMEM;

//Star
extern const uint8_p Star_letter[292] PROGMEM;
extern const uint8_p Star_constellation[292] PROGMEM;
extern const uint16_p Star_ra[292] PROGMEM;
extern const int16_p Star_dec[292] PROGMEM;

void getcatdms(const short& v, short& v1, uint8_t& v2);
void getcatdf(const short& v, float& v1);
void getcathms(const unsigned short& v, uint8_t& v1, uint8_t& v2, uint8_t& v3);
void getcathf(const unsigned short& v, float& v1);

