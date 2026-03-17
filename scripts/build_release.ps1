# Build all TeenAstro release artifacts and collect them under release/.
#
# Usage (from repo root or scripts\):
#   .\build_release.ps1                 # Build everything
#   .\build_release.ps1 -SkipASCOM      # Skip ASCOM driver
#   .\build_release.ps1 -SkipMSI        # Skip MSI (emulator + uploader + app)
#   .\build_release.ps1 -SkipApp        # Skip standalone Flutter app copy
#   .\build_release.ps1 -FlutterPath "C:\flutter\bin"
#
# Prerequisites: PlatformIO, MSBuild (.NET 4.7.2), Flutter, WiX Toolset 3.
#
# Output layout:
#   release\ascom\          ASCOM driver (ASCOM.TeenAstro.exe + deps)
#   release\msi\            TeenAstroEmulator.msi
#   release\app\            Flutter Windows app (standalone copy)

param(
    [switch]$SkipASCOM,
    [switch]$SkipMSI,
    [switch]$SkipApp,
    [string]$FlutterPath
)

$ErrorActionPreference = "Stop"

# Resolve repo root
$_scriptFile = $MyInvocation.MyCommand.Path
if ($_scriptFile -and (Test-Path $_scriptFile)) {
    $_scriptDir = Split-Path $_scriptFile -Parent
} else {
    $_scriptDir = $PSScriptRoot
}
$RepoRoot = (Resolve-Path (Join-Path $_scriptDir "..")).Path
$ReleaseDir = Join-Path $RepoRoot "release"

Write-Host "Repo root : $RepoRoot" -ForegroundColor Cyan
Write-Host "Release   : $ReleaseDir" -ForegroundColor Cyan

# ---------- Helper: find MSBuild ----------
function Find-MSBuild {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        if ($found) { return $found }
    }
    $cmd = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $fw = Join-Path $env:SystemRoot "Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
    if (Test-Path $fw) { return $fw }
    return $null
}

# ================================================================
#  1. ASCOM DRIVER
# ================================================================
if (-not $SkipASCOM) {
    Write-Host "`n========================================" -ForegroundColor Yellow
    Write-Host "  Building ASCOM driver" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow

    $msbuild = Find-MSBuild
    if (-not $msbuild) {
        Write-Host "MSBuild not found. Skipping ASCOM build." -ForegroundColor Red
    } else {
        $ascomSln = Join-Path $RepoRoot "TeenAstroASCOM_V7\TeenAstroASCOM_V7.sln"
        Write-Host "Using MSBuild: $msbuild" -ForegroundColor Gray
        & $msbuild $ascomSln /p:Configuration=Release /v:minimal
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ASCOM build failed." -ForegroundColor Red
            exit $LASTEXITCODE
        }

        $ascomBin = Join-Path $RepoRoot "TeenAstroASCOM_V7\TeenAstroASCOM_V7\bin\Release"
        $ascomDest = Join-Path $ReleaseDir "ascom"
        if (Test-Path $ascomDest) { Remove-Item -Recurse -Force $ascomDest }
        New-Item -ItemType Directory -Path $ascomDest -Force | Out-Null
        Copy-Item "$ascomBin\*" $ascomDest -Recurse -Force
        Write-Host "ASCOM driver copied to: $ascomDest" -ForegroundColor Green
    }
}

# ================================================================
#  2. MSI (SHC Emulator + Uploader + App)
# ================================================================
if (-not $SkipMSI) {
    Write-Host "`n========================================" -ForegroundColor Yellow
    Write-Host "  Building MSI" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow

    $buildMsi = Join-Path $RepoRoot "TeenAstroEmulator\installer\build.ps1"
    $msiArgs = @()
    if ($FlutterPath) { $msiArgs += "-FlutterPath"; $msiArgs += $FlutterPath }
    & $buildMsi @msiArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "MSI build failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }

    $msiSrc = Join-Path $RepoRoot "TeenAstroEmulator\installer\out\TeenAstroEmulator.msi"
    $msiDest = Join-Path $ReleaseDir "msi"
    if (Test-Path $msiDest) { Remove-Item -Recurse -Force $msiDest }
    New-Item -ItemType Directory -Path $msiDest -Force | Out-Null
    if (Test-Path $msiSrc) {
        Copy-Item $msiSrc $msiDest -Force
        Write-Host "MSI copied to: $msiDest" -ForegroundColor Green
    }
}

# ================================================================
#  3. Standalone Flutter app copy
# ================================================================
if (-not $SkipApp) {
    Write-Host "`n========================================" -ForegroundColor Yellow
    Write-Host "  Copying Flutter Windows app" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow

    $appBuild = Join-Path $RepoRoot "teenastro_app\build\windows\x64\runner\Release"
    if (Test-Path $appBuild) {
        $appDest = Join-Path $ReleaseDir "app"
        if (Test-Path $appDest) { Remove-Item -Recurse -Force $appDest }
        New-Item -ItemType Directory -Path $appDest -Force | Out-Null
        Copy-Item "$appBuild\*" $appDest -Recurse -Force
        Write-Host "App copied to: $appDest" -ForegroundColor Green
    } else {
        Write-Host "Flutter Windows build not found at $appBuild. Build the app first or run without -SkipMSI." -ForegroundColor Yellow
    }
}

# ================================================================
#  Summary
# ================================================================
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  Release artifacts" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
if (Test-Path (Join-Path $ReleaseDir "ascom")) {
    Write-Host "  ASCOM : release\ascom\" -ForegroundColor Green
}
if (Test-Path (Join-Path $ReleaseDir "msi")) {
    Write-Host "  MSI   : release\msi\TeenAstroEmulator.msi" -ForegroundColor Green
}
if (Test-Path (Join-Path $ReleaseDir "app")) {
    Write-Host "  App   : release\app\" -ForegroundColor Green
}
Write-Host "`nDone." -ForegroundColor Green
