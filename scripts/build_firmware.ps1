# Build TeenAstro firmware (MainUnit, SHC, Server, Focuser) with PlatformIO.
# Run setup_build_env.ps1 first if pio is not on PATH.
#
# Usage (from repo root or scripts\):
#   .\build_firmware.ps1                 # Build MainUnit + SHC + Server + Focuser
#   .\build_firmware.ps1 -MainUnit       # Build only MainUnit
#   .\build_firmware.ps1 -SHC            # Build only SHC
#   .\build_firmware.ps1 -MainUnit -SHC  # Build MainUnit and SHC
#   .\build_firmware.ps1 -Tests          # Run unit tests only

param(
    [switch]$MainUnit,
    [switch]$SHC,
    [switch]$Server,
    [switch]$Focuser,
    [switch]$Tests
)

$ErrorActionPreference = "Stop"

# Resolve script and repo root (same logic as setup_build_env.ps1)
$_scriptFile = $MyInvocation.MyCommand.Path
if ($_scriptFile -and (Test-Path $_scriptFile)) {
    $_scriptDir = Split-Path $_scriptFile -Parent
} else {
    $_scriptDir = $PSScriptRoot
}
$RepoRoot = if (Test-Path (Join-Path $_scriptDir "..\TeenAstroMainUnit\platformio.ini")) {
    (Resolve-Path (Join-Path $_scriptDir "..")).Path
} elseif (Test-Path (Join-Path $_scriptDir "TeenAstroMainUnit\platformio.ini")) {
    $_scriptDir
} else {
    $_scriptDir
}
if (-not (Test-Path (Join-Path $RepoRoot "TeenAstroMainUnit\platformio.ini"))) {
    Write-Host "Repo root not found. Run from repo root or scripts\." -ForegroundColor Red
    exit 1
}

# Find pio
$pioCmd = $null
if (Get-Command pio -ErrorAction SilentlyContinue) { $pioCmd = "pio" }
if (-not $pioCmd) {
    $pioExe = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
    if (Test-Path $pioExe) { $pioCmd = $pioExe }
}
if (-not $pioCmd) {
    Write-Host "pio not found. Run scripts\setup_build_env.ps1 first, then reopen PowerShell." -ForegroundColor Red
    exit 1
}

# If no target specified, build all firmware (not tests)
$buildAll = -not ($MainUnit -or $SHC -or $Server -or $Focuser -or $Tests)

$failed = $false
Push-Location $RepoRoot
try {
    if ($Tests) {
        Write-Host "`n=== pio test -d tests ===" -ForegroundColor Cyan
        & $pioCmd test -d tests
        if ($LASTEXITCODE -ne 0) { $failed = $true }
    }
    if ($buildAll -or $MainUnit) {
        Write-Host "`n=== pio run -d TeenAstroMainUnit ===" -ForegroundColor Cyan
        & $pioCmd run -d TeenAstroMainUnit
        if ($LASTEXITCODE -ne 0) { $failed = $true }
        else { Write-Host "Output: TeenAstroMainUnit\pio\" -ForegroundColor Gray }
    }
    if ($buildAll -or $SHC) {
        Write-Host "`n=== pio run -d TeenAstroSHC ===" -ForegroundColor Cyan
        & $pioCmd run -d TeenAstroSHC
        if ($LASTEXITCODE -ne 0) { $failed = $true }
        else { Write-Host "Output: TeenAstroSHC\pio\" -ForegroundColor Gray }
    }
    if ($buildAll -or $Server) {
        Write-Host "`n=== pio run -d TeenAstroServer ===" -ForegroundColor Cyan
        & $pioCmd run -d TeenAstroServer
        if ($LASTEXITCODE -ne 0) { $failed = $true }
        else { Write-Host "Output: TeenAstroServer\pio\" -ForegroundColor Gray }
    }
    if ($buildAll -or $Focuser) {
        Write-Host "`n=== pio run -d TeenAstroFocuser ===" -ForegroundColor Cyan
        & $pioCmd run -d TeenAstroFocuser
        if ($LASTEXITCODE -ne 0) { $failed = $true }
        else { Write-Host "Output: TeenAstroFocuser\pio\" -ForegroundColor Gray }
    }
} finally {
    Pop-Location
}

if ($failed) { exit 1 }
Write-Host "`nDone. Firmware is in each project's pio\ folder." -ForegroundColor Green
