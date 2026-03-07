# Build TeenAstro MSI: SHC Emulator + Firmware Uploader + TeenAstro App.
# Requires: PlatformIO (pio), MSBuild (.NET Framework), Flutter, WiX Toolset 3.
#
# Run from repo root: .\TeenAstroEmulator\installer\build_msi.ps1
# Or from this folder: .\build_msi.ps1 (script detects repo root)
#
# Switches:
#   -SkipEmulator    Skip building the SHC emulator (reuse staged exe)
#   -SkipUploader    Skip building the firmware uploader
#   -SkipApp         Skip building the Flutter Windows app
#   -FlutterPath     Path to Flutter bin/ (default: flutter on PATH)

param(
    [switch]$SkipEmulator,
    [switch]$SkipUploader,
    [switch]$SkipApp,
    [string]$FlutterPath
)

$ErrorActionPreference = "Stop"

# ---------- Resolve paths ----------
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Get-Location }
$EmuDir = Split-Path $ScriptDir -Parent
if (-not (Test-Path (Join-Path $EmuDir "platformio.ini"))) {
    $EmuDir = Join-Path (Get-Location) "TeenAstroEmulator"
}
if (-not (Test-Path (Join-Path $EmuDir "platformio.ini"))) {
    Write-Host "Cannot find TeenAstroEmulator (platformio.ini). Run from repo root or TeenAstroEmulator\installer." -ForegroundColor Red
    exit 1
}
$RepoRoot     = Split-Path $EmuDir -Parent
$InstallerDir = Join-Path $EmuDir "installer"
$StageDir     = Join-Path $InstallerDir "stage"
$BuildSHC     = Join-Path $EmuDir ".pio\build\emu_shc"
$UploaderProj = Join-Path $RepoRoot "TeenAstroUploader\TeenAstroUploader\TeenAstroUploader.vbproj"
$UploaderBin  = Join-Path $RepoRoot "TeenAstroUploader\TeenAstroUploader\bin\Release"
$AppRoot      = Join-Path $RepoRoot "teenastro_app"
$AppBuildDir  = Join-Path $AppRoot "build\windows\x64\runner\Release"

Write-Host "Repo root    : $RepoRoot" -ForegroundColor Cyan
Write-Host "Emu root     : $EmuDir" -ForegroundColor Cyan
Write-Host "Installer dir: $InstallerDir" -ForegroundColor Cyan

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

# ---------- Helper: find WiX bin ----------
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

# ================================================================
#  BUILD STEPS
# ================================================================

# ---------- 1. Build SHC emulator ----------
if (-not $SkipEmulator) {
    Push-Location $RepoRoot
    try {
        Write-Host "`n=== Building SHC Emulator ===" -ForegroundColor Yellow
        pio run -d TeenAstroEmulator -e emu_shc
    } finally { Pop-Location }
}

# ---------- 2. Build Firmware Uploader ----------
if (-not $SkipUploader) {
    Write-Host "`n=== Building Firmware Uploader ===" -ForegroundColor Yellow
    $msbuild = Find-MSBuild
    if (-not $msbuild) {
        Write-Host "MSBuild not found. Install Visual Studio or .NET Framework SDK." -ForegroundColor Red
        exit 1
    }
    Write-Host "Using MSBuild: $msbuild" -ForegroundColor Cyan
    & $msbuild $UploaderProj /p:Configuration=Release /v:minimal
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# ---------- 3. Build Flutter Windows app ----------
if (-not $SkipApp) {
    Write-Host "`n=== Building TeenAstro App (Flutter Windows) ===" -ForegroundColor Yellow
    if ($FlutterPath) {
        $env:Path = "$FlutterPath;$env:Path"
    }
    if (-not (Get-Command flutter -ErrorAction SilentlyContinue)) {
        Write-Host "Flutter not found on PATH. Use -FlutterPath or add Flutter\bin to PATH." -ForegroundColor Red
        exit 1
    }
    Push-Location $AppRoot
    try {
        flutter pub get
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        flutter build windows
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    } finally { Pop-Location }
    if (-not (Test-Path $AppBuildDir)) {
        Write-Host "Flutter build output not found at $AppBuildDir" -ForegroundColor Red
        exit 1
    }
}

# ================================================================
#  STAGE FILES
# ================================================================
Write-Host "`n=== Staging files ===" -ForegroundColor Yellow
$null = New-Item -ItemType Directory -Force -Path $StageDir

# Emulator
if (Test-Path (Join-Path $BuildSHC "program.exe")) {
    Copy-Item (Join-Path $BuildSHC "program.exe") (Join-Path $StageDir "TeenAstroSHC.exe") -Force
    Copy-Item (Join-Path $BuildSHC "SDL2.dll")    (Join-Path $StageDir "SDL2.dll") -Force
}
# Icon
Copy-Item (Join-Path $InstallerDir "icon.ico") (Join-Path $StageDir "icon.ico") -Force

# Uploader
$uploaderExe = Join-Path $UploaderBin "TeenAstroUploader.exe"
if (Test-Path $uploaderExe) {
    Copy-Item $uploaderExe (Join-Path $StageDir "TeenAstroUploader.exe") -Force
}

# Flutter app -- copy all files flat into stage (exe + DLLs + data/)
if (Test-Path $AppBuildDir) {
    Copy-Item "$AppBuildDir\*" $StageDir -Recurse -Force
}

Write-Host "Staged: $StageDir" -ForegroundColor Green
Get-ChildItem $StageDir | ForEach-Object { Write-Host "  $($_.Name)" }

# ================================================================
#  WIX: harvest app files + compile + link
# ================================================================
Write-Host "`n=== Building MSI ===" -ForegroundColor Yellow
$wixBin = Find-WixBin
if (-not $wixBin) {
    Write-Host "WiX Toolset 3 not found. Install from https://wixtoolset.org/ and add to PATH (or set WIX env var)." -ForegroundColor Red
    exit 1
}

$outDir = Join-Path $InstallerDir "out"
$null = New-Item -ItemType Directory -Force -Path $outDir

# Use heat.exe to harvest the Flutter app files (exe + DLLs + data/ tree) into a WiX fragment.
# We harvest the stage dir for the app files, excluding the files we already list explicitly.
$heatExe = Join-Path $wixBin "heat.exe"
$appWxs  = Join-Path $outDir "AppFiles.wxs"
$appObj  = Join-Path $outDir "AppFiles.wixobj"

Push-Location $InstallerDir
try {
    Write-Host "  Harvesting app files with heat.exe..." -ForegroundColor Gray
    & $heatExe dir $StageDir -nologo -cg AppFiles -dr INSTALLFOLDER `
        -srd -ke -gg -sfrag -template fragment `
        -var "var.StageDir" `
        -out $appWxs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $mainWxs = Join-Path $InstallerDir "TeenAstroEmulator.wxs"
    $mainObj = Join-Path $outDir "TeenAstroEmulator.wixobj"
    $msi     = Join-Path $outDir "TeenAstroEmulator.msi"

    Write-Host "  Compiling WiX (candle)..." -ForegroundColor Gray
    & (Join-Path $wixBin "candle.exe") -nologo `
        "-dStageDir=$StageDir" `
        -out "$outDir\" `
        $mainWxs $appWxs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Linking MSI (light)..." -ForegroundColor Gray
    & (Join-Path $wixBin "light.exe") -nologo -out $msi `
        -b $StageDir `
        $mainObj $appObj
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "`nMSI created: $msi" -ForegroundColor Green
} finally { Pop-Location }
