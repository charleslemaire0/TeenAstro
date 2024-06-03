@echo off
rem usage : TeenAstroBuilder -target MainUnit|Focuser -driver TMC260|TMC2130|TMC5160|TMC2660 -hwvers 240 -release x.y -v

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
				) else if /i [!driver!] == [TMC2660] (
				  set AxisDriver=4
				) else if /i [!driver!] == [TMC260] (
				  set AxisDriver=1
				) else (
				  echo driver must be  TMC2130, TMC5160, or TMC2660
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
				if /i [!hwvers!] NEQ [260] ( 
		  		if /i [!hwvers!] NEQ [250] ( 
            if /i [!hwvers!] NEQ [240] ( 
              if /i [!hwvers!] NEQ [230] (
                if /i [!hwvers!] NEQ [220] (
                  echo hwvers must be  220, 230, 240, 250 or 260
                  exit /b
                )
              )
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
set IDE_VERSION=10819
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
if /i [!hwvers!] == [220]  ( 
  set BOARD="teensy:avr:teensy31:usb=serial,speed=96,opt=o2std,keys=en-us"
  set OPTION=--build-property "compiler.cpp.extra_flags= -DVERSION=!hwvers! -DAxisDriver=!AxisDriver!"
) else if /i [!hwvers!] == [230]  ( 
  set BOARD="teensy:avr:teensy31:usb=serial,speed=96,opt=o2std,keys=en-us"
  set OPTION=--build-property "compiler.cpp.extra_flags=-DVERSION=!hwvers! -DAxisDriver=!AxisDriver!"
) else if /i [!hwvers!] == [240]  ( 
  set BOARD="teensy:avr:teensy31:usb=serial,speed=96,opt=o2std,keys=en-us"
  set OPTION=--build-property "compiler.cpp.extra_flags=-DVERSION=!hwvers! -DAxisDriver=!AxisDriver!"
) else if /i [!hwvers!] == [250]  ( 
  set BOARD="teensy:avr:teensy40:usb=serial,speed=450,opt=o2std,keys=en-us"
  set OPTION=--build-property "compiler.cpp.extra_flags=-DVERSION=!hwvers! -DAxisDriver=!AxisDriver!"
) else if /i [!hwvers!] == [260]  ( 
  set BOARD="teensy:avr:teensyMM:usb=serial,speed=450,opt=o2std,keys=en-us"
  set OPTION=--build-property "compiler.cpp.extra_flags=-DVERSION=!hwvers! -DAxisDriver=!AxisDriver!"
)

if /i [!target!] == [MainUnit] ( 
  set SKETCH=..\..\..\TeenAstroMainUnit\TeenAstroMainUnit.ino

  set TOOLS_PATH2=!Arduino_Root_Path!\hardware\tools\avr
  set Target_File=TeenAstro_!release!_!hwvers!_!driver!
  set Buid_File=TeenAstroMainUnit.ino
  goto compil_teensy
) 

if /i [!target!] == [Focuser] ( 
  set SKETCH=..\..\..\TeenAstroFocuser\TeenAstroFocuser.ino
  set TOOLS_PATH2=!Arduino_Root_Path!\hardware\tools\avr
  set Target_File=TeenAstroFocuser_!release!_!hwvers!_!driver!
  set Buid_File=\TeenAstroFocuser.ino
  goto compil_teensy
) 


:compil_teensy
set verb

if /i !verbose!==[y] ( 
  set verb="--verbose"
  echo on 
  )
echo on
..\..\..\ArduinoCli\arduino-cli.exe compile --clean --libraries ..\..\..\libraries %verb% -b %BOARD% --output-dir %BUILD_PATH% %OPTION% %SKETCH%
echo on

move /Y !BUILD_PATH!\!Buid_File!.elf !RELEASE_PATH!_latest\!Target_File!.elf
move /Y !BUILD_PATH!\!Buid_File!.hex !RELEASE_PATH!_latest\!Target_File!.hex

exit /b

