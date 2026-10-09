@echo off
REM Build TeenAstro Firmware Uploader (Python + PyInstaller), create MSI, copy to Released data\Firmware
REM Requires: Python 3.10+ with tkinter, WiX Toolset 3
REM Optional: TeenAstroUploader\TeenAstroUploader\bin\Release Teensy tools (bundled into MSI when present)
setlocal
set "REPO_ROOT=%~dp0.."
set "OUT_DIR=%~dp0Firmware"
set "MSI_SRC=%REPO_ROOT%\TeenAstroUploader\installer\.out\TeenAstroUploader.msi"
set "BUILD_PS1=%REPO_ROOT%\TeenAstroUploader\installer\build.ps1"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Remove existing MSI(s) in output directory
del /Q "%OUT_DIR%\*.msi" 2>nul

if not exist "%BUILD_PS1%" (
    echo Build script not found: %BUILD_PS1%
    exit /b 1
)

echo Building Firmware Uploader MSI (Python)...
echo   App:      %REPO_ROOT%\TeenAstroUploader\python
echo   Script:   %BUILD_PS1%
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%BUILD_PS1%"
if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

if exist "%MSI_SRC%" (
    copy /Y "%MSI_SRC%" "%OUT_DIR%\"
    echo.
    echo MSI created: %MSI_SRC%
    echo MSI copied to: %OUT_DIR%
) else (
    echo.
    echo MSI not found at %MSI_SRC%
    exit /b 1
)
