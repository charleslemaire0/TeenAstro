# Makefile for Smart Hand Controller using Arduino command line interface

OUTPUT = $(HOME)/Sources/build/shc/SmartHandController
BOARD = esp8266:esp8266:d1_mini 
OPTIONS = --build-properties compiler.cpp.extra_flags=-DLANGUAGE

all:  english french german


french: 
	arduino-cli compile -v -b $(BOARD) -o $(OUTPUT)_FR $(OPTIONS)=FRENCH TeenAstroSHC.ino
	
english: 
	arduino-cli compile -v -b $(BOARD) -o $(OUTPUT)_EN $(OPTIONS)=ENGLISH TeenAstroSHC.ino

german: 
	arduino-cli compile -v -b $(BOARD) -o $(OUTPUT)_DE $(OPTIONS)=GERMAN TeenAstroSHC.ino


