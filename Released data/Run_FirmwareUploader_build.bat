@echo off
REM Build Firmware Uploader (MSBuild), create MSI with all required files, copy MSI to Released data\Firmware
setlocal
set "REPO_ROOT=%~dp0.."
set "OUT_DIR=%~dp0Firmware"
set "MSI_SRC=%REPO_ROOT%\TeenAstroEmulator\installer\.out\TeenAstroUploader.msi"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Remove existing MSI(s) in output directory
del /Q "%OUT_DIR%\*.msi" 2>nul

echo Building Firmware Uploader MSI...
powershell -ExecutionPolicy Bypass -File "%REPO_ROOT%\TeenAstroEmulator\installer\build_msi.ps1" -UploaderOnly
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
