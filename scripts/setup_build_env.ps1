# TeenAstro build environment setup (Windows)
# Installs PlatformIO via pip and MinGW toolchain for native unit tests,
# and adds MinGW to the user PATH.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Repo root: when run from scripts\ it's parent of PSScriptRoot
$RepoRoot = if (Test-Path (Join-Path $PSScriptRoot "..\TeenAstroMainUnit\platformio.ini")) { (Resolve-Path (Join-Path $PSScriptRoot "..")).Path } else { $PSScriptRoot }
if (-not (Test-Path (Join-Path $RepoRoot "TeenAstroMainUnit\platformio.ini"))) {
    Write-Host "Run this script from the TeenAstro repo root or from scripts\." -ForegroundColor Red
    exit 1
}

Write-Host "TeenAstro build setup (PlatformIO + MinGW)" -ForegroundColor Cyan
Write-Host "Repo root: $RepoRoot" -ForegroundColor Gray
Write-Host ""

# 1. Check Python / pip
$pip = $null
if (Get-Command pip -ErrorAction SilentlyContinue) { $pip = "pip" }
elseif (Get-Command pip3 -ErrorAction SilentlyContinue) { $pip = "pip3" }
if (-not $pip) {
    Write-Host "Python pip not found. Install Python from https://www.python.org/ and ensure 'Add Python to PATH' is checked." -ForegroundColor Red
    exit 1
}
Write-Host "[1/3] Using $pip for PlatformIO..." -ForegroundColor Green

# 2. Install PlatformIO
& $pip install --user platformio 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to install PlatformIO. Try: $pip install --user platformio" -ForegroundColor Red
    exit 1
}
$pioCmd = $null
if (Get-Command pio -ErrorAction SilentlyContinue) { $pioCmd = "pio" }
else {
    $pyScripts = [Environment]::GetFolderPath("UserProfile") + "\.platformio\penv\Scripts"
    $pioExe = Join-Path $pyScripts "pio.exe"
    if (Test-Path $pioExe) { $pioCmd = $pioExe }
    else {
        $localAppData = [Environment]::GetFolderPath("LocalApplicationData")
        $pipBin = Join-Path $localAppData "Programs\Python\Python*\Scripts\pio.exe"
        $found = Get-Item $pipBin -ErrorAction SilentlyContinue
        if ($found) { $pioCmd = $found[0].FullName }
    }
}
if (-not $pioCmd) {
    Write-Host "PlatformIO installed but 'pio' not found. Add Python Scripts to PATH, e.g.:" -ForegroundColor Yellow
    Write-Host "  $env:USERPROFILE\AppData\Local\Programs\Python\Python3XX\Scripts" -ForegroundColor Gray
    Write-Host "Then reopen PowerShell and run this script again, or run: pio pkg install -g --tool platformio/toolchain-gccmingw32" -ForegroundColor Yellow
}
else {
    Write-Host "[2/3] Installing MinGW toolchain for native tests (this may take a minute)..." -ForegroundColor Green
    Push-Location $RepoRoot
    try {
        & $pioCmd pkg install -g --tool "platformio/toolchain-gccmingw32"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "MinGW install failed. You can run later: pio pkg install -g --tool platformio/toolchain-gccmingw32" -ForegroundColor Yellow
        }
    }
    finally {
        Pop-Location
    }
}

# 3. Add MinGW to user PATH
$pioHome = Join-Path $env:USERPROFILE ".platformio"
$mingwBin = Join-Path $pioHome "packages\toolchain-gccmingw32\bin"
if (Test-Path $mingwBin) {
    $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
    if ($userPath -notlike "*$mingwBin*") {
        [Environment]::SetEnvironmentVariable("Path", $userPath + ";" + $mingwBin, "User")
        Write-Host "[3/3] Added MinGW to user PATH: $mingwBin" -ForegroundColor Green
    }
    else {
        Write-Host "[3/3] MinGW already on user PATH." -ForegroundColor Green
    }
}
else {
    Write-Host "[3/3] MinGW path not found; add it manually after first 'pio run' or 'pio test':" -ForegroundColor Yellow
    Write-Host "  $mingwBin" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Setup done. Next steps:" -ForegroundColor Cyan
Write-Host "  1. Close and reopen PowerShell (so PATH is updated)." -ForegroundColor White
Write-Host "  2. From repo root run:" -ForegroundColor White
Write-Host "     pio run -d TeenAstroMainUnit" -ForegroundColor Gray
Write-Host "     pio run -d TeenAstroSHC" -ForegroundColor Gray
Write-Host "     pio run -d TeenAstroServer" -ForegroundColor Gray
Write-Host "     pio run -d TeenAstroFocuser" -ForegroundColor Gray
Write-Host "     pio test -d tests" -ForegroundColor Gray
Write-Host "  See BUILD_SETUP.md for .NET and Flutter builds." -ForegroundColor White
