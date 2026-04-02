# =============================================================================
# TeenAstro ASCOM — build, sign, Inno installer, publish to Released data\driver
#
# Order (when signing is enabled):
#   1) MSBuild Release  ->  ASCOM.TeenAstro.exe (+ tests, etc.)
#   2) Sign the driver EXE (Authenticode)
#   3) Inno (ISS)       ->  TeenAstro Setup 1.6.exe next to the solution output
#   4) Sign the setup EXE (Authenticode) + signtool verify
#   5) Copy TeenAstro Setup 1.6.exe into Released data\driver\ (merges; does not delete that folder). Optional: -CopyBinToRelease.
#
#   -SkipSign            : steps 2 and 4 skipped (unsigned binaries)
#   -SkipCopyToRelease   : step 5 skipped
#   -SkipBuild           : step 1 skipped (Release output must already exist)
#   -NoTimestamp         : never use a timestamp server (skips RFC 3161; use if you prefer one attempt only)
#                         If timestamp fails, the script also auto-retries signing without a timestamp unless -NoTimestamp is set.
#
# PFX password: TEENASTRO_PFX_PASSWORD, -PfxPasswordFile, {PfxPath}.password.txt, or prompt.
# Must stay in sync: TeenAstro Setup.iss [Setup] OutputBaseFilename + ".exe"  ==  $SetupInstallerExeName below.
# =============================================================================

param(
    [switch]$SkipBuild,
    [string]$Configuration = "Release",
    [string]$ISCC = "",
    [switch]$SkipSign,
    [string]$PfxPath = "C:\Users\charl\TeenAstroCodeSigning.pfx",
    [string]$PfxPasswordFile = "",
    [switch]$NoAutoPasswordFile,
    [System.Security.SecureString]$PfxPassword,
    [string]$TimestampUrl = "http://timestamp.digicert.com",
    [string]$SignToolPath = "",
    [switch]$NoTimestamp,
    [switch]$SkipTrustRoot,
    [switch]$ForceTrustSigningCert,
    [switch]$SkipCopyToRelease,
    [string]$ReleaseSubdir = "Released data\driver",
    [switch]$NonInteractive,
    [switch]$CopyBinToRelease
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..")).Path
. (Join-Path $ScriptDir "AuthenticodeSigning.ps1")

$Sln = Join-Path $RepoRoot "TeenAstroASCOM_V7\TeenAstroASCOM_V7.sln"
$Iss = Join-Path $RepoRoot "TeenAstroASCOM_V7\TeenAstroASCOM_V7\TeenAstro Setup.iss"
$AscomBin = Join-Path $RepoRoot "TeenAstroASCOM_V7\TeenAstroASCOM_V7\bin\$Configuration"
$DriverExe = Join-Path $AscomBin "ASCOM.TeenAstro.exe"
# Same base name as [Setup] OutputBaseFilename in TeenAstro Setup.iss (Inno adds .exe)
$SetupInstallerExeName = "TeenAstro Setup 1.6.exe"
$SetupExeBuilt = Join-Path $RepoRoot "TeenAstroASCOM_V7\$SetupInstallerExeName"
$DriverReleaseDir = Join-Path $RepoRoot $ReleaseSubdir

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

function Find-ISCC {
    param([string]$Explicit)
    if ($Explicit -and (Test-Path -LiteralPath $Explicit)) { return (Resolve-Path $Explicit).Path }
    foreach ($p in @(
            "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
            "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
            "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe",
            "${env:ProgramFiles}\Inno Setup 5\ISCC.exe")) {
        if (Test-Path -LiteralPath $p) { return $p }
    }
    return $null
}

function Convert-SecureStringToPlain {
    param([System.Security.SecureString]$Secure)
    if (-not $Secure) { return $null }
    $BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($Secure)
    try { return [System.Runtime.InteropServices.Marshal]::PtrToStringUni($BSTR) }
    finally { [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($BSTR) }
}

function Publish-DriverRelease {
    param(
        [string]$DriverBin,
        [string]$SetupSourcePath,
        [string]$DestinationDir,
        [string]$SetupFileName,
        [switch]$IncludeBinFolder
    )
    Write-Host "Publishing to $DestinationDir ..." -ForegroundColor Yellow
    # Do not remove the directory: preserve sibling folders (e.g. Archiv) and other hand-placed files.
    if (-not (Test-Path -LiteralPath $DestinationDir)) {
        New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null
    }
    if ($IncludeBinFolder) {
        Copy-Item -Path (Join-Path $DriverBin "*") -Destination $DestinationDir -Recurse -Force
    }
    $destSetup = Join-Path $DestinationDir $SetupFileName
    if (Test-Path -LiteralPath $destSetup) {
        Remove-Item -LiteralPath $destSetup -Force -ErrorAction Stop
    }
    Copy-Item -LiteralPath $SetupSourcePath -Destination $destSetup -Force -ErrorAction Stop
    Write-Host "Replaced: $destSetup" -ForegroundColor Green
}

# --- PFX / signtool (only if signing) -------------------------------------------------
$pwdPlain = $null
$signTool = $null

if (-not $SkipSign) {
    if (-not (Test-Path -LiteralPath $PfxPath)) {
        Write-Error "PFX not found: $PfxPath (use -SkipSign for unsigned build)"
        exit 1
    }
    $signTool = Get-SignToolPath -SignToolPath $SignToolPath
    if (-not $signTool) {
        Write-Error "signtool.exe not found. Install Windows SDK signing tools, or pass -SignToolPath '...\signtool.exe'"
        exit 1
    }
    Write-Host "Using signtool: $signTool" -ForegroundColor Gray

    $pwdPlain = Get-PfxPasswordPlain -SecurePassword $PfxPassword -PfxPath $PfxPath -PasswordFile $PfxPasswordFile -SkipAutoPasswordFile:$NoAutoPasswordFile
    if ($null -eq $pwdPlain) {
        if ($NonInteractive) {
            $autoHint = $PfxPath + ".password.txt"
            Write-Error @"
No PFX password in non-interactive mode. Use sidecar $autoHint, -PfxPasswordFile, TEENASTRO_PFX_PASSWORD, or omit -NonInteractive.
"@
            exit 1
        }
        $sec = Read-Host -AsSecureString "Enter PFX password"
        $pwdPlain = Convert-SecureStringToPlain -Secure $sec
    }

    $probeFiles = @()
    if ($PfxPasswordFile -and (Test-Path -LiteralPath $PfxPasswordFile)) { $probeFiles += $PfxPasswordFile }
    $autoSidecar = $PfxPath + ".password.txt"
    if (($autoSidecar -notin $probeFiles) -and (Test-Path -LiteralPath $autoSidecar)) { $probeFiles += $autoSidecar }

    try {
        $pwdPlain = Resolve-PfxPasswordAgainstCertificate -PfxPath $PfxPath -InitialPlain $pwdPlain -PasswordFilesToProbe $probeFiles
    }
    catch {
        if ($NonInteractive) {
            Write-Error $_
            exit 1
        }
        Write-Warning $_
        Write-Host "Enter PFX password (keyboard; not echoed):" -ForegroundColor Yellow
        $sec = Read-Host -AsSecureString "PFX password"
        $pwdPlain = Convert-SecureStringToPlain -Secure $sec
        if (-not (Test-X509PfxOpens -PfxPath $PfxPath -PasswordPlain $pwdPlain)) {
            Write-Error "That password did not open the PFX: $PfxPath"
            exit 1
        }
    }

    if (-not $SkipTrustRoot) {
        try {
            Install-SelfSignedSignerIntoCurrentUserTrustStores -PfxPath $PfxPath -PasswordPlain $pwdPlain -ForceTrust:([bool]$ForceTrustSigningCert)
        }
        catch {
            $hint = ""
            if ("$_" -match 'password|network password|not correct') {
                $hint = "`n(Try -SkipTrustRoot for a public CA certificate.)"
            }
            Write-Error "Trust store setup failed: $_$hint"
            exit 1
        }
    }
}

# --- 1) Build -------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $Iss)) {
    Write-Error "Inno script not found: $Iss"
    exit 1
}

if (-not $SkipBuild) {
    $msb = Find-MSBuild
    if (-not $msb) {
        Write-Error "MSBuild not found. Install Visual Studio Build Tools or pass -SkipBuild."
        exit 1
    }
    Write-Host "`n[1/5] MSBuild: $msb" -ForegroundColor Cyan
    Write-Host "Building $Configuration ..." -ForegroundColor Yellow
    & $msb $Sln /p:Configuration=$Configuration /v:minimal
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
else {
    Write-Host "`n[1/5] Skipped (-SkipBuild)" -ForegroundColor DarkGray
}

if (-not (Test-Path -LiteralPath $DriverExe)) {
    Write-Error "Driver EXE missing: $DriverExe"
    exit 1
}

# --- 2) Sign built driver EXE -----------------------------------------------------------
if (-not $SkipSign) {
    Write-Host "`n[2/5] Signing driver: $DriverExe" -ForegroundColor Cyan
    $rc = Invoke-AuthenticodeSign -SignToolExe $signTool -FilePath $DriverExe -PfxPath $PfxPath -PasswordPlain $pwdPlain -TimestampUrl $TimestampUrl -NoTimestamp:$NoTimestamp
    if ($rc -ne 0) {
        Write-Host @"

Driver signing failed. Common causes:
  - Timestamp server blocked or slow: run again with -NoTimestamp
  - Wrong PFX password (re-run and check)
  - File locked (close apps using $DriverExe)

"@ -ForegroundColor Yellow
        exit $rc
    }
}

# --- 3) Inno compiler (ISS) -----------------------------------------------------------
$isccExe = Find-ISCC -Explicit $ISCC
if (-not $isccExe) {
    Write-Error "Inno Setup (ISCC.exe) not found. Install from https://jrsoftware.org/isinfo.php or pass -ISCC."
    exit 1
}

Write-Host "`n[3/5] Inno: $isccExe" -ForegroundColor Cyan
Write-Host "Compiling TeenAstro Setup.iss ..." -ForegroundColor Yellow
& $isccExe $Iss
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path -LiteralPath $SetupExeBuilt)) {
    Write-Error "Inno did not produce: $SetupExeBuilt (check OutputBaseFilename in TeenAstro Setup.iss)"
    exit 1
}
Write-Host "Built installer: $SetupExeBuilt" -ForegroundColor Green

# --- 4) Sign setup EXE from Inno ------------------------------------------------------
if (-not $SkipSign) {
    Write-Host "`n[4/5] Signing installer: $SetupExeBuilt" -ForegroundColor Cyan
    $rc = Invoke-AuthenticodeSign -SignToolExe $signTool -FilePath $SetupExeBuilt -PfxPath $PfxPath -PasswordPlain $pwdPlain -TimestampUrl $TimestampUrl -NoTimestamp:$NoTimestamp
    if ($rc -ne 0) {
        $pwdPlain = $null
        Write-Host "Installer signing failed. Try -NoTimestamp if the timestamp step failed or timed out." -ForegroundColor Yellow
        exit $rc
    }
    $rc = Invoke-AuthenticodeVerify -SignToolExe $signTool -FilePath $SetupExeBuilt
    $pwdPlain = $null
    if ($rc -ne 0) { exit $rc }
    Write-Host "Installer signature verified." -ForegroundColor Green
}
else {
    if ($pwdPlain) { $pwdPlain = $null }
}

# --- 5) Copy installer to Released data\driver --------------------------------------
if (-not $SkipCopyToRelease) {
    Write-Host "`n[5/5] Publishing release folder" -ForegroundColor Cyan
    Publish-DriverRelease -DriverBin $AscomBin -SetupSourcePath $SetupExeBuilt -DestinationDir $DriverReleaseDir -SetupFileName $SetupInstallerExeName -IncludeBinFolder:$CopyBinToRelease
    if ($CopyBinToRelease) {
        Write-Host "Done. bin\Release\* + $SetupInstallerExeName -> $DriverReleaseDir" -ForegroundColor Green
    }
    else {
        Write-Host "Done. $SetupInstallerExeName -> $DriverReleaseDir (installer only; use -CopyBinToRelease for full bin\Release)" -ForegroundColor Green
    }
}
else {
    Write-Host "`n[5/5] Skipped (-SkipCopyToRelease). Installer left at: $SetupExeBuilt" -ForegroundColor Yellow
}
