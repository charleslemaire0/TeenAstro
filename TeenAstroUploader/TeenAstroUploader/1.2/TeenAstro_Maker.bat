REM Prepared but doesn't work
set PRJNAME=TeenAstro_1.2
set OUTPUT=.\%PRJNAME%
set BOARD=teensy:avr:teensy31:usb=serial,speed=72,opt=o2std,keys=en-us
set SKETCH=..\..\..\TeenAstroMainUnit\TeenAstroMainUnit.ino

REM ..\..\..\ArduinoCli\arduino-cli.exe config init --overwrite
set PCBV=220
set DRIVER=TMC260
set OPTIONS2=--build-property compiler.cpp.extra_flags=-DVERSION=%PCBV%
set DIRNAME=%OUTPUT%_%PCBV%_%DRIVER%
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% --output-dir %DIRNAME% %OPTIONS2% %SKETCH%
REN %DIRNAME%\TeenAstroMainUnit.ino.bin %PRJNAME%_%PCBV%_%DRIVER%.bin
REN %DIRNAME%\TeenAstroMainUnit.ino.elf %PRJNAME%_%PCBV%_%DRIVER%.elf
REN %DIRNAME%\TeenAstroMainUnit.ino.map %PRJNAME%_%PCBV%_%DRIVER%.map
move /Y %DIRNAME%\*.* .\


