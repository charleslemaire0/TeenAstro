UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
TOOLS=$(HOME)/arduino-1.8.12
LIB=$(HOME)/Arduino/libraries
ARD15=$(HOME)/.arduino15
endif

ifeq ($(UNAME), Darwin)
TOOLS=/Applications/Arduino.app/Contents/Java
LIB=$(HOME)/Documents/Arduino/libraries
ARD15=$(HOME)/Library/Arduino15
endif

OPTIONS = --build-properties compiler.cpp.extra_flags=-DLANGUAGE



all:
	$(TOOLS)/arduino-builder -compile -logger=machine -hardware $(TOOLS)/hardware -hardware $(ARD15)/packages \
                            -tools $(TOOLS)/tools-builder -tools $(TOOLS)/hardware/tools/avr \
                            -tools $(ARD15)/packages -built-in-libraries $(TOOLS)/libraries \
                            -libraries $(LIB) -fqbn=teensy:avr:teensy31:usb=serial,speed=96,opt=o2std,keys=en-us -ide-version=10810 \
                            -build-path $(HOME)/Sources/build/mu -warnings=none -build-cache /tmp/arduino_cache_170105 \
							-verbose $(PWD)/TeenAstroMainUnit.ino
