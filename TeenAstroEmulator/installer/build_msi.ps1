# Build TeenAstro MSI: MainUnit + SHC emulators; optionally Firmware Uploader and TeenAstro App.
# Requires: PlatformIO (pio), WiX Toolset 3. Optional: MSBuild (uploader), Flutter (app), MinGW (emulator).
#
# Run from repo root: .\TeenAstroEmulator\installer\build_msi.ps1
# Or from this folder: .\build_msi.ps1 (script detects repo root)
#
# Switches:
#   -EmulatorsOnly   Build and install only the emulators (MainUnit + SHC). No Uploader, no App. No MSBuild/Flutter needed.
#   -UploaderOnly    Build and install only the Firmware Uploader (all required exes + config). No emulators, no App. MSI -> TeenAstroUploader.msi
#   -SkipEmulator    Skip building the emulators (reuse staged exe)
#   -SkipUploader    Skip building the firmware uploader
#   -SkipApp         Skip building the Flutter Windows app
#   -FlutterPath     Path to Flutter bin/ (default: flutter on PATH)

param(
    [switch]$EmulatorsOnly,
    [switch]$UploaderOnly,
    [switch]$SkipEmulator,
    [switch]$SkipUploader,
    [switch]$SkipApp,
    [string]$FlutterPath
)

$ErrorActionPreference = "Stop"

# When the script fails (e.g. double-clicked), keep the window open so the user can read the error.
function Pause-IfInteractive {
    if ([Environment]::UserInteractive) {
        Write-Host "`nPress Enter to close this window..." -ForegroundColor Yellow
        $null = Read-Host
    }
}

# ---------- Resolve paths ----------
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Get-Location }
$EmuDir = Split-Path $ScriptDir -Parent
if (-not (Test-Path (Join-Path $EmuDir "platformio.ini"))) {
    $EmuDir = Join-Path (Get-Location) "TeenAstroEmulator"
}
if (-not (Test-Path (Join-Path $EmuDir "platformio.ini"))) {
    Write-Host "Cannot find TeenAstroEmulator (platformio.ini). Run from repo root or TeenAstroEmulator\installer." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
$RepoRoot     = Split-Path $EmuDir -Parent
$InstallerDir = Join-Path $EmuDir "installer"
$StageDir     = Join-Path $InstallerDir ".stage"
if ($EmulatorsOnly) {
    $SkipUploader = $true
    $SkipApp     = $true
}
if ($UploaderOnly) {
    $SkipEmulator = $true
    $SkipApp      = $true
}
$BuildEmu     = Join-Path $EmuDir ".pio\build\emu"
$BuildMainUnit = $BuildEmu
$BuildSHC     = $BuildEmu
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

# ---------- 1. Build MainUnit and SHC emulators ----------
if (-not $SkipEmulator) {
    # PlatformIO's toolchain-gccmingw32 is not always on PATH when pio spawns the compiler.
    $pioHome = if ($env:PIOHOME_DIR) { $env:PIOHOME_DIR } else { Join-Path $env:USERPROFILE ".platformio" }
    $toolchainBin = Join-Path $pioHome "packages\toolchain-gccmingw32\bin"
    if (Test-Path (Join-Path $toolchainBin "g++.exe")) {
        $env:Path = "$toolchainBin;$env:Path"
    }
    Push-Location $RepoRoot
    try {
        Write-Host "`n=== Building MainUnit Emulator (extended state view) ===" -ForegroundColor Yellow
        pio run -d TeenAstroEmulator -e emu_mainunit
        if ($LASTEXITCODE -ne 0) {
            Write-Host "MainUnit emulator build failed. Use -SkipEmulator if you have pre-built mainunit_emu.exe in .pio\build\emu." -ForegroundColor Red
            Pause-IfInteractive; exit $LASTEXITCODE
        }
        Write-Host "`n=== Building SHC Emulator ===" -ForegroundColor Yellow
        pio run -d TeenAstroEmulator -e emu_shc
        if ($LASTEXITCODE -ne 0) {
            Write-Host "SHC emulator build failed. Use -SkipEmulator if you have pre-built shc_emu.exe in .pio\build\emu." -ForegroundColor Red
            Pause-IfInteractive; exit $LASTEXITCODE
        }
    } finally { Pop-Location }
}

# ---------- 2. Build Firmware Uploader ----------
if (-not $SkipUploader) {
    Write-Host "`n=== Building Firmware Uploader ===" -ForegroundColor Yellow
    $msbuild = Find-MSBuild
    if (-not $msbuild) {
        Write-Host "MSBuild not found. Install Visual Studio or .NET Framework SDK." -ForegroundColor Red
        Pause-IfInteractive; exit 1
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
        Write-Host "Flutter not found on PATH. Use -SkipApp to build without the app, or -FlutterPath / add Flutter\bin to PATH." -ForegroundColor Red
        Pause-IfInteractive; exit 1
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
        Pause-IfInteractive; exit 1
    }
}

# ================================================================
#  STAGE FILES
# ================================================================
Write-Host "`n=== Staging files ===" -ForegroundColor Yellow
$null = New-Item -ItemType Directory -Force -Path $StageDir
# When -SkipApp, -EmulatorsOnly, or -UploaderOnly, clear stage so we don't harvest leftover files.
if ($SkipApp -or $EmulatorsOnly -or $UploaderOnly) {
    Get-ChildItem $StageDir -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

# MainUnit emulator (extended state view / tabbed cockpit) — skip when UploaderOnly
if (-not $UploaderOnly -and (Test-Path (Join-Path $BuildEmu "mainunit_emu.exe"))) {
    Copy-Item (Join-Path $BuildEmu "mainunit_emu.exe") (Join-Path $StageDir "TeenAstroMainUnit.exe") -Force
}
# SHC emulator — skip when UploaderOnly
if (-not $UploaderOnly -and (Test-Path (Join-Path $BuildEmu "shc_emu.exe"))) {
    Copy-Item (Join-Path $BuildEmu "shc_emu.exe") (Join-Path $StageDir "TeenAstroSHC.exe") -Force
}
# SDL2 (used by both emulators; from shared build dir or C:\SDL2\bin) — skip when UploaderOnly
if (-not $UploaderOnly) {
    $sdl2 = Join-Path $BuildEmu "SDL2.dll"
    if (-not (Test-Path $sdl2)) { $sdl2 = "C:\SDL2\bin\SDL2.dll" }
    if (Test-Path $sdl2) {
        Copy-Item $sdl2 (Join-Path $StageDir "SDL2.dll") -Force
    } else {
        Write-Host "  Warning: SDL2.dll not found in build or C:\SDL2\bin. MSI may fail if SDL2 is required." -ForegroundColor Yellow
    }
}
# Icon (required for MSI and shortcuts)
$iconSrc = Join-Path $InstallerDir "icon.ico"
if (-not (Test-Path $iconSrc)) {
    Write-Host "Missing installer/icon.ico. Add an icon.ico to TeenAstroEmulator\installer (used for MSI and shortcuts)." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Copy-Item $iconSrc (Join-Path $StageDir "icon.ico") -Force

# Uploader: single exe when full build; full bin (exe + config + esptool + teensy*) when UploaderOnly. App creates version dirs (1.6, etc.) if missing.
if ($UploaderOnly) {
    if (Test-Path $UploaderBin) {
        Copy-Item "$UploaderBin\*" $StageDir -Recurse -Force
        Write-Host "  Staged Firmware Uploader (all files from bin\Release)." -ForegroundColor Gray
    } else {
        Write-Host "Firmware Uploader build output not found at $UploaderBin. Build with MSBuild first." -ForegroundColor Red
        Pause-IfInteractive; exit 1
    }
} elseif (-not $EmulatorsOnly) {
    $uploaderExe = Join-Path $UploaderBin "TeenAstroUploader.exe"
    if (Test-Path $uploaderExe) {
        Copy-Item $uploaderExe (Join-Path $StageDir "TeenAstroUploader.exe") -Force
    }
}

# Flutter app -- copy all files flat into stage (exe + DLLs + data/) (skipped when -EmulatorsOnly or -SkipApp)
if (-not $EmulatorsOnly -and -not $SkipApp -and (Test-Path $AppBuildDir)) {
    Copy-Item "$AppBuildDir\*" $StageDir -Recurse -Force
}

Write-Host "Staged: $StageDir" -ForegroundColor Green
Get-ChildItem $StageDir | ForEach-Object { Write-Host "  $($_.Name)" }

# ================================================================
#  WIX: harvest app files (full build only) + compile + link
# ================================================================
Write-Host "`n=== Building MSI ===" -ForegroundColor Yellow
$wixBin = Find-WixBin
if (-not $wixBin) {
    Write-Host "WiX Toolset 3 not found. Install from https://wixtoolset.org/ and add to PATH (or set WIX env var)." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}

$outDir = Join-Path $InstallerDir ".out"
$null = New-Item -ItemType Directory -Force -Path $outDir

Push-Location $InstallerDir
try {
    $mainWxs = $null
    $mainObj = $null
    $msi     = if ($UploaderOnly) { Join-Path $outDir "TeenAstroUploader.msi" } else { Join-Path $outDir "TeenAstroEmulator.msi" }
    $candleInputs = @()
    $lightInputs  = @()

    if ($EmulatorsOnly) {
        # Emulators only: use minimal wxs, no heat. Use relative paths so candle/light do not mis-parse "C:\".
        $mainWxs = Join-Path $InstallerDir "TeenAstroEmulator_emulators_only.wxs"
        $mainObj = Join-Path $outDir "TeenAstroEmulator_emulators_only.wixobj"
        $candleInputs = @("TeenAstroEmulator_emulators_only.wxs")
        $lightInputs  = @(".out\TeenAstroEmulator_emulators_only.wixobj")
        Write-Host "  Emulators-only package (MainUnit + SHC)." -ForegroundColor Gray
    } elseif ($UploaderOnly) {
        # Firmware Uploader only: harvest staged files with heat, then compile main wxs + fragment. Use relative paths.
        $heatExe = Join-Path $wixBin "heat.exe"
        $appWxs  = Join-Path $outDir "AppFiles_uploader.wxs"
        $appObj  = Join-Path $outDir "AppFiles_uploader.wixobj"
        Write-Host "  Harvesting Firmware Uploader files with heat.exe..." -ForegroundColor Gray
        & $heatExe dir ".stage" -nologo -cg UploaderFiles -dr INSTALLFOLDER -srd -ke -gg -sfrag -sreg -template fragment -var "var.StageDir" -out $appWxs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $mainWxs = Join-Path $InstallerDir "TeenAstroUploader_only.wxs"
        $mainObj = Join-Path $outDir "TeenAstroUploader_only.wixobj"
        $candleInputs = @("TeenAstroUploader_only.wxs", ".out\AppFiles_uploader.wxs")
        $lightInputs  = @(".out\TeenAstroUploader_only.wixobj", ".out\AppFiles_uploader.wixobj")
        Write-Host "  Firmware Uploader package." -ForegroundColor Gray
    } else {
        # Full package: harvest Flutter app with heat, then compile main wxs + AppFiles.
        $heatExe = Join-Path $wixBin "heat.exe"
        $heatXsl = Join-Path $InstallerDir "heat_exclude_main.wxs.xsl"
        $appWxs  = Join-Path $outDir "AppFiles.wxs"
        $appObj  = Join-Path $outDir "AppFiles.wixobj"
        Write-Host "  Harvesting app files with heat.exe..." -ForegroundColor Gray
        & $heatExe dir $StageDir -nologo -cg AppFiles -dr INSTALLFOLDER `
            -srd -ke -gg -sfrag -sreg -template fragment `
            -var "var.StageDir" `
            -t $heatXsl `
            -out $appWxs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $mainWxs = Join-Path $InstallerDir "TeenAstroEmulator.wxs"
        $mainObj = Join-Path $outDir "TeenAstroEmulator.wixobj"
        $candleInputs = $mainWxs, $appWxs
        $lightInputs  = $mainObj, $appObj
    }

    Write-Host "  Compiling WiX (candle)..." -ForegroundColor Gray
    $candleArgs = @("-nologo", "-out", "$outDir\")
    if (-not $EmulatorsOnly) {
        # UploaderOnly: use relative StageDir so candle/light do not mis-parse "C:\"
        $stageDirArg = if ($UploaderOnly) { ".stage" } else { $StageDir }
        $candleArgs += "-dStageDir=`"$stageDirArg`""
    }
    & (Join-Path $wixBin "candle.exe") @candleArgs @candleInputs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Linking MSI (light)..." -ForegroundColor Gray
    # EmulatorsOnly/UploaderOnly: use relative -b and -out so light.exe does not mis-parse "C:\"
    $bindPath = if ($EmulatorsOnly -or $UploaderOnly) { "." } else { $StageDir }
    $lightOut = if ($EmulatorsOnly) { ".out\TeenAstroEmulator.msi" } elseif ($UploaderOnly) { ".out\TeenAstroUploader.msi" } else { $msi }
    & (Join-Path $wixBin "light.exe") -nologo -out $lightOut -b $bindPath @lightInputs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "`nMSI created: $msi" -ForegroundColor Green
} finally { Pop-Location }
