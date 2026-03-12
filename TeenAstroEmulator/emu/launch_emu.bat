@echo off
REM ============================================================
REM  TeenAstro PC Emulator Launcher
REM  Builds and starts both MainUnit and SHC emulators.
REM
REM  Prerequisites:
REM    - PlatformIO CLI (pio) on PATH
REM    - SDL2 installed at C:\SDL2  (bin\SDL2.dll, lib\, include\)
REM
REM  Usage:  launch_emu.bat [--no-build]
REM ============================================================

setlocal
set EMU_DIR=%~dp0..
set MU_EXE=%EMU_DIR%\.pio\build\emu_mainunit\program.exe
set SHC_EXE=%EMU_DIR%\.pio\build\emu_shc\program.exe
set SDL2_DLL=C:\SDL2\bin\SDL2.dll

REM Kill any previous emulator instances
taskkill /f /im program.exe >nul 2>&1

if "%1"=="--no-build" goto :skip_build

echo [1/3] Building MainUnit emulator...
pushd "%EMU_DIR%"
pio run -e emu_mainunit
if errorlevel 1 (
    echo ERROR: MainUnit build failed.
    popd
    exit /b 1
)

echo [2/3] Building SHC emulator...
pio run -e emu_shc
if errorlevel 1 (
    echo ERROR: SHC build failed.
    popd
    exit /b 1
)
popd

:skip_build

REM Ensure SDL2.dll is next to both executables
for %%D in (emu_mainunit emu_shc) do (
    if not exist "%EMU_DIR%\.pio\build\%%D\SDL2.dll" (
        if exist "%SDL2_DLL%" (
            copy "%SDL2_DLL%" "%EMU_DIR%\.pio\build\%%D\" >nul
        ) else (
            echo WARNING: SDL2.dll not found at %SDL2_DLL%. %%D may fail to start.
        )
    )
)

echo [3/3] Starting emulators...
echo.
echo   MainUnit:  TCP 127.0.0.1:9997 (USB)  /  127.0.0.1:9998 (SHC)
echo   SHC:       connects to 127.0.0.1:9998
echo.
echo   SHC keyboard controls:
echo     Space = Shift,  W/Up = North,  S/Down = South
echo     A/Left = West,  D/Right = East
echo     F = Fast button,  G = Slow button
echo.
echo   Press Ctrl+C in this window to stop both processes.
echo.

start "TeenAstro MainUnit" "%MU_EXE%"
timeout /t 2 /nobreak >nul
start "TeenAstro SHC" "%SHC_EXE%"

echo Both emulators launched. Close this window or press Ctrl+C to stop.
pause
taskkill /f /im program.exe >nul 2>&1
