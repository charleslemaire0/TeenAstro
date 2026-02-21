# Build TeenAstro Flutter app (Android APK + Windows). Optionally fix broken Flutter SDK.
#
# Usage (from repo root or scripts\):
#   .\build_app.ps1                    # Build Android + Windows
#   .\build_app.ps1 -SkipAndroid       # Windows only
#   .\build_app.ps1 -SkipWindows      # Android only
#   .\build_app.ps1 -FixFlutter        # Reinstall Flutter then build
#   .\build_app.ps1 -FlutterPath "C:\path\to\flutter\bin"
#
# Prerequisites:
#   - Flutter on PATH (or -FlutterPath); Android SDK for APK; VS with C++ for Windows
#
# Output: Released data\App\ (APK and Windows\ subfolder)

param(
    [switch]$FixFlutter,
    [string]$FlutterPath = "C:\Users\charl\OneDrive\Dokumente\develop\flutter\bin",
    [switch]$SkipAndroid,
    [switch]$SkipWindows
)

$ErrorActionPreference = "Stop"
$DevelopRoot = "C:\Users\charl\OneDrive\Dokumente\develop"

# Resolve script location: script lives in repo\scripts\, so RepoRoot = parent of scripts\
$_scriptFile = $MyInvocation.MyCommand.Path
if ($_scriptFile -and (Test-Path $_scriptFile)) {
    $_scriptDir = Split-Path $_scriptFile -Parent
} else {
    $_scriptDir = $PSScriptRoot
}
$RepoRoot = (Resolve-Path (Join-Path $_scriptDir "..")).Path
$AppRoot = Join-Path $RepoRoot "teenastro_app"
$ReleaseAppDir = Join-Path $RepoRoot "Released data\App"

if (-not (Test-Path (Join-Path $AppRoot "pubspec.yaml"))) {
    Write-Error "teenastro_app not found at $AppRoot"
}

Write-Host "AppRoot:  $AppRoot" -ForegroundColor Gray
Write-Host "Output:   $ReleaseAppDir" -ForegroundColor Gray

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

$androidOk = $false
$androidSkipped = $SkipAndroid
if (-not $SkipAndroid) {
    # Set JAVA_HOME to Android Studio JBR if not set (gradlew and Flutter need it)
    if (-not $env:JAVA_HOME -or -not (Test-Path (Join-Path $env:JAVA_HOME "bin\java.exe"))) {
        $jbrPaths = @(
            "${env:ProgramFiles}\Android\Android Studio\jbr",
            "${env:ProgramFiles}\Android\Android Studio1\jbr",
            "${env:ProgramFiles(x86)}\Android\Android Studio\jbr",
            "$env:LOCALAPPDATA\Programs\Android Studio\jbr"
        )
        foreach ($jbr in $jbrPaths) {
            if ($jbr -and (Test-Path (Join-Path $jbr "bin\java.exe"))) {
                $env:JAVA_HOME = $jbr
                break
            }
        }
    }
    # Set ANDROID_HOME from local.properties if not set (so Flutter finds the SDK)
    if (-not $env:ANDROID_HOME) {
        $lp = Join-Path $AppRoot "android\local.properties"
        if (Test-Path $lp) {
            $m = Get-Content $lp | Select-String 'sdk\.dir=(.+)'
            if ($m -and $m.Matches[0].Groups[1].Value) {
                $env:ANDROID_HOME = $m.Matches[0].Groups[1].Value.Trim().Replace('\\','\')
                $env:ANDROID_SDK_ROOT = $env:ANDROID_HOME
            }
        }
        if (-not $env:ANDROID_HOME -and (Test-Path "$env:LOCALAPPDATA\Android\Sdk")) {
            $env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk"
            $env:ANDROID_SDK_ROOT = $env:ANDROID_HOME
        }
    }
    Write-Host "`n=== Flutter doctor (Android) ===" -ForegroundColor Cyan
    flutter doctor -v
    Write-Host "`n=== Building Android APK ===" -ForegroundColor Cyan
    $prevErrAction = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { $apkOut = & flutter build apk 2>&1 } finally { $ErrorActionPreference = $prevErrAction }
    $apkExit = $LASTEXITCODE
    if ($apkExit -ne 0) {
        Write-Warning "Android build failed. Continuing with Windows build..."
        Write-Host "Last lines of build output:" -ForegroundColor Yellow
        ($apkOut | Select-Object -Last 20) | ForEach-Object { Write-Host $_ }
        $androidSkipped = $true
    } else {
        $androidOk = $true
        $apk = Join-Path $AppRoot "build\app\outputs\flutter-apk\app-release.apk"
        if (Test-Path $apk) {
            Write-Host "APK: $apk" -ForegroundColor Green
        }
    }
}

$windowsOk = $false
if (-not $SkipWindows) {
    Write-Host "`n=== Building Windows ===" -ForegroundColor Cyan
    flutter build windows
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Windows build failed. Ensure Visual Studio with 'Desktop development with C++' is installed."
    } else {
        $windowsOk = $true
        $winDir = Join-Path $AppRoot "build\windows\x64\runner\Release"
        if (Test-Path $winDir) {
            Write-Host "Windows build: $winDir" -ForegroundColor Green
            Get-ChildItem $winDir -Filter "*.exe" | ForEach-Object { Write-Host "  $($_.FullName)" }
        }
    }
}

# --- Copy outputs to Released data\App ---
if (-not (Test-Path $ReleaseAppDir)) {
    New-Item -ItemType Directory -Path $ReleaseAppDir -Force | Out-Null
}
$copied = $false
if ($androidOk) {
    $apkSrc = Join-Path $AppRoot "build\app\outputs\flutter-apk\app-release.apk"
    if (Test-Path $apkSrc) {
        $apkDest = Join-Path $ReleaseAppDir "teenastro_app-release.apk"
        Copy-Item -Path $apkSrc -Destination $apkDest -Force
        Write-Host "`nCopied Android APK to: $apkDest" -ForegroundColor Green
        $copied = $true
    }
}
if ($windowsOk) {
    $winSrc = Join-Path $AppRoot "build\windows\x64\runner\Release"
    if (Test-Path $winSrc) {
        $winDest = Join-Path $ReleaseAppDir "Windows"
        if (Test-Path $winDest) { Remove-Item -Recurse -Force $winDest }
        New-Item -ItemType Directory -Path $winDest -Force | Out-Null
        Copy-Item -Path "$winSrc\*" -Destination $winDest -Recurse -Force
        Write-Host "Copied Windows build to: $winDest" -ForegroundColor Green
        $copied = $true
    }
}
if ($copied) {
    Write-Host "All outputs are in: $ReleaseAppDir" -ForegroundColor Cyan
}

if (-not $androidSkipped -and -not $androidOk) { exit 1 }
if (-not $SkipWindows -and -not $windowsOk) { exit 1 }
Write-Host "`nDone." -ForegroundColor Green
