# Fix Flutter SDK (if broken) and build TeenAstro app for Android and Windows.
#
# Usage:
#   .\fix_flutter_and_build.ps1                    # Build only (Flutter must be on PATH or use -FlutterPath)
#   .\fix_flutter_and_build.ps1 -FixFlutter         # Reinstall Flutter with proper clone, then build
#   .\fix_flutter_and_build.ps1 -FlutterPath "C:\path\to\flutter\bin"
#
# Prerequisites:
#   - Git (for -FixFlutter)
#   - Android SDK / Android Studio (for build apk)
#   - Visual Studio with "Desktop development with C++" (for build windows)

param(
    [switch]$FixFlutter,
    [string]$FlutterPath = "C:\Users\charl\OneDrive\Dokumente\develop\flutter\bin",
    [switch]$SkipAndroid,
    [switch]$SkipWindows
)

$ErrorActionPreference = "Stop"
$DevelopRoot = "C:\Users\charl\OneDrive\Dokumente\develop"
$ScriptDir = $PSScriptRoot
$AppRoot = (Resolve-Path (Join-Path $ScriptDir "..")).Path

# Ensure Flutter is on PATH for this session
function Add-FlutterToPath {
    param([string]$BinPath)
    $bin = $BinPath.TrimEnd("\")
    if (Test-Path "$bin\flutter.bat") {
        $env:Path = "$bin;$env:Path"
        return $true
    }
    return $false
}

# --- Optional: Fix Flutter (reinstall with proper git clone) ---
if ($FixFlutter) {
    Write-Host "=== Fixing Flutter SDK ===" -ForegroundColor Cyan
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        Write-Error "Git is required for -FixFlutter. Install Git and try again."
    }
    Set-Location $DevelopRoot
    if (Test-Path "flutter") {
        Write-Host "Backing up existing flutter folder to flutter_broken..."
        if (Test-Path "flutter_broken") {
            Remove-Item -Recurse -Force flutter_broken
        }
        Rename-Item flutter flutter_broken -ErrorAction SilentlyContinue
        if (Test-Path "flutter") {
            Write-Host "Removing existing flutter folder for fresh clone..."
            Remove-Item -Recurse -Force flutter
        }
    }
    Write-Host "Cloning Flutter (stable)..."
    git clone https://github.com/flutter/flutter.git -b stable
    if (-not (Test-Path "flutter\bin\flutter.bat")) {
        Write-Error "Clone failed or flutter\bin\flutter.bat not found."
    }
    $FlutterPath = (Resolve-Path "flutter\bin").Path
    Add-FlutterToPath -BinPath $FlutterPath
    Write-Host "Running flutter doctor (first run downloads Dart SDK, may take a few minutes)..."
    & "$FlutterPath\flutter.bat" doctor -v
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "flutter doctor reported issues. You may need to fix them before building."
    }
    Write-Host "Flutter fix step done." -ForegroundColor Green
}

# --- Ensure we can run Flutter ---
if (-not (Get-Command flutter -ErrorAction SilentlyContinue)) {
    if (Add-FlutterToPath -BinPath $FlutterPath) {
        Write-Host "Using Flutter at: $FlutterPath" -ForegroundColor Gray
    } else {
        Write-Error "Flutter not found. Add Flutter\bin to PATH, use -FlutterPath, or run with -FixFlutter."
    }
}

# --- Build app ---
Set-Location $AppRoot
Write-Host "`n=== flutter pub get ===" -ForegroundColor Cyan
flutter pub get
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$androidOk = $true
if (-not $SkipAndroid) {
    Write-Host "`n=== Building Android APK ===" -ForegroundColor Cyan
    flutter build apk
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Android build failed (e.g. Android SDK not installed or v1 embedding). Continuing with Windows build..."
        $androidOk = $false
    } else {
        $apk = Join-Path $AppRoot "build\app\outputs\flutter-apk\app-release.apk"
        if (Test-Path $apk) {
            Write-Host "APK: $apk" -ForegroundColor Green
        }
    }
}

$windowsOk = $true
if (-not $SkipWindows) {
    Write-Host "`n=== Building Windows ===" -ForegroundColor Cyan
    flutter build windows
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Windows build failed. Ensure Visual Studio with 'Desktop development with C++' is installed."
        $windowsOk = $false
    } else {
        $winDir = Join-Path $AppRoot "build\windows\x64\runner\Release"
        if (Test-Path $winDir) {
            Write-Host "Windows build: $winDir" -ForegroundColor Green
            Get-ChildItem $winDir -Filter "*.exe" | ForEach-Object { Write-Host "  $($_.FullName)" }
        }
    }
}

if (-not $androidOk) { exit 1 }
if (-not $windowsOk) { exit 1 }
Write-Host "`nDone." -ForegroundColor Green
