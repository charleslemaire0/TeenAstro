# Configure Android SDK for building the TeenAstro Flutter app.
#
# Usage (from repo root or scripts\):
#   .\setup_android_sdk.ps1 -Check                    # Verify SDK path and components
#   .\setup_android_sdk.ps1 -SdkPath "C:\path\to\sdk"  # Set ANDROID_HOME and local.properties
#   .\setup_android_sdk.ps1 -SdkPath "C:\path\to\sdk" -Install  # Also install platform-tools, build-tools, platform
#
# See scripts\ANDROID_SETUP.md for full setup options (Android Studio vs command-line).

param(
    [switch]$Check,
    [string]$SdkPath,
    [switch]$Install
)

$_scriptFile = $MyInvocation.MyCommand.Path
if ($_scriptFile -and (Test-Path $_scriptFile)) {
    $_scriptDir = Split-Path $_scriptFile -Parent
} else {
    $_scriptDir = $PSScriptRoot
}
$RepoRoot = (Resolve-Path (Join-Path $_scriptDir "..")).Path
$AppRoot = Join-Path $RepoRoot "teenastro_app"
$LocalProps = Join-Path $AppRoot "android\local.properties"

function Get-CurrentSdkPath {
    $ah = [Environment]::GetEnvironmentVariable("ANDROID_HOME", "User")
    if (-not $ah) { $ah = [Environment]::GetEnvironmentVariable("ANDROID_HOME", "Process") }
    if ($ah) { return $ah }
    if (Test-Path $LocalProps) {
        $line = Get-Content $LocalProps | Select-String 'sdk\.dir'
        if ($line -match 'sdk\.dir=(.+)') {
            return $Matches[1].Trim().Replace('\\', '\')
        }
    }
    if (Test-Path "$env:LOCALAPPDATA\Android\Sdk") {
        return "$env:LOCALAPPDATA\Android\Sdk"
    }
    return $null
}

function Test-SdkComponents {
    param([string]$Root)
    $ok = $true
    if (-not (Test-Path (Join-Path $Root "platform-tools\adb.exe"))) {
        Write-Host "  Missing: platform-tools (no platform-tools\adb.exe)" -ForegroundColor Yellow
        $ok = $false
    } else {
        Write-Host "  OK: platform-tools" -ForegroundColor Green
    }
    $bt = Get-ChildItem -Path (Join-Path $Root "build-tools") -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $bt) {
        Write-Host "  Missing: build-tools (no build-tools\* version folder)" -ForegroundColor Yellow
        $ok = $false
    } else {
        Write-Host "  OK: build-tools ($($bt.Name))" -ForegroundColor Green
    }
    $pl = Get-ChildItem -Path (Join-Path $Root "platforms") -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $pl) {
        Write-Host "  Missing: platforms (no platforms\android-* folder)" -ForegroundColor Yellow
        $ok = $false
    } else {
        Write-Host "  OK: platforms ($($pl.Name))" -ForegroundColor Green
    }
    return $ok
}

if ($Check) {
    Write-Host "=== Android SDK check ===" -ForegroundColor Cyan
    $sdk = Get-CurrentSdkPath
    if (-not $sdk) {
        Write-Host "ANDROID_HOME is not set and no sdk.dir in local.properties." -ForegroundColor Red
        Write-Host "Run with -SdkPath ""C:\path\to\android\sdk"" or see scripts\ANDROID_SETUP.md" -ForegroundColor Gray
        exit 1
    }
    Write-Host "SDK root: $sdk" -ForegroundColor Gray
    if (-not (Test-Path $sdk)) {
        Write-Host "Path does not exist." -ForegroundColor Red
        exit 1
    }
    $allOk = Test-SdkComponents -Root $sdk
    Write-Host ""
    if ($allOk) {
        Write-Host "SDK looks ready. Run: .\scripts\build_app.ps1" -ForegroundColor Green
    } else {
        Write-Host "Install missing components (Android Studio SDK Manager or sdkmanager). See scripts\ANDROID_SETUP.md" -ForegroundColor Yellow
        exit 1
    }
    exit 0
}

if ($SdkPath) {
    $SdkPath = $SdkPath.TrimEnd("\")
    if (-not (Test-Path $SdkPath)) {
        Write-Host "Path does not exist: $SdkPath" -ForegroundColor Red
        exit 1
    }
    Write-Host "Setting ANDROID_HOME and ANDROID_SDK_ROOT to: $SdkPath" -ForegroundColor Cyan
    [Environment]::SetEnvironmentVariable("ANDROID_HOME", $SdkPath, "User")
    [Environment]::SetEnvironmentVariable("ANDROID_SDK_ROOT", $SdkPath, "User")
    $env:ANDROID_HOME = $SdkPath
    $env:ANDROID_SDK_ROOT = $SdkPath

    # Update local.properties (sdk.dir with escaped backslashes for Java)
    $sdkDirValue = $SdkPath.Replace("\", "\\")
    $content = @()
    if (Test-Path $LocalProps) {
        $content = Get-Content $LocalProps
        $hasSdkDir = $false
        $content = $content | ForEach-Object {
            if ($_ -match '^sdk\.dir=') { $hasSdkDir = $true; "sdk.dir=$sdkDirValue" } else { $_ }
        }
        if (-not $hasSdkDir) { $content += "sdk.dir=$sdkDirValue" }
    } else {
        $content = @("sdk.dir=$sdkDirValue")
    }
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllLines($LocalProps, $content, $utf8NoBom)
    Write-Host "Updated $LocalProps" -ForegroundColor Green

    if ($Install) {
        $sdkmanager = Join-Path $SdkPath "cmdline-tools\latest\bin\sdkmanager.bat"
        if (-not (Test-Path $sdkmanager)) {
            Write-Host "sdkmanager not found at $sdkmanager. Install command-line tools (see ANDROID_SETUP.md) or use Android Studio." -ForegroundColor Yellow
            exit 0
        }
        Write-Host "Installing platform-tools, build-tools;34.0.0, platforms;android-34..." -ForegroundColor Cyan
        & $sdkmanager --sdk_root=$SdkPath --install "platform-tools" "build-tools;34.0.0" "platforms;android-34"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "sdkmanager failed. Run it manually and accept licenses if prompted." -ForegroundColor Yellow
            exit 1
        }
        Write-Host "Done. Run: .\scripts\setup_android_sdk.ps1 -Check" -ForegroundColor Green
    }

    Write-Host "Close and reopen PowerShell for ANDROID_HOME to apply in new windows, then run: .\scripts\build_app.ps1" -ForegroundColor Gray
    exit 0
}

# No -Check or -SdkPath: show usage
Write-Host "Usage:" -ForegroundColor Cyan
Write-Host "  .\setup_android_sdk.ps1 -Check"
Write-Host "  .\setup_android_sdk.ps1 -SdkPath ""C:\path\to\sdk"" [ -Install ]"
Write-Host "See scripts\ANDROID_SETUP.md for full setup steps."
