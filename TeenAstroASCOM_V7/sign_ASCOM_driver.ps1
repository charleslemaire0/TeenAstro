param(
  [string]$PfxPath = "C:\Users\charl\TeenAstroCodeSigning.pfx",
  [string]$TimestampUrl = "http://timestamp.sectigo.com",
  [string]$SignToolPath = "",
  [switch]$NoTimestamp,
  [switch]$SkipTrustRoot,
  [switch]$ForceTrustSigningCert
)

# Self-signed certs are not in the Trusted Root store -> SignTool fails with:
# "terminated in a root certificate which is not trusted by the trust provider"
function Test-X509SelfSigned {
  param([System.Security.Cryptography.X509Certificates.X509Certificate2]$Cert)
  # String compare fails when DN field order/format differs; RawData of names is authoritative.
  $s = $Cert.SubjectName.RawData
  $i = $Cert.IssuerName.RawData
  if ($s.Length -ne $i.Length) { return $false }
  for ($k = 0; $k -lt $s.Length; $k++) { if ($s[$k] -ne $i[$k]) { return $false } }
  return $true
}

function Add-PublicCertToCurrentUserStoreIfMissing {
  param(
    [System.Security.Cryptography.X509Certificates.X509Certificate2]$PublicCert,
    [System.Security.Cryptography.X509Certificates.StoreName]$StoreName
  )
  $store = New-Object System.Security.Cryptography.X509Certificates.X509Store(
    $StoreName,
    [System.Security.Cryptography.X509Certificates.StoreLocation]::CurrentUser)
  $store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
  try {
    $existing = $store.Certificates.Find(
      [System.Security.Cryptography.X509Certificates.X509FindType]::FindByThumbprint,
      $PublicCert.Thumbprint,
      $false)
    if ($existing.Count -gt 0) {
      Write-Host "  Already present in $StoreName : $($PublicCert.Thumbprint)"
      return
    }
    $store.Add($PublicCert)
    Write-Host "  Added to Current User $StoreName : $($PublicCert.Thumbprint)"
  }
  finally {
    $store.Close()
  }
}

function Install-SelfSignedSignerIntoCurrentUserTrustStores {
  param([string]$PfxPath, [string]$PasswordPlain, [bool]$ForceTrust)
  $flags = [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::UserKeySet
  $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($PfxPath, $PasswordPlain, $flags)
  try {
    $isSelfSigned = (Test-X509SelfSigned -Cert $cert)
    if (-not $isSelfSigned -and -not $ForceTrust) {
      Write-Host "PFX is not self-signed (SubjectName/IssuerName differ); not modifying trust stores."
      Write-Host "If SignTool still reports an untrusted root, use a publicly trusted code-signing certificate."
      return
    }
    if (-not $isSelfSigned -and $ForceTrust) {
      Write-Warning "-ForceTrustSigningCert: installing leaf certificate into trust stores (use only for dev / self-made CAs)."
    }

    # Must not use New-Object X509Certificate2($cert.RawData): PowerShell splats byte[] into hundreds of ctor args.
    $publicOnly = [System.Security.Cryptography.X509Certificates.X509Certificate2]::new([byte[]]$cert.RawData)
    try {
      Write-Host "Trust stores (Current User): installing public cert for SignTool chain validation..."
      Add-PublicCertToCurrentUserStoreIfMissing -PublicCert $publicOnly -StoreName ([System.Security.Cryptography.X509Certificates.StoreName]::Root)
      Add-PublicCertToCurrentUserStoreIfMissing -PublicCert $publicOnly -StoreName ([System.Security.Cryptography.X509Certificates.StoreName]::TrustedPublisher)
      Write-Host "Done. Thumbprint: $($cert.Thumbprint)"
    }
    finally {
      $publicOnly.Dispose()
    }
  }
  finally {
    $cert.Dispose()
  }
}

# Resolve signtool.exe (Windows SDK) — not on PATH by default
function Get-SignToolPath {
  if ($SignToolPath -and (Test-Path -LiteralPath $SignToolPath)) {
    return (Resolve-Path -LiteralPath $SignToolPath).Path
  }
  $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }

  $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhere) {
    $found = & $vswhere -latest -products * -find "**\signtool.exe" 2>$null
    if ($found) {
      foreach ($line in $found) {
        if ($line -and (Test-Path -LiteralPath $line)) { return $line }
      }
    }
  }

  $kitsBin = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
  if (Test-Path $kitsBin) {
    $candidates = Get-ChildItem -Path $kitsBin -Directory -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -match '^\d+\.\d+' } |
      Sort-Object {
        try { [version]$_.Name } catch { [version]'0.0' }
      } -Descending
    foreach ($verDir in $candidates) {
      $x64 = Join-Path $verDir.FullName "x64\signtool.exe"
      if (Test-Path -LiteralPath $x64) { return $x64 }
    }
  }

  # Optional: standalone Windows Kits under Program Files
  $kitsBin64 = "${env:ProgramFiles}\Windows Kits\10\bin"
  if (Test-Path $kitsBin64) {
    $candidates = Get-ChildItem -Path $kitsBin64 -Directory -ErrorAction SilentlyContinue |
      Where-Object { $_.Name -match '^\d+\.\d+' } |
      Sort-Object {
        try { [version]$_.Name } catch { [version]'0.0' }
      } -Descending
    foreach ($verDir in $candidates) {
      $x64 = Join-Path $verDir.FullName "x64\signtool.exe"
      if (Test-Path -LiteralPath $x64) { return $x64 }
    }
  }

  return $null
}

# Path to the TeenAstro ASCOM installer EXE
$exePath = ".\TeenAstroASCOM_V7\TeenAstro Setup.exe"

if (-not (Test-Path $exePath)) {
  Write-Error "EXE not found: $exePath"
  exit 1
}

if (-not (Test-Path $PfxPath)) {
  Write-Error "PFX not found: $PfxPath"
  exit 1
}

$signTool = Get-SignToolPath
if (-not $signTool) {
  Write-Error @"
signtool.exe not found. It is part of the Windows SDK (not installed or not on PATH).

Install one of:
  - Visual Studio Installer -> Individual components -> search for "Windows SDK" and "Signing Tools for Desktop Apps", or
  - https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/ (install the SDK; signtool is under bin\x64\)

Then either:
  - Add the SDK x64 folder to your user PATH, or
  - Re-run this script with:
      -SignToolPath 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.xxxxx.0\x64\signtool.exe'

Typical path after install:
  C:\Program Files (x86)\Windows Kits\10\bin\10.0.xxxxx.0\x64\signtool.exe
"@
  exit 1
}
Write-Host "Using signtool: $signTool"

# Ask for the PFX password securely
$pwd = Read-Host -AsSecureString "Enter PFX password"

# Convert secure string to plain text just for this command call
$pwdBSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($pwd)
$pwdPlain = [System.Runtime.InteropServices.Marshal]::PtrToStringUni($pwdBSTR)

if (-not $SkipTrustRoot) {
  try {
    Install-SelfSignedSignerIntoCurrentUserTrustStores -PfxPath $PfxPath -PasswordPlain $pwdPlain -ForceTrust:([bool]$ForceTrustSigningCert)
  }
  catch {
    Write-Error "Could not install signing certificate into trust stores: $_`n`nManual fix (PowerShell):`n  `$c = Get-PfxData -FilePath '$PfxPath' -Password (Read-Host -AsSecureString)`n  Or export .cer and: Import-Certificate -FilePath cert.cer -CertStoreLocation Cert:\CurrentUser\Root`n  Import-Certificate -FilePath cert.cer -CertStoreLocation Cert:\CurrentUser\TrustedPublisher"
    $pwdPlain = $null
    exit 1
  }
}

Write-Host "Signing '$exePath' with certificate '$PfxPath'..."

$signArgs = @(
  'sign',
  '/f', $PfxPath,
  '/p', $pwdPlain,
  '/fd', 'sha256'
)
if (-not $NoTimestamp) {
  $signArgs += @('/tr', $TimestampUrl, '/td', 'sha256')
}
else {
  Write-Host "Timestamp: skipped (-NoTimestamp). Signature is still valid but omitting RFC 3161 timestamp."
}
$signArgs += $exePath

& $signTool @signArgs
if ($LASTEXITCODE -ne 0) {
  $pwdPlain = $null
  exit $LASTEXITCODE
}

# Clear plain-text password variable
$pwdPlain = $null

Write-Host "Verifying signature..."
& $signTool verify /pa /v "$exePath"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Done."

