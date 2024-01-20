set VER=1.5
set SHCNAME=TeenAstroSHC_%VER%
set OUTPUT=.\%SHCNAME%
set BOARD=esp8266:esp8266:d1_mini:xtal=80,vt=flash,eesz=4M1M,ip=lm2n,dbg=Disabled,lvl=None____,wipe=none,baud=921600
set OPTIONS=--build-property compiler.cpp.extra_flags=-DLANGUAGE
set SKETCH=..\..\..\TeenAstroSHC\TeenAstroSHC.ino

..\..\..\ArduinoCli\arduino-cli.exe config init --overwrite

set LANG=_French
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% --output-dir %OUTPUT%%LANG% %OPTIONS%=FRENCH %SKETCH%
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.bin %SHCNAME%%LANG%.bin
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.elf %SHCNAME%%LANG%.elf
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.map %SHCNAME%%LANG%.map
move /Y %OUTPUT%%LANG%\*.* ..\%VER%

set LANG=_German
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% --output-dir %OUTPUT%%LANG% %OPTIONS%=GERMAN %SKETCH%
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.bin %SHCNAME%%LANG%.bin
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.elf %SHCNAME%%LANG%.elf
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.map %SHCNAME%%LANG%.map
move /Y %OUTPUT%%LANG%\*.* ..\%VER%

set LANG=_English
..\..\..\ArduinoCli\arduino-cli.exe compile --libraries ..\..\..\libraries -v -b %BOARD% --output-dir %OUTPUT%%LANG% %OPTIONS%=ENGLISH %SKETCH%
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.bin %SHCNAME%%LANG%.bin
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.elf %SHCNAME%%LANG%.elf
REN %OUTPUT%%LANG%\TeenAstroSHC.ino.map %SHCNAME%%LANG%.map
move /Y %OUTPUT%%LANG%\*.* ..\%VER%
