# Build TeenAstro Firmware Uploader MSI. Entry point for Run_FirmwareUploader_build.bat.
# Requires: MSBuild, WiX Toolset 3. MSI output: .out\TeenAstroUploader.msi (relative to this installer dir).
# Run from repo root or any dir; script resolves paths.

$ErrorActionPreference = "Stop"

function Pause-IfInteractive {
    if ([Environment]::UserInteractive) {
        Write-Host "`nPress Enter to close this window..." -ForegroundColor Yellow
        $null = Read-Host
    }
}

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
$UploaderProj = Join-Path $RepoRoot "TeenAstroUploader\TeenAstroUploader\TeenAstroUploader.vbproj"
$UploaderBin = Join-Path $RepoRoot "TeenAstroUploader\TeenAstroUploader\bin\Release"
$IconSrc = Join-Path $RepoRoot "TeenAstroEmulator\installer\icon.ico"

Write-Host "Repo root  : $RepoRoot" -ForegroundColor Cyan
Write-Host "Installer  : $InstallerDir" -ForegroundColor Cyan

# ---------- Build Firmware Uploader ----------
Write-Host "`n=== Building Firmware Uploader ===" -ForegroundColor Yellow
$msbuild = Find-MSBuild
if (-not $msbuild) {
    Write-Host "MSBuild not found. Install Visual Studio or .NET Framework SDK." -ForegroundColor Red
    Pause-IfInteractive; exit 1
}
Write-Host "Using MSBuild: $msbuild" -ForegroundColor Cyan
& $msbuild $UploaderProj /p:Configuration=Release /v:minimal
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path $UploaderBin)) {
    Write-Host "Firmware Uploader build output not found at $UploaderBin" -ForegroundColor Red
    Pause-IfInteractive; exit 1
}

# ---------- Stage ----------
Write-Host "`n=== Staging files ===" -ForegroundColor Yellow
$null = New-Item -ItemType Directory -Force -Path $StageDir
Get-ChildItem $StageDir -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$UploaderBin\*" $StageDir -Recurse -Force
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
