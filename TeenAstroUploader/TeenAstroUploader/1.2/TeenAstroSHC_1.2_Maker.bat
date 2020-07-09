set OUTPUT=./TeenAstroSHC_1.2
set BOARD=esp8266:esp8266:d1_mini:xtal=80,vt=flash,eesz=4M1M,ip=lm2n,dbg=Disabled,lvl=None____,wipe=none,baud=921600
set OPTIONS=--build-properties compiler.cpp.extra_flags=-DLANGUAGE
set SKETCH=..\..\..\TeenAstroSHC\TeenAstroSHC.ino
..\..\..\ArduinoCli\arduino-cli.exe config init
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% -o %OUTPUT%_French %OPTIONS%=FRENCH %SKETCH%
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% -o %OUTPUT%_German %OPTIONS%=GERMAN %SKETCH%
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% -o %OUTPUT%_English %OPTIONS%=ENGLISH %SKETCH%
