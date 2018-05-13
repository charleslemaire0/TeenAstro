#pragma once
#include <Arduino.h>

enum Catalog { STAR, MESSIER, HERSCHEL };

extern const char *constellation_txt[];
extern const char *catalog_txt[];

//Herschel
extern const char *Herschel_info_txt[];
extern const unsigned short Herschel_NGC[400];
extern const byte Herschel_info[400];
extern const unsigned short Herschel_ra[400];
extern const short Herschel_dec[400];
extern const byte Herschel_obj[400];
extern const byte Hershel_constellation[400];
extern const byte Hershel_dMag[400];

//Star
extern const byte Star_letter[292];
extern const byte Star_constellation[292];
extern const unsigned short Star_ra[292];
extern const short Star_dec[292];



void getcatdms(const short& v, short& v1, uint8_t& v2);
void getcatdf(const short& v, float& v1);
void getcathms(const unsigned short& v, uint8_t& v1, uint8_t& v2, uint8_t& v3);
void getcathf(const short& v, float& v1);