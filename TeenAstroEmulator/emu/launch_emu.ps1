param(
    [switch]$NoBuild
)
<#
.SYNOPSIS
    TeenAstro PC Emulator Launcher (PowerShell)
.DESCRIPTION
    Builds and starts both MainUnit and SHC emulators as separate processes.
    The MainUnit listens on TCP ports 9997 (USB/ASCOM) and 9998 (SHC link).
    The SHC connects to port 9998 and opens an SDL2 window.
.PARAMETER NoBuild
    Skip the PlatformIO build step and run existing executables.
.EXAMPLE
    .\launch_emu.ps1
    .\launch_emu.ps1 -NoBuild
#>

$ErrorActionPreference = "Stop"
$EmuDir   = Split-Path -Parent $PSScriptRoot
$MuExe    = Join-Path $EmuDir ".pio\build\emu_mainunit\program.exe"
$ShcExe   = Join-Path $EmuDir ".pio\build\emu_shc\program.exe"
$Sdl2Dll  = "C:\SDL2\bin\SDL2.dll"

# Kill leftovers
Get-Process -Name "program" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

if (-not $NoBuild) {
    Write-Host "[1/3] Building MainUnit emulator..." -ForegroundColor Cyan
    & pio run -d $EmuDir -e emu_mainunit
    if ($LASTEXITCODE -ne 0) { throw "MainUnit build failed." }

    Write-Host "[2/3] Building SHC emulator..." -ForegroundColor Cyan
    & pio run -d $EmuDir -e emu_shc
    if ($LASTEXITCODE -ne 0) { throw "SHC build failed." }
}

foreach ($env in @("emu_mainunit", "emu_shc")) {
    $dir = Join-Path $EmuDir ".pio\build\$env"
    if (-not (Test-Path (Join-Path $dir "SDL2.dll"))) {
        if (Test-Path $Sdl2Dll) {
            Copy-Item $Sdl2Dll $dir -Force
        } else {
            Write-Warning "SDL2.dll not found at $Sdl2Dll. $env may fail to start."
        }
    }
}

Write-Host ""
Write-Host "[3/3] Starting emulators..." -ForegroundColor Cyan
Write-Host ""
Write-Host "  MainUnit:  TCP 127.0.0.1:9997 (USB)  /  127.0.0.1:9998 (SHC)"
Write-Host "  SHC:       connects to 127.0.0.1:9998"
Write-Host ""
Write-Host "  SHC keyboard controls:" -ForegroundColor Yellow
Write-Host "    Space = Shift,  W/Up = North,  S/Down = South"
Write-Host "    A/Left = West,  D/Right = East"
Write-Host "    F = Fast button,  G = Slow button"
Write-Host ""

$muProc = Start-Process -FilePath $MuExe -PassThru -WindowStyle Normal
Start-Sleep -Seconds 2
$shcProc = Start-Process -FilePath $ShcExe -PassThru -WindowStyle Normal

Write-Host "MainUnit PID: $($muProc.Id),  SHC PID: $($shcProc.Id)" -ForegroundColor Green
Write-Host "Press Enter to stop both emulators..."
$null = Read-Host

$shcProc  | Stop-Process -Force -ErrorAction SilentlyContinue
$muProc   | Stop-Process -Force -ErrorAction SilentlyContinue
Write-Host "Emulators stopped." -ForegroundColor Cyan
