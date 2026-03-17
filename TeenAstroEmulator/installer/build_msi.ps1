# Build TeenAstro MSI: MainUnit + SHC emulators; optionally Firmware Uploader and TeenAstro App.
# Requires: PlatformIO (pio), WiX Toolset 3. Optional: MSBuild (uploader), Flutter (app), MinGW (emulator).
#
# Run from repo root: .\TeenAstroEmulator\installer\build_msi.ps1
# Or from this folder: .\build_msi.ps1 (script detects repo root)
#
# Switches:
#   -EmulatorsOnly   Build and install only the emulators (MainUnit + SHC). No Uploader, no App. No MSBuild/Flutter needed.
#   -UploaderOnly    Build and install only the Firmware Uploader (all required exes + config). No emulators, no App. MSI -> TeenAstroUploader.msi
#   -AppOnly         Build and install only the TeenAstro Windows app (Flutter). No emulators, no Uploader. MSI -> TeenAstroApp.msi
#   -SkipEmulator    Skip building the emulators (reuse staged exe)
#   -SkipUploader    Skip building the firmware uploader
#   -SkipApp         Skip building the Flutter Windows app
#   -FlutterPath     Path to Flutter bin/ (default: flutter on PATH)

param(
    [switch]$EmulatorsOnly,
    [switch]$UploaderOnly,
    [switch]$AppOnly,
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
if ($AppOnly) {
    $SkipEmulator = $true
    $SkipUploader = $true
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
# When -SkipApp, -EmulatorsOnly, -UploaderOnly, or -AppOnly, clear stage so we don't harvest leftover files.
if ($SkipApp -or $EmulatorsOnly -or $UploaderOnly -or $AppOnly) {
    Get-ChildItem $StageDir -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

# Emulators: stage all build output (exes + all DLLs: SDL2, MinGW runtime, etc.) so MSI has everything — skip when UploaderOnly or AppOnly
if (-not $UploaderOnly -and -not $AppOnly) {
    $emuStaged = $false
    # Prefer shared dir .pio/build/emu (setup_env.py) with mainunit_emu.exe and shc_emu.exe
    if (Test-Path $BuildEmu) {
        Get-ChildItem -Path $BuildEmu -File | ForEach-Object {
            $destName = $_.Name
            if ($_.Name -eq "mainunit_emu.exe") { $destName = "TeenAstroMainUnit.exe" }
            if ($_.Name -eq "shc_emu.exe")       { $destName = "TeenAstroSHC.exe" }
            Copy-Item $_.FullName (Join-Path $StageDir $destName) -Force
        }
        if ((Test-Path (Join-Path $StageDir "TeenAstroMainUnit.exe")) -and (Test-Path (Join-Path $StageDir "TeenAstroSHC.exe"))) { $emuStaged = $true }
    }
    # Fallback: PlatformIO may use per-env dirs .pio/build/emu_mainunit and .pio/build/emu_shc with program.exe
    if (-not $emuStaged) {
        $buildMainUnit = Join-Path $EmuDir ".pio\build\emu_mainunit"
        $buildSHC = Join-Path $EmuDir ".pio\build\emu_shc"
        $mainExe = Join-Path $buildMainUnit "program.exe"
        $shcExe = Join-Path $buildSHC "program.exe"
        if ((Test-Path $mainExe) -and (Test-Path $shcExe)) {
            Copy-Item $mainExe (Join-Path $StageDir "TeenAstroMainUnit.exe") -Force
            Copy-Item $shcExe (Join-Path $StageDir "TeenAstroSHC.exe") -Force
            Get-ChildItem -Path $buildMainUnit -Filter "*.dll" -File -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item $_.FullName (Join-Path $StageDir $_.Name) -Force }
            Get-ChildItem -Path $buildSHC -Filter "*.dll" -File -ErrorAction SilentlyContinue | ForEach-Object { Copy-Item $_.FullName (Join-Path $StageDir $_.Name) -Force }
            Write-Host "  Staged emulators from .pio\build\emu_mainunit and emu_shc (program.exe)." -ForegroundColor Gray
            $emuStaged = $true
        }
    }
    if (-not $emuStaged) {
        Write-Host "Emulator build output not found. Expected .pio\build\emu\mainunit_emu.exe and shc_emu.exe, or .pio\build\emu_mainunit\program.exe and .pio\build\emu_shc\program.exe. Run the build first (e.g. pio run -d TeenAstroEmulator -e emu_mainunit and -e emu_shc)." -ForegroundColor Red
        Pause-IfInteractive; exit 1
    }
    if ($emuStaged) {
        # If SDL2.dll was not in build output, try C:\SDL2\bin (e.g. when build uses system SDL2)
        if (-not (Test-Path (Join-Path $StageDir "SDL2.dll"))) {
            $sdl2 = "C:\SDL2\bin\SDL2.dll"
            if (Test-Path $sdl2) {
                Copy-Item $sdl2 (Join-Path $StageDir "SDL2.dll") -Force
                Write-Host "  SDL2.dll taken from C:\SDL2\bin." -ForegroundColor Gray
            } else {
                Write-Host "  Warning: SDL2.dll not in build and not at C:\SDL2\bin. MSI may be missing it." -ForegroundColor Yellow
            }
        }
        # MinGW runtime DLLs (libgcc_s_dw2-1, libstdc++-6, libwinpthread-1) are not copied to build output; take from toolchain
        $mingwDlls = @("libgcc_s_dw2-1.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
        $pioHome = if ($env:PIOHOME_DIR) { $env:PIOHOME_DIR } else { Join-Path $env:USERPROFILE ".platformio" }
        $toolchainBins = @(
            (Join-Path $pioHome "packages\toolchain-gccmingw32\bin"),
            (Join-Path $RepoRoot ".platformio\packages\toolchain-gccmingw32\bin")
        )
        foreach ($dll in $mingwDlls) {
            if (Test-Path (Join-Path $StageDir $dll)) { continue }
            foreach ($tc in $toolchainBins) {
                $src = Join-Path $tc $dll
                if (Test-Path $src) {
                    Copy-Item $src (Join-Path $StageDir $dll) -Force
                    Write-Host "  $dll from toolchain." -ForegroundColor Gray
                    break
                }
            }
        }
        if ($EmulatorsOnly) { Write-Host "  Staged all emulator build files (exes + DLLs + MinGW runtime)." -ForegroundColor Gray }
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
} elseif (-not $EmulatorsOnly -and -not $AppOnly) {
    $uploaderExe = Join-Path $UploaderBin "TeenAstroUploader.exe"
    if (Test-Path $uploaderExe) {
        Copy-Item $uploaderExe (Join-Path $StageDir "TeenAstroUploader.exe") -Force
    }
}

# Flutter app -- copy all files flat into stage (exe + DLLs + data/) (skipped when -EmulatorsOnly or -SkipApp)
if (-not $EmulatorsOnly -and -not $SkipApp -and (Test-Path $AppBuildDir)) {
    Copy-Item "$AppBuildDir\*" $StageDir -Recurse -Force
    if ($AppOnly) {
        Write-Host "  Staged TeenAstro App (Flutter Windows: exe, DLLs, data\)." -ForegroundColor Gray
        # Ensure app MSI will contain all required files: exe, runtime DLL, and data folder
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
    }
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
    $msi     = if ($UploaderOnly) { Join-Path $outDir "TeenAstroUploader.msi" } elseif ($AppOnly) { Join-Path $outDir "TeenAstroApp.msi" } else { Join-Path $outDir "TeenAstroEmulator.msi" }
    $candleInputs = @()
    $lightInputs  = @()

    if ($EmulatorsOnly) {
        # Emulators only: harvest all staged files (exes + all DLLs) with heat so MSI contains everything.
        $heatExe = Join-Path $wixBin "heat.exe"
        $emuWxs  = Join-Path $outDir "EmulatorFiles.wxs"
        Write-Host "  Harvesting emulator files with heat.exe..." -ForegroundColor Gray
        & $heatExe dir ".stage" -nologo -cg EmulatorFiles -dr INSTALLFOLDER -srd -ke -gg -sfrag -sreg -template fragment -var "var.StageDir" -out $emuWxs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $mainWxs = Join-Path $InstallerDir "TeenAstroEmulator_emulators_only.wxs"
        $mainObj = Join-Path $outDir "TeenAstroEmulator_emulators_only.wixobj"
        $candleInputs = @("TeenAstroEmulator_emulators_only.wxs", ".out\EmulatorFiles.wxs")
        $lightInputs  = @(".out\TeenAstroEmulator_emulators_only.wixobj", ".out\EmulatorFiles.wixobj")
        Write-Host "  Emulators-only package (all exes + DLLs)." -ForegroundColor Gray
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
    } elseif ($AppOnly) {
        # TeenAstro App only: harvest staged Flutter files with heat (no exclude), then compile app-only wxs. Use relative paths.
        $heatExe = Join-Path $wixBin "heat.exe"
        $appWxs  = Join-Path $outDir "AppFiles_app.wxs"
        Write-Host "  Harvesting TeenAstro App files with heat.exe..." -ForegroundColor Gray
        & $heatExe dir ".stage" -nologo -cg AppFiles -dr INSTALLFOLDER -srd -ke -gg -sfrag -sreg -template fragment -var "var.StageDir" -out $appWxs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $mainWxs = Join-Path $InstallerDir "TeenAstroApp_only.wxs"
        $mainObj = Join-Path $outDir "TeenAstroApp_only.wixobj"
        $candleInputs = @("TeenAstroApp_only.wxs", ".out\AppFiles_app.wxs")
        $lightInputs  = @(".out\TeenAstroApp_only.wixobj", ".out\AppFiles_app.wixobj")
        Write-Host "  TeenAstro App package." -ForegroundColor Gray
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
    # EmulatorsOnly/UploaderOnly/AppOnly: use relative StageDir so candle/light do not mis-parse "C:\"
    $stageDirArg = if ($EmulatorsOnly -or $UploaderOnly -or $AppOnly) { ".stage" } else { $StageDir }
    $candleArgs += "-dStageDir=`"$stageDirArg`""
    & (Join-Path $wixBin "candle.exe") @candleArgs @candleInputs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Linking MSI (light)..." -ForegroundColor Gray
    # EmulatorsOnly/UploaderOnly: use relative -b and -out so light.exe does not mis-parse "C:\"
    $bindPath = if ($EmulatorsOnly -or $UploaderOnly -or $AppOnly) { "." } else { $StageDir }
    $lightOut = if ($EmulatorsOnly) { ".out\TeenAstroEmulator.msi" } elseif ($UploaderOnly) { ".out\TeenAstroUploader.msi" } elseif ($AppOnly) { ".out\TeenAstroApp.msi" } else { $msi }
    & (Join-Path $wixBin "light.exe") -nologo -out $lightOut -b $bindPath @lightInputs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "`nMSI created: $msi" -ForegroundColor Green
} finally { Pop-Location }
