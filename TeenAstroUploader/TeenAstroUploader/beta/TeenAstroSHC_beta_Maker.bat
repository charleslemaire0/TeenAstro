set OUTPUT=./TeenAstroSHC_beta
set BOARD=esp8266:esp8266:d1_mini:CpuFrequency=80,VTable=flash,FlashSize=4M1M,LwIPVariant=v2mss536,Debug=Disabled,DebugLevel=None____,FlashErase=none,UploadSpeed=921600
set OPTIONS=--build-properties compiler.cpp.extra_flags=-DLANGUAGE
set SKETCH=..\..\..\SmartHandController\SmartHandController.ino
REM ..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_French %OPTIONS%=FRENCH %SKETCH%
REM ..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_German %OPTIONS%=GERMAN %SKETCH%
..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_English %OPTIONS%=ENGLISH %SKETCH%