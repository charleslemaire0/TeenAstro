@echo off
REM Wrapper to sign the TeenAstro ASCOM driver setup EXE.
REM Calls the PowerShell script sign-teenastro-setup.ps1 from the repo root.

setlocal

REM Determine repo root (folder containing this BAT file)
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Call the PowerShell signing script
powershell -ExecutionPolicy Bypass -File ".\sign_ASCOM_driver.ps1"

endlocal

