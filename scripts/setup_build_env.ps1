# TeenAstro build environment setup (Windows)
# Installs PlatformIO via pip and MinGW toolchain for native unit tests,
# and adds MinGW to the user PATH.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Resolve the real script directory robustly ($PSScriptRoot can be wrong in Cursor/temp-copy terminals)
$_scriptFile = $MyInvocation.MyCommand.Path
if ($_scriptFile -and (Test-Path $_scriptFile)) {
    $_scriptDir = Split-Path $_scriptFile -Parent
} else {
    $_scriptDir = $PSScriptRoot
}

# Repo root: if run from scripts\ it's the parent; if run from repo root it's here
$RepoRoot = if (Test-Path (Join-Path $_scriptDir "..\TeenAstroMainUnit\platformio.ini")) {
    (Resolve-Path (Join-Path $_scriptDir "..")).Path
} elseif (Test-Path (Join-Path $_scriptDir "TeenAstroMainUnit\platformio.ini")) {
    $_scriptDir
} else {
    $_scriptDir
}
if (-not (Test-Path (Join-Path $RepoRoot "TeenAstroMainUnit\platformio.ini"))) {
    Write-Host "Cannot locate repo root (TeenAstroMainUnit\platformio.ini not found)." -ForegroundColor Red
    Write-Host "Run this script from the repo root or from the scripts\ subfolder." -ForegroundColor Red
    exit 1
}

Write-Host "TeenAstro build setup (PlatformIO + MinGW)" -ForegroundColor Cyan
Write-Host "Repo root: $RepoRoot" -ForegroundColor Gray
Write-Host ""

# Helper: resolve a command only if it points to a real file on disk
function Resolve-Exe {
    param([string]$Name)
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd -and ($cmd.CommandType -eq 'Application') -and (Test-Path $cmd.Source)) { return $cmd.Source }
    return $null
}

# 1. Find Python / pip - prefer python/py (confirmed working) over bare pip
# Try python -m pip, py -m pip first, then pip/pip3 if they resolve to real files
$pipInvoke = $null
$pipLabel = $null
$pythonExe = Resolve-Exe 'python'
$pyExe     = Resolve-Exe 'py'
$pipExe    = Resolve-Exe 'pip'
$pip3Exe   = Resolve-Exe 'pip3'

if ($pythonExe) {
    $pipInvoke = [scriptblock]::Create("& '$pythonExe' -m pip install --user platformio")
    $pipLabel = "python -m pip"
}
elseif ($pyExe) {
    $pipInvoke = [scriptblock]::Create("& '$pyExe' -m pip install --user platformio")
    $pipLabel = "py -m pip"
}
elseif ($pipExe) {
    $pipInvoke = [scriptblock]::Create("& '$pipExe' install --user platformio")
    $pipLabel = "pip"
}
elseif ($pip3Exe) {
    $pipInvoke = [scriptblock]::Create("& '$pip3Exe' install --user platformio")
    $pipLabel = "pip3"
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

$pioInstalled = Test-Path (Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe")
$mingwInstalled = Test-Path (Join-Path $env:USERPROFILE ".platformio\packages\toolchain-gccmingw32\bin\g++.exe")

Write-Host ""
Write-Host "Installed tools:" -ForegroundColor Cyan
Write-Host "  pio.exe   : $(if ($pioInstalled) { 'OK - ' + (Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe') } else { 'NOT FOUND' })" -ForegroundColor $(if ($pioInstalled) { 'Green' } else { 'Red' })
Write-Host "  g++.exe   : $(if ($mingwInstalled) { 'OK - ' + (Join-Path $env:USERPROFILE '.platformio\packages\toolchain-gccmingw32\bin\g++.exe') } else { 'NOT FOUND' })" -ForegroundColor $(if ($mingwInstalled) { 'Green' } else { 'Red' })
Write-Host ""
Write-Host "NOTE: This script installs build TOOLS only. It does NOT compile firmware." -ForegroundColor Yellow
Write-Host ""
Write-Host "Next steps to build firmware:" -ForegroundColor Cyan
Write-Host "  1. Close and reopen PowerShell (so updated PATH takes effect)." -ForegroundColor White
Write-Host "  2. From the repo root run one of:" -ForegroundColor White
Write-Host "     pio run -d TeenAstroMainUnit   -> firmware in TeenAstroMainUnit\pio\" -ForegroundColor Gray
Write-Host "     pio run -d TeenAstroSHC        -> firmware in TeenAstroSHC\pio\" -ForegroundColor Gray
Write-Host "     pio run -d TeenAstroServer     -> firmware in TeenAstroServer\pio\" -ForegroundColor Gray
Write-Host "     pio run -d TeenAstroFocuser    -> firmware in TeenAstroFocuser\pio\" -ForegroundColor Gray
Write-Host "     pio test -d tests              -> run unit tests" -ForegroundColor Gray
Write-Host '  See BUILD_SETUP.md for .NET (ASCOM driver) and Flutter (Android/Windows app) builds.' -ForegroundColor White
