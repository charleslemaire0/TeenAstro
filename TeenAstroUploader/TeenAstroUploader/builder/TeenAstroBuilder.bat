@echo off
rem usage : TeenAstroBuilder -target MainUnit|SHC|Focuser -driver TMC2130|TMC5160|TMC2160 -hwvers 240 -local FRENCH|GERMAN|ENGLISH -release x.y -v

setlocal enableDelayedExpansion


set target=
set driver=
set hwvers=
set local=
set release=
set verbose=n
set AxisDriver=


@rem SET man1=%1
@rem SET man2=%2

rem SHIFT & SHIFT


rem
rem Loop until we found the last option or flag:
rem
:moreOptionsOrFlags


rem
rem Options and flags start with a hyphen.
rem We assing the currently examined parameter (%1) to curArg because
rem the ~x,y construct is not possible on %n (n=0 … 9) variables
rem
    set  curArg=%1

rem
rem Assign first character to curArg1stChar
rem
    set  curArg1stChar=!curArg:~0,1!

    if [!curArg1stChar!] == [-] (

rem
rem    The argument starts with a hyphen. Now check
rem    for options or flags and assign them to their
rem    respective variables
rem
       if /i [!curArg!] == [-target]  (
           if not [%2] == [] (
				set target=%~2
				if /i [!target!] NEQ [MainUnit] ( 
					if /i [!target!] NEQ [SHC] (
						if /i [!target!] NEQ [Focuser] (
							echo target must be MainUnit or SHC or Focuser
							exit /b
						)
					)
				) 
				
				shift & shift
           ) else (
             echo No value specified for !curArg!
             exit /b
           )

       ) else if /i [!curArg!] == [-driver] (

           if not [%2] == [] (
              set driver=%~2
				if /i [!driver!] == [TMC2130] ( 
				  set AxisDriver=2
				) else if /i [!driver!] == [TMC5160] (
				  set AxisDriver=3
				) else if /i [!driver!] == [TMC2160] (
				  set AxisDriver=4
				) else (
				  echo driver must be  TMC2130, TMC5160, or TMC2160
				  exit /b		
				) 
              shift & shift
           ) else (
             echo No value specified for !curArg!
             exit /b
           )

       ) else if /i [!curArg!] == [-hwvers] (

           if not [%2] == [] (
              set hwvers=%~2
				if /i [!hwvers!] NEQ [240] ( 
					if /i [!hwvers!] NEQ [230] (
						if /i [!hwvers!] NEQ [220] (
							echo hwvers must be  220, 230, or 240
							exit /b
						)
					)
				) 
              shift & shift
           ) else (
             echo No value specified for !curArg!
             exit /b
           )
       ) else if /i [!curArg!] == [-local] (

           if not [%2] == [] (
              set local=%~2
				if /i [!local!] NEQ [FRENCH] ( 
					if /i [!local!] NEQ [GERMAN] (
						if /i [!local!] NEQ [ENGLISH] (
							echo local must be  FRENCH, GERMAN, or ENGLISH
							exit /b
						)
					)
				) 
              shift & shift
           ) else (
             echo No value specified for !curArg!
             exit /b
           )

       ) else if /i [!curArg!] == [-release] (
           if not [%2] == [] (
              set release=%~2

            shift & shift
			) else (
             echo No value specified for !curArg!
             exit /b
           )			

       ) else if /i [!curArg!] == [-v] (

            set verbose=y
            shift

       ) else (

         echo Unexpected option or flag !curArg!
         exit /b

       )

rem
rem    We still might have more flags or options to process.
rem    So jump back to the loop and check again
rem
       goto :moreOptionsOrFlags
    )

	
rem
rem    No more flags or options.
rem
rem    Check if the required amount of parameters is given to the script
rem
if /i [!target!] == [MainUnit] ( 
	if /i [!Driver!] == [] (
		echo !target! require Driver and Hardware parameters
		exit /b
	)
	if /i [!hwvers!] == [] (
		echo !target! require Driver and Hardware parameters
		exit /b
	)	
) 

if /i [!target!] == [SHC] ( 
	if /i [!local!] == [] (
		echo !target! require local parameter English asumed
		set local=ENGLISH
	)
) 

if /i [!target!] == [Focuser] ( 
	if /i [!driver!] == [] (
		echo !target! require Driver and Hardware parameters
		exit /b
	)
	if /i [!hwvers!] == [] (
		echo !target! require Driver and Hardware parameters
		exit /b
	)	
) 


rem
rem  Print values of variables
rem
echo(
echo Target  :  !target!
echo Driver  :  !driver!
echo Hardware:  !hwvers!
echo Release :  !release!
echo Locale  :  !local!
echo(
echo Verbose:     !verbose!
echo(


REM Arduino installation Path
set Arduino_Root_Path=%ProgramFiles(x86)%\Arduino
REM Arduino Hardware database
set HARDW_PATH1=%Arduino_Root_Path%\hardware
REM Arduino Hardware packages
set HARDW_PATH2=%LOCALAPPDATA%\Arduino15\packages
REM Arduino Tools 
set TOOLS_PATH1=%Arduino_Root_Path%\tools-builder
REM Arduino Tools for avr
set TOOLS_PATH2=%Arduino_Root_Path%\hardware\tools\avr
REM Arduino Tools in local appdata
set TOOLS_PATH3=%LOCALAPPDATA%\Arduino15\packages
REM Arduino libraries (core)
set BI_LIB=%Arduino_Root_Path%\libraries
REM Our application libraries
set EXT_LIB=..\..\..\libraries

REM Arduino IDE Version ID 
set IDE_VERSION=10812
REM Target Build Folder
set BUILD_PATH=.\!target!_Build
Rem Cache for core libraries
set BUILD_CACHE=.\!target!Build_Cache
rem Release Folder
set RELEASE_PATH=..\!release!

if not exist !BUILD_PATH! mkdir !BUILD_PATH!
if not exist !BUILD_CACHE! mkdir !BUILD_CACHE!
if not exist !RELEASE_PATH! mkdir !RELEASE_PATH!

rem
rem    No more flags or options.
rem
rem    Process Now
rem

if /i [!target!] == [MainUnit] ( 
  set SKETCH=..\..\..\TeenAstroMainUnit\TeenAstroMainUnit.ino
  set OPTION1=-prefs=compiler.cpp.extra_flags=-DVERSION=!hwvers! -DAxisDriver=!AxisDriver!
  set OPTION2=-prefs=build.extra_flags= 
  set TOOLS_PATH2=!Arduino_Root_Path!\hardware\tools\avr
  set BOARD="teensy:avr:teensy31:usb=serial,speed=72,opt=o2std,keys=en-us"
  set Target_File=TeenAstro_!release!_!hwvers!_!driver!
  set Buid_File=TeenAstroMainUnit.ino
  goto compil_teensy
) 

if /i [!target!] == [SHC] ( 
  set SKETCH=..\..\..\TeenAstroSHC\TeenAstroSHC.ino
  set OPTION1=-prefs=compiler.cpp.extra_flags=-DLANGUAGE=!local!
rem  set OPTION2=-prefs=build.extra_flags=
  set OPTION2=
  set TOOLS_PATH2=!Arduino_Root_Path!\hardware\tools\esp8266
  set BOARD=esp8266:esp8266:d1_mini:xtal=80,vt=flash,eesz=4M1M,ip=lm2n,dbg=Disabled,lvl=None____,wipe=none,baud=921600
  if /i [!local!] == [FRENCH] (
  set OUTPUT=!RELEASE_PATH!/TeenAstroSHC_!release!_French
  ) else if /i [!local!] == [GERMAN] (
  set OUTPUT=!RELEASE_PATH!/TeenAstroSHC_!release!_German
  )	else if /i [!local!] == [ENGLISH] (
  set OUTPUT=!RELEASE_PATH!/TeenAstroSHC_!release!_English
  ) else (
		echo local must be  FRENCH, GERMAN, or ENGLISH
		exit /b
  ) 

  set OPTIONS=--build-properties compiler.cpp.extra_flags=-DLANGUAGE=!local!
  set SKETCH=..\..\..\TeenAstroSHC\TeenAstroSHC.ino
  goto compil_wemos_D1_mini
) 

if /i [!target!] == [Focuser] ( 
  set SKETCH=..\..\..\TeenAstroFocuser\TeenAstroFocuser.ino
  echo Focuser not yet   TBD
  exit /b
) 


:compil_teensy
set verb

if /i !verbose!==[y] ( 
  set verb="-verbose"
  echo on 
  )

arduino-builder.exe -dump-prefs -logger=human -warnings=none !verb! -hardware "%HARDW_PATH1%" -hardware "%HARDW_PATH2%" -tools "%TOOLS_PATH1%" -tools "%TOOLS_PATH2%" -tools "%TOOLS_PATH3%" -built-in-libraries "%BI_LIB%" -libraries "%EXT_LIB%" -fqbn "%BOARD%" -ide-version="%IDE_VERSION%" -build-path "%BUILD_PATH%" -build-cache "%BUILD_CACHE%" "%OPTION1%" "%OPTION2%" "%SKETCH%" 
arduino-builder.exe -compile -logger=human -warnings=none !verb! -hardware "%HARDW_PATH1%" -hardware "%HARDW_PATH2%" -tools "%TOOLS_PATH1%" -tools "%TOOLS_PATH2%" -tools "%TOOLS_PATH3%" -built-in-libraries "%BI_LIB%" -libraries "%EXT_LIB%" -fqbn "!BOARD!" -ide-version="%IDE_VERSION%" -build-path "%BUILD_PATH%" -build-cache "%BUILD_CACHE%" "%OPTION1%" "%OPTION2%" "%SKETCH%" 

if %ERRORLEVEL% NEQ 0 exit /b

copy /Y !BUILD_PATH!\!Buid_File!.elf !RELEASE_PATH!\!Target_File!.elf
copy /Y !BUILD_PATH!\!Buid_File!.hex !RELEASE_PATH!\!Target_File!.hex

exit /b

:compil_wemos_D1_mini
if /i !verbose!==[y] echo on

..\..\..\ArduinoCli\arduino-cli.exe compile -v -b !BOARD! -o !OUTPUT! !OPTIONS!=!local! !SKETCH!

exit /b

