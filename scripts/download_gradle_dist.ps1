# Download Gradle distribution zip for the Android build (avoids Java SSL/trust-store issues).
# Run once from repo root: .\scripts\download_gradle_dist.ps1
# The zip is placed in teenastro_app\android\gradle\wrapper\ so the Gradle wrapper uses it.

$ErrorActionPreference = "Stop"
$url = "https://services.gradle.org/distributions/gradle-8.14-all.zip"
$name = "gradle-8.14-all.zip"

$_scriptFile = $MyInvocation.MyCommand.Path
$_scriptDir = if ($_scriptFile -and (Test-Path $_scriptFile)) { Split-Path $_scriptFile -Parent } else { $PSScriptRoot }
$RepoRoot = (Resolve-Path (Join-Path $_scriptDir "..")).Path
$destDir = Join-Path $RepoRoot "teenastro_app\android\gradle\wrapper"
$destFile = Join-Path $destDir $name

if (Test-Path $destFile) {
    Write-Host "Already present: $destFile" -ForegroundColor Green
    exit 0
}

Write-Host "Downloading $name to $destDir ..." -ForegroundColor Cyan
if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir -Force | Out-Null }

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $url -OutFile $destFile -UseBasicParsing
} catch {
    Write-Warning "Download failed: $_"
    Write-Host "Download the zip in your browser and save it as:" -ForegroundColor Yellow
    Write-Host "  $destFile" -ForegroundColor Yellow
    Write-Host "From: $url" -ForegroundColor Gray
    exit 1
}

Write-Host "Done. You can run: .\scripts\build_app.ps1" -ForegroundColor Green
