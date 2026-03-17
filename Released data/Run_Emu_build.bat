@echo off
REM Run emulators-only MSI build and copy the MSI to Released data\Emulator
setlocal
set "REPO_ROOT=%~dp0.."
set "OUT_DIR=%~dp0Emulator"
set "MSI_SRC=%REPO_ROOT%\TeenAstroEmulator\installer\.out\TeenAstroEmulator.msi"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Remove existing MSI(s) in output directory
del /Q "%OUT_DIR%\*.msi" 2>nul

echo Building emulators MSI...
powershell -ExecutionPolicy Bypass -File "%REPO_ROOT%\TeenAstroEmulator\installer\build_msi.ps1" -EmulatorsOnly
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

if exist "%MSI_SRC%" (
    copy /Y "%MSI_SRC%" "%OUT_DIR%\"
    echo.
    echo MSI copied to: %OUT_DIR%
) else (
    echo MSI not found at %MSI_SRC%
    exit /b 1
)
