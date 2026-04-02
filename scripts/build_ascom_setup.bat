@echo off
REM Build ASCOM driver (Release) and Inno Setup installer -> TeenAstroASCOM_V7\TeenAstro Setup 1.6.exe, Released data\driver\
REM See scripts\build_ascom_setup.ps1 for -SkipBuild and -ISCC options.

setlocal
cd /d "%~dp0.."
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_ascom_setup.ps1" %*
set EXITCODE=%ERRORLEVEL%
endlocal & exit /b %EXITCODE%
