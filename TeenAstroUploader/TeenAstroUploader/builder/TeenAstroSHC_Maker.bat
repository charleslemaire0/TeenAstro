set VER=1.5
set SHCNAME=TeenAstroSHC_%VER%
set OUTPUT=.\%SHCNAME%
set BOARD=esp8266:esp8266:d1_mini:xtal=80,vt=flash,eesz=4M1M,ip=lm2n,dbg=Disabled,lvl=None____,wipe=none,baud=921600
set OPTIONS=--build-property compiler.cpp.extra_flags=-DLANGUAGE
set SKETCH=..\..\..\TeenAstroSHC\TeenAstroSHC.ino

..\..\..\ArduinoCli\arduino-cli.exe config init --overwrite

set LANG=_French
del %OUTPUT%\TeenAstroSHC.ino.bin
del %OUTPUT%\TeenAstroSHC.ino.elf
del %OUTPUT%\TeenAstroSHC.ino.map
..\..\..\ArduinoCli\arduino-cli.exe compile --clean --libraries ..\..\..\libraries -b %BOARD% --output-dir %OUTPUT% %OPTIONS%=FRENCH %SKETCH%
move /Y %OUTPUT%\TeenAstroSHC.ino.bin ..\%VER%_latest\%SHCNAME%%LANG%.bin
move /Y %OUTPUT%\TeenAstroSHC.ino.elf ..\%VER%_latest\%SHCNAME%%LANG%.elf
move /Y %OUTPUT%\TeenAstroSHC.ino.map ..\%VER%_latest\%SHCNAME%%LANG%.map


set LANG=_German
del %OUTPUT%\TeenAstroSHC.ino.bin
del %OUTPUT%\TeenAstroSHC.ino.elf
del %OUTPUT%\TeenAstroSHC.ino.map
..\..\..\ArduinoCli\arduino-cli.exe compile --clean  --libraries ..\..\..\libraries -b %BOARD% --output-dir %OUTPUT% %OPTIONS%=GERMAN %SKETCH%
move /Y %OUTPUT%\TeenAstroSHC.ino.bin ..\%VER%_latest\%SHCNAME%%LANG%.bin
move /Y %OUTPUT%\TeenAstroSHC.ino.elf ..\%VER%_latest\%SHCNAME%%LANG%.elf
move /Y %OUTPUT%\TeenAstroSHC.ino.map ..\%VER%_latest\%SHCNAME%%LANG%.map


set LANG=_English
del %OUTPUT%\TeenAstroSHC.ino.bin
del %OUTPUT%\TeenAstroSHC.ino.elf
del %OUTPUT%\TeenAstroSHC.ino.map
..\..\..\ArduinoCli\arduino-cli.exe compile --clean --libraries ..\..\..\libraries -b %BOARD% --output-dir %OUTPUT% %OPTIONS%=ENGLISH %SKETCH%
move /Y %OUTPUT%\TeenAstroSHC.ino.bin ..\%VER%_latest\%SHCNAME%%LANG%.bin
move /Y %OUTPUT%\TeenAstroSHC.ino.elf ..\%VER%_latest\%SHCNAME%%LANG%.elf
move /Y %OUTPUT%\TeenAstroSHC.ino.map ..\%VER%_latest\%SHCNAME%%LANG%.map

