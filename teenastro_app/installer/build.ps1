# Build TeenAstro App MSI (Flutter Windows). Entry point for Run_App_build.bat.
# Run from repo root or any dir; script resolves paths. MSI output: TeenAstroEmulator\installer\.out\TeenAstroApp.msi.

$ErrorActionPreference = "Stop"
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$RepoRoot = Split-Path (Split-Path $ScriptDir -Parent) -Parent
$BuildMsi = Join-Path $RepoRoot "TeenAstroEmulator\installer\build_msi.ps1"
if (-not (Test-Path $BuildMsi)) {
    Write-Host "build_msi.ps1 not found at $BuildMsi" -ForegroundColor Red
    exit 1
}
& $BuildMsi -AppOnly
exit $LASTEXITCODE
