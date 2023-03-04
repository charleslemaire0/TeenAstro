#include <RokkitHash.h>
#include <OneWire.h>

OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)
unsigned int wire_devices[256]; // store device IDs (up to 256 addresses/hashes)

void setup(void) {
	byte k,i=0;
	char hexchar[3];
	byte addr[8];
	while (ds.search((uint8_t*)addr)) {
		if (OneWire::crc8(addr,7)!=addr[7]){
			Serial.println("checksum err");
			// do some reset stuff here
			while(true) {
				delay(10);
			}
		}
		wire_devices[i] = rokkit((const char*)addr,8);
		Serial.print("Address: ");
		for (k=0;k<8;k++) {
			sprintf(hexchar,"%02x\0",addr[k]);
			Serial.print(hexchar);
		}
		Serial.println(".");
		Serial.print("Address hash: ");
		Serial.println(wire_devices[i],HEX);
		i++;
    }
}

void loop(void) {
	delay(10);
}