# Build TeenAstro App MSI (Flutter Windows). Entry point for Run_App_build.bat.
# Requires: Flutter, WiX Toolset 3. MSI output: .out\TeenAstroApp.msi (relative to this installer dir).
# Run from repo root or any dir; script resolves paths.

param([string]$FlutterPath)

$ErrorActionPreference = "Stop"

function Pause-IfInteractive {
    if ([Environment]::UserInteractive) {
        Write-Host "`nPress Enter to close this window..." -ForegroundColor Yellow
        $null = Read-Host
    }
}

function Find-WixBin {
    $wp = $env:WIX
    if ($wp) {
        $bin = if (Test-Path (Join-Path $wp "bin")) { Join-Path $wp "bin" } else { $wp }
        return $bin
    }
    $candle = Get-Command candle -ErrorAction SilentlyContinue
    $light  = Get-Command light  -ErrorAction SilentlyContinue
    if ($candle -and $light) { return (Split-Path $candle.Source -Parent) }
    return $null
}

# ---------- Paths ----------
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$AppDir = Split-Path $ScriptDir -Parent
$RepoRoot = Split-Path $AppDir -Parent
$InstallerDir = $ScriptDir
$StageDir = Join-Path $InstallerDir ".stage"
$OutDir = Join-Path $InstallerDir ".out"
$AppBuildDir = Join-Path $AppDir "build\windows\x64\runner\Release"
$IconSrc = Join-Path $RepoRoot "TeenAstroEmulator\installer\icon.ico"

Write-Host "Repo root  : $RepoRoot" -ForegroundColor Cyan
Write-Host "Installer  : $InstallerDir" -ForegroundColor Cyan

# ---------- Build Flutter app ----------
Write-Host "`n=== Building TeenAstro App (Flutter Windows) ===" -ForegroundColor Yellow
if ($FlutterPath) { $env:Path = "$FlutterPath;$env:Path" }
if (-not (Get-Command flutter -ErrorAction SilentlyContinue)) {
    Write-Host "Flutter not found on PATH. Add Flutter\bin to PATH or use -FlutterPath." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Push-Location $AppDir
try {
    flutter pub get
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    flutter build windows
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally { Pop-Location }

if (-not (Test-Path $AppBuildDir)) {
    Write-Host "Flutter build output not found at $AppBuildDir" -ForegroundColor Red
    Pause-IfInteractive; exit 1
}

# ---------- Stage ----------
Write-Host "`n=== Staging files ===" -ForegroundColor Yellow
$null = New-Item -ItemType Directory -Force -Path $StageDir
Get-ChildItem $StageDir -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$AppBuildDir\*" $StageDir -Recurse -Force
if (-not (Test-Path $IconSrc)) {
    Write-Host "Icon not found at $IconSrc" -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Copy-Item $IconSrc (Join-Path $StageDir "icon.ico") -Force

$required = @(
    (Join-Path $StageDir "teenastro_app.exe"),
    (Join-Path $StageDir "flutter_windows.dll"),
    (Join-Path $StageDir "data")
)
$missing = $required | Where-Object { -not (Test-Path $_) }
if ($missing.Count -gt 0) {
    Write-Host "Missing required app files in stage: $($missing -join ', ')" -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Write-Host "Staged: $StageDir" -ForegroundColor Green

# ---------- WiX ----------
Write-Host "`n=== Building MSI ===" -ForegroundColor Yellow
$wixBin = Find-WixBin
if (-not $wixBin) {
    Write-Host "WiX Toolset 3 not found. Install from https://wixtoolset.org/ and add to PATH (or set WIX env var)." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
$null = New-Item -ItemType Directory -Force -Path $OutDir

Push-Location $InstallerDir
try {
    $heatExe = Join-Path $wixBin "heat.exe"
    $appWxs = Join-Path $OutDir "AppFiles_app.wxs"
    Write-Host "  Harvesting TeenAstro App files with heat.exe..." -ForegroundColor Gray
    & $heatExe dir ".stage" -nologo -cg AppFiles -dr INSTALLFOLDER -srd -ke -gg -sfrag -sreg -template fragment -var "var.StageDir" -out $appWxs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Compiling WiX (candle)..." -ForegroundColor Gray
    & (Join-Path $wixBin "candle.exe") -nologo -out "$OutDir\" -dStageDir=".stage" "TeenAstroApp_only.wxs" ".out\AppFiles_app.wxs"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Linking MSI (light)..." -ForegroundColor Gray
    $msiOut = Join-Path $OutDir "TeenAstroApp.msi"
    & (Join-Path $wixBin "light.exe") -nologo -out $msiOut -b "." ".out\TeenAstroApp_only.wixobj" ".out\AppFiles_app.wixobj"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "`nMSI created: $msiOut" -ForegroundColor Green
} finally { Pop-Location }
