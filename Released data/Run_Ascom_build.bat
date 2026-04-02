@echo off
REM =============================================================================
REM TeenAstro ASCOM: build Release, sign, Inno Setup, copy to Released data\driver
REM Edit PFX_PATH if needed. You will be prompted for the PFX password each run.
REM Optional: set TEENASTRO_PFX_PASSWORD in your user environment to skip the prompt.
REM Optional args pass through, e.g.  Run_Ascom_build.bat -SkipSign
REM If signing hangs or fails at [2/5] or [4/5] (timestamp server): add  -NoTimestamp
REM =============================================================================
setlocal
set "PFX_PATH=C:\Users\charl\TeenAstroCodeSigning.pfx"

cd /d "%~dp0.."
powershell -NoProfile -ExecutionPolicy Bypass -File "scripts\build_ascom_setup.ps1" ^
  -PfxPath "%PFX_PATH%" ^
  -NoAutoPasswordFile ^
  %*
set EXITCODE=%ERRORLEVEL%
endlocal & exit /b %EXITCODE%
