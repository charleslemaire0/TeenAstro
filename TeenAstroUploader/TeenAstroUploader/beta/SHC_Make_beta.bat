set OUTPUT=./TeenAstroSHC_beta
set BOARD=esp8266:esp8266:d1_mini
set OPTIONS=--build-properties compiler.cpp.extra_flags=-DLANGUAGE
set SKETCH=..\..\..\SmartHandController\SmartHandController.ino
..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_French %OPTIONS%=FRENCH %SKETCH%
..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_German %OPTIONS%=GERMAN %SKETCH%
..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_English %OPTIONS%=ENGLISH %SKETCH%