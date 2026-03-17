# Build emulators-only MSI (MainUnit + SHC). Entry point for Run_Emu_build.bat.
# Run from repo root or any dir; script resolves paths. MSI output: .out\TeenAstroEmulator.msi (relative to this installer dir).

$ErrorActionPreference = "Stop"
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$BuildMsi = Join-Path $ScriptDir "build_msi.ps1"
if (-not (Test-Path $BuildMsi)) {
    Write-Host "build_msi.ps1 not found at $BuildMsi" -ForegroundColor Red
    exit 1
}
& $BuildMsi -EmulatorsOnly
exit $LASTEXITCODE
