# Makefile for TeenAstro Smart Hand Controller (ESP8266)
#
# Uses PlatformIO for compilation.
# See platformio.ini for available environments.
#
# Examples:
#   make                 # build all languages
#   make english         # build English only
#   make clean           # clean build artefacts
#
# Requires: pip install platformio

.PHONY: all english french german clean

all: english french german

english:
	pio run -e ENGLISH

french:
	pio run -e FRENCH

german:
	pio run -e GERMAN

clean:
	pio run --target clean
