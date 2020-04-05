set OUTPUT=./TeenAstro_beta
set BOARD=esp8266:esp8266:d1_mini:CpuFrequency=80,VTable=flash,FlashSize=4M1M,LwIPVariant=v2mss536,Debug=Disabled,DebugLevel=None____,FlashErase=none,UploadSpeed=921600
set OPTIONS=--build-properties compiler.cpp.extra_flags=-DVERSION
set SKETCH=..\..\..\TeenAstoMainUnit\TeenAstro\TeenAstro.ino
..\..\..\ArduinoCli\arduino-cli.exe compile -v -b %BOARD% -o %OUTPUT%_240 %OPTIONS%=240 %SKETCH%
