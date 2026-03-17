@echo off
REM Build TeenAstro Windows app (Flutter), create MSI, copy MSI to Released data\app
setlocal
set "REPO_ROOT=%~dp0.."
set "OUT_DIR=%~dp0app"
set "MSI_SRC=%REPO_ROOT%\teenastro_app\installer\.out\TeenAstroApp.msi"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Remove existing MSI(s) in output directory
del /Q "%OUT_DIR%\*.msi" 2>nul

echo Building TeenAstro App MSI...
powershell -ExecutionPolicy Bypass -File "%REPO_ROOT%\teenastro_app\installer\build.ps1"
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
