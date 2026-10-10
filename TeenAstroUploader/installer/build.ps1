# Build TeenAstro Firmware Uploader (Python) + MSI.
# Entry point for Released data\Run_FirmwareUploader_build.bat.
# Requires: Python 3.10+, WiX Toolset 3. MSI: .out\TeenAstroUploader.msi

$ErrorActionPreference = "Stop"

function Pause-IfInteractive {
    if ([Environment]::UserInteractive) {
        Write-Host "`nPress Enter to close this window..." -ForegroundColor Yellow
        $null = Read-Host
    }
}

function Find-Python {
    $candidates = @()
    try {
        $py = & py -3 -c "import sys; print(sys.executable)" 2>$null
        if ($LASTEXITCODE -eq 0 -and $py) { $candidates += $py.Trim() }
    } catch {}
    foreach ($name in @("python", "python3")) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { $candidates += $cmd.Source }
    }
    foreach ($p in $candidates) {
        if (-not (Test-Path $p)) { continue }
        # Skip Windows Store stub
        if ($p -match "WindowsApps") { continue }
        try {
            & $p -c "import tkinter" 2>$null
            if ($LASTEXITCODE -eq 0) { return $p }
        } catch {}
    }
    return $null
}

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

# ---------- Paths ----------
$ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$UploaderDir = Split-Path $ScriptDir -Parent
$RepoRoot = Split-Path $UploaderDir -Parent
$InstallerDir = $ScriptDir
$StageDir = Join-Path $InstallerDir ".stage"
$OutDir = Join-Path $InstallerDir ".out"
$PythonDir = Join-Path $UploaderDir "python"
$VenvDir = Join-Path $PythonDir ".venv"
$DistDir = Join-Path $PythonDir "dist\TeenAstroUploader"
$LegacyTools = Join-Path $UploaderDir "TeenAstroUploader\bin\Release"
$IconSrc = Join-Path $PythonDir "packaging\icon.ico"
if (-not (Test-Path $IconSrc)) {
    $IconSrc = Join-Path $RepoRoot "TeenAstroEmulator\installer\icon.ico"
}

Write-Host "Repo root  : $RepoRoot" -ForegroundColor Cyan
Write-Host "Installer  : $InstallerDir" -ForegroundColor Cyan
Write-Host "Python app : $PythonDir" -ForegroundColor Cyan

# ---------- Python venv + PyInstaller ----------
Write-Host "`n=== Building Python Uploader (PyInstaller) ===" -ForegroundColor Yellow
$python = Find-Python
if (-not $python) {
    Write-Host "Python 3 with tkinter not found. Install Python from https://www.python.org/ and retry." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Write-Host "Using Python: $python" -ForegroundColor Cyan

$venvPython = Join-Path $VenvDir "Scripts\python.exe"
if (-not (Test-Path $venvPython)) {
    Write-Host "Creating venv..." -ForegroundColor Gray
    & $python -m venv $VenvDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
& $venvPython -m pip install --upgrade pip
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $venvPython -m pip install -r (Join-Path $PythonDir "requirements-dev.txt")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Push-Location $PythonDir
try {
    if (Test-Path "build") { Remove-Item "build" -Recurse -Force }
    if (Test-Path "dist") { Remove-Item "dist" -Recurse -Force }
    & $venvPython -m PyInstaller --noconfirm "packaging\TeenAstroUploader.spec"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally { Pop-Location }

if (-not (Test-Path (Join-Path $DistDir "TeenAstroUploader.exe"))) {
    Write-Host "PyInstaller output not found at $DistDir" -ForegroundColor Red
    Pause-IfInteractive; exit 1
}

# ---------- Stage ----------
Write-Host "`n=== Staging files ===" -ForegroundColor Yellow
$null = New-Item -ItemType Directory -Force -Path $StageDir
Get-ChildItem $StageDir -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$DistDir\*" $StageDir -Recurse -Force

# Bundle PJRC Teensy tools from legacy VB Release build when available
$toolNames = @(
    "teensy.exe", "teensy_post_compile.exe", "teensy_reboot.exe",
    "teensy_restart.exe", "teensy_gateway.exe", "esptool.exe"
)
if (Test-Path $LegacyTools) {
    foreach ($name in $toolNames) {
        $src = Join-Path $LegacyTools $name
        if (Test-Path $src) {
            Copy-Item $src $StageDir -Force
            Write-Host "  Bundled tool: $name" -ForegroundColor Gray
        }
    }
} else {
    Write-Host "  Legacy tools folder not found ($LegacyTools). MSI will rely on teensy_loader_cli / python esptool." -ForegroundColor Yellow
}

if (-not (Test-Path $IconSrc)) {
    Write-Host "Icon not found at $IconSrc" -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Copy-Item $IconSrc (Join-Path $StageDir "icon.ico") -Force
Write-Host "Staged: $StageDir" -ForegroundColor Green

# ---------- WiX ----------
Write-Host "`n=== Building MSI ===" -ForegroundColor Yellow
$wixBin = Find-WixBin
if (-not $wixBin) {
    Write-Host "WiX Toolset 3 not found. Install from https://wixtoolset.org/ and add to PATH (or set WIX env var)." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
$null = New-Item -ItemType Directory -Force -Path $OutDir

Push-Location $InstallerDir
try {
    $heatExe = Join-Path $wixBin "heat.exe"
    $appWxs = Join-Path $OutDir "AppFiles_uploader.wxs"
    Write-Host "  Harvesting Firmware Uploader files with heat.exe..." -ForegroundColor Gray
    & $heatExe dir ".stage" -nologo -cg UploaderFiles -dr INSTALLFOLDER -srd -ke -gg -sfrag -sreg -template fragment -var "var.StageDir" -out $appWxs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Compiling WiX (candle)..." -ForegroundColor Gray
    & (Join-Path $wixBin "candle.exe") -nologo -out "$OutDir\" -dStageDir=".stage" "TeenAstroUploader_only.wxs" ".out\AppFiles_uploader.wxs"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "  Linking MSI (light)..." -ForegroundColor Gray
    $msiOut = Join-Path $OutDir "TeenAstroUploader.msi"
    & (Join-Path $wixBin "light.exe") -nologo -out $msiOut -b "." ".out\TeenAstroUploader_only.wixobj" ".out\AppFiles_uploader.wixobj"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "`nMSI created: $msiOut" -ForegroundColor Green
} finally { Pop-Location }
