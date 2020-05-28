set OUTPUT=./TeenAstro_beta_240_TMC5160
set BOARD=teensy:avr:teensy31:speed=72
set OPTIONS=--build-properties compiler.cpp.extra_flags=-DVERSION
set OPTION2=--build-properties build.extra_flags=
rem set SKETCH=..\..\..\TeenAstoMainUnit\TeenAstroMainUnit.ino
set SKETCH=..\..\..\TeenAstroMainUnit\TeenAstroMainUnit.ino
"C:\Program Files (x86)\Arduino\arduino-cli.exe" compile -v -b %BOARD% -o %OUTPUT% %OPTIONS%=240,-DAxisDriver=3 %OPTION2% %SKETCH%
