set OUTPUT=./TeenAstro_beta
set BOARD=teensy:avr:teensy32:speed=72
set OPTIONS=--build-properties compiler.cpp.extra_flags=-DVERSION
set SKETCH=..\..\..\TeenAstoMainUnit\TeenAstro\TeenAstro.ino
..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_240_TMC5160 %OPTIONS%=240,-DAxisDriver=3 %SKETCH%
