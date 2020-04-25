TOOLS=$(HOME)/arduino-1.8.10
ARD15=$(HOME)/.arduino15


all:
	$(TOOLS)/arduino-builder -compile -logger=machine -hardware $(TOOLS)/hardware -hardware $(ARD15)/packages \
                            -tools $(TOOLS)/tools-builder -tools $(TOOLS)/hardware/tools/avr \
                            -tools $(ARD15)/packages -built-in-libraries $(TOOLS)/libraries \
                            -libraries $(HOME)/Arduino/libraries -fqbn=teensy:avr:teensy31:usb=serial,speed=96,opt=o2std,keys=en-us -ide-version=10810 \
                            -build-path $(HOME)/Sources/build/mu -warnings=none -build-cache /tmp/arduino_cache_170105 \
							-verbose $(PWD)/TeenAstroMainUnit.ino
