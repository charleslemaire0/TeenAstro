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

# 1. Find Python / pip (try pip, pip3, python -m pip, py -m pip)
$pipInvoke = $null
$pipLabel = $null
if (Get-Command pip -ErrorAction SilentlyContinue) {
    $pipInvoke = { & pip install --user platformio }
    $pipLabel = "pip"
}
elseif (Get-Command pip3 -ErrorAction SilentlyContinue) {
    $pipInvoke = { & pip3 install --user platformio }
    $pipLabel = "pip3"
}
elseif (Get-Command python -ErrorAction SilentlyContinue) {
    $pipInvoke = { & python -m pip install --user platformio }
    $pipLabel = "python -m pip"
}
elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $pipInvoke = { & py -m pip install --user platformio }
    $pipLabel = "py -m pip"
}

if (-not $pipInvoke) {
    Write-Host "Python / pip not found. Install Python and ensure it's on PATH:" -ForegroundColor Red
    Write-Host "  - https://www.python.org/downloads/ (check Add Python to PATH)" -ForegroundColor Gray
    Write-Host "  - Or install from Microsoft Store (search Python 3)" -ForegroundColor Gray
    Write-Host "Then reopen PowerShell and run this script again." -ForegroundColor Gray
    exit 1
}
Write-Host "[1/3] Using $pipLabel for PlatformIO..." -ForegroundColor Green

# 2. Install PlatformIO (ignore pip's stderr warnings e.g. "Scripts not on PATH" so they don't trigger -Stop)
$prevErrorAction = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
try {
    & $pipInvoke 2>&1 | Out-Null
} finally {
    $ErrorActionPreference = $prevErrorAction
}
$pipExit = $LASTEXITCODE
if ($pipExit -ne 0) {
    Write-Host "Failed to install PlatformIO. Try: $pipLabel install --user platformio" -ForegroundColor Red
    exit 1
}

# Find pio: PATH, then PlatformIO penv, then Python Scripts (Local and Roaming)
$pioCmd = $null
if (Get-Command pio -ErrorAction SilentlyContinue) { $pioCmd = "pio" }
if (-not $pioCmd) {
    $pyScripts = Join-Path $env:USERPROFILE ".platformio\penv\Scripts"
    $pioExe = Join-Path $pyScripts "pio.exe"
    if (Test-Path $pioExe) { $pioCmd = $pioExe }
}
if (-not $pioCmd) {
    $roamingScripts = Join-Path $env:APPDATA "Python\Python*\Scripts\pio.exe"
    $found = Get-Item $roamingScripts -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $pioCmd = $found.FullName }
}
if (-not $pioCmd) {
    $localScripts = Join-Path $env:LOCALAPPDATA "Programs\Python\Python*\Scripts\pio.exe"
    $found = Get-Item $localScripts -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $pioCmd = $found.FullName }
}
if (-not $pioCmd) {
    Write-Host "PlatformIO installed but 'pio' not found. Add Python Scripts to PATH, e.g.:" -ForegroundColor Yellow
    Write-Host "  $env:APPDATA\Python\Python314\Scripts" -ForegroundColor Gray
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

# 3. Add Python Scripts and MinGW to user PATH
$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$pathChanged = $false

if ($pioCmd -and $pioCmd -match '\\') {
    $pioDir = [System.IO.Path]::GetDirectoryName($pioCmd)
    if ($userPath -notlike "*$pioDir*") {
        $userPath = $userPath + ';' + $pioDir
        $pathChanged = $true
        Write-Host "[3/3] Adding Python Scripts to user PATH: $pioDir" -ForegroundColor Green
    }
}

$pioHome = Join-Path $env:USERPROFILE ".platformio"
$mingwBin = Join-Path $pioHome "packages\toolchain-gccmingw32\bin"
if (Test-Path $mingwBin) {
    if ($userPath -notlike "*$mingwBin*") {
        $userPath = $userPath + ';' + $mingwBin
        $pathChanged = $true
        Write-Host "[3/3] Adding MinGW to user PATH: $mingwBin" -ForegroundColor Green
    }
}
if ($pathChanged) {
    [Environment]::SetEnvironmentVariable('Path', $userPath, 'User')
}
if (-not $pathChanged -and (Test-Path $mingwBin)) {
    Write-Host "[3/3] Python Scripts and MinGW already on user PATH." -ForegroundColor Green
}
elseif (-not (Test-Path $mingwBin)) {
    Write-Host "[3/3] MinGW path not found; add it manually after first 'pio run' or 'pio test': $mingwBin" -ForegroundColor Yellow
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
Write-Host '  See BUILD_SETUP.md for .NET and Flutter builds.' -ForegroundColor White
