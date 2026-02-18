# Set ANDROID_HOME for Flutter Android builds when using Android Studio
# Run this after you know your Android SDK path (see BUILD_SETUP.md).

param(
    [Parameter(Mandatory=$false)]
    [string]$SdkPath
)

if (-not $SdkPath) {
    Write-Host "Usage: .\scripts\set_android_sdk.ps1 -SdkPath 'C:\path\to\Android\Sdk'" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To find your Android SDK path:" -ForegroundColor Yellow
    Write-Host "  1. Open Android Studio (Panda)" -ForegroundColor White
    Write-Host "  2. More Actions / Settings (Ctrl+Alt+S)" -ForegroundColor White
    Write-Host "  3. Appearance & Behavior -> System Settings -> Android SDK" -ForegroundColor White
    Write-Host "  4. Copy the path shown as 'Android SDK Location'" -ForegroundColor White
    Write-Host "  5. Run: .\scripts\set_android_sdk.ps1 -SdkPath 'YOUR_PATH'" -ForegroundColor White
    exit 1
}

if (-not (Test-Path $SdkPath)) {
    Write-Host "Error: Path not found: $SdkPath" -ForegroundColor Red
    exit 1
}

$platformTools = Join-Path $SdkPath "platform-tools"
if (-not (Test-Path $platformTools)) {
    Write-Host "Warning: platform-tools not found. Install Android SDK components in Android Studio (SDK Manager)." -ForegroundColor Yellow
}

[Environment]::SetEnvironmentVariable("ANDROID_HOME", $SdkPath, "User")
[Environment]::SetEnvironmentVariable("ANDROID_SDK_ROOT", $SdkPath, "User")
Write-Host "Set ANDROID_HOME = $SdkPath" -ForegroundColor Green
Write-Host "Close and reopen the terminal, then run: flutter doctor -v" -ForegroundColor Cyan
Write-Host "Accept licenses if needed: flutter doctor --android-licenses" -ForegroundColor Cyan
Write-Host "Build APK: flutter build apk" -ForegroundColor Cyan
