# Makefile for TeenAstro Main Unit
#
# Uses PlatformIO for compilation.
# See platformio.ini for available environments (board/driver combinations).
#
# Examples:
#   make                     # build default env (240_5160)
#   make env-240_5160        # build one specific env
#   make env-250_2130        # build Teensy 4.0 + TMC2130
#   make all-envs            # build every env defined in platformio.ini
#   make clean               # clean all build artefacts
#
# Requires: pip install platformio   (or: curl -fsSL https://platformio.org/install.sh | sh)

.PHONY: all all-envs clean

all:
	pio run

env-%:
	pio run -e $*

all-envs:
	pio run

clean:
	pio run --target clean
