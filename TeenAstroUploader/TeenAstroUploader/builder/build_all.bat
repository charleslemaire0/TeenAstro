call TeenAstroBuilder.bat -target MainUnit -driver TMC5160 -hwvers 240 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target MainUnit -driver TMC2130 -hwvers 240 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target MainUnit -driver TMC260 -hwvers 220 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target MainUnit -driver TMC260 -hwvers 230 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target SHC -local ENGLISH -release 1.2 > log.txt
call TeenAstroBuilder.bat -target SHC -local FRENCH -release 1.2 > log.txt
call TeenAstroBuilder.bat -target SHC -local GERMAN -release 1.2 > log.txt
call TeenAstroBuilder.bat -target SHC -Focuser -driver TMC2130 -hwvers 220 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target SHC -Focuser -driver TMC2130 -hwvers 230 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target SHC -Focuser -driver TMC2130 -hwvers 240 -release 1.2 -v > log.txt
call TeenAstroBuilder.bat -target SHC -Focuser -driver TMC5160 -hwvers 240 -release 1.2 -v > log.txt