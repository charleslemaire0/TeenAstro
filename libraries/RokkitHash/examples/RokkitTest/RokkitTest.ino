#include <RokkitHash.h>

char data[] = "the quick brown fox jumps over the lazy dog.";

#ifdef ROKKIT_ENABLE_FLASH_FUNCTIONS
const char pdata[] PROGMEM = "the quick brown fox jumps over the lazy dog.";
#endif

#define ITERATIONS 1000

void setup () {
	uint32_t hash, len, start, stop;

	Serial.begin (9600);

	len = strlen (data);
	start = micros ();
	for (int i = 0; i < ITERATIONS; i++) {
		hash = rokkit (data, len);
	}
	stop = micros ();
	Serial.print ("RAM:\t");
	Serial.print (stop - start);
	Serial.print ("\t");
	Serial.println (hash, HEX);

#ifdef ROKKIT_ENABLE_FLASH_FUNCTIONS
	len = strlen_P (pdata);
	start = micros ();
	for (int i = 0; i < ITERATIONS; i++) {
		hash = rokkit (reinterpret_cast <const __FlashStringHelper *> (pdata), len);
	}
	stop = micros ();
	Serial.print ("Flash:\t");
	Serial.print (stop - start);
	Serial.print ("\t");
	Serial.println (hash, HEX);
#endif
}

void loop () {
}
