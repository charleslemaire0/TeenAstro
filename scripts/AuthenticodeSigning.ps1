# Dot-source only. Shared Authenticode (signtool) helpers for ASCOM build/sign scripts.

function Test-X509SelfSigned {
    param([System.Security.Cryptography.X509Certificates.X509Certificate2]$Cert)
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

function Get-SignToolPath {
    param([string]$SignToolPath = "")
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

function Invoke-AuthenticodeSign {
    param(
        [Parameter(Mandatory)][string]$SignToolExe,
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string]$PfxPath,
        [Parameter(Mandatory)][string]$PasswordPlain,
        [string]$TimestampUrl = "http://timestamp.digicert.com",
        [switch]$NoTimestamp,
        [switch]$NoAutoRetryWithoutTimestamp
    )
    # Capture stdout/stderr so verbose output is not returned as the function's output (would break $LASTEXITCODE handling).
    function Invoke-AuthenticodeSignInner {
        param([bool]$UseTimestamp, [string]$TsUrl)
        $signArgs = @('sign', '/v', '/f', $PfxPath, '/p', $PasswordPlain, '/fd', 'sha256')
        if ($UseTimestamp) {
            Write-Host "Timestamp server: $TsUrl" -ForegroundColor DarkGray
            $signArgs += @('/tr', $TsUrl, '/td', 'sha256')
        }
        $signArgs += $FilePath
        $raw = & $SignToolExe @signArgs 2>&1
        $exit = $LASTEXITCODE
        if ($null -ne $raw) {
            $raw | ForEach-Object { Write-Host $_ }
        }
        return $exit
    }

    Write-Host "Signing '$FilePath'..."
    if ($NoTimestamp) {
        Write-Host "Timestamp: skipped (-NoTimestamp)." -ForegroundColor DarkGray
        $code = Invoke-AuthenticodeSignInner -UseTimestamp $false -TsUrl ""
        if ($code -ne 0) { Write-Host "signtool sign failed with exit code $code." -ForegroundColor Red }
        return $code
    }

    $code = Invoke-AuthenticodeSignInner -UseTimestamp $true -TsUrl $TimestampUrl
    if ($code -ne 0) {
        Write-Host "signtool sign failed with exit code $code (with timestamp server)." -ForegroundColor Red
        if (-not $NoAutoRetryWithoutTimestamp) {
            Write-Warning "Retrying without RFC 3161 timestamp (signature is still valid; some networks block timestamp servers)."
            $code = Invoke-AuthenticodeSignInner -UseTimestamp $false -TsUrl ""
            if ($code -eq 0) {
                Write-Host "Signed successfully without timestamp." -ForegroundColor Yellow
            }
            else {
                Write-Host "signtool sign failed with exit code $code (without timestamp)." -ForegroundColor Red
            }
        }
    }
    return $code
}

function Invoke-AuthenticodeVerify {
    param(
        [Parameter(Mandatory)][string]$SignToolExe,
        [Parameter(Mandatory)][string]$FilePath
    )
    Write-Host "Verifying signature: $FilePath"
    $raw = & $SignToolExe verify /pa /v $FilePath 2>&1
    $exit = $LASTEXITCODE
    if ($null -ne $raw) {
        $raw | ForEach-Object { Write-Host $_ }
    }
    return $exit
}

# Reads first line from a password sidecar file. Handles UTF-8 (with/without BOM) and UTF-16 LE/BE (Notepad "Unicode").
function Read-PfxPasswordSidecarFirstLine {
    param([Parameter(Mandatory)][string]$Path)
    [byte[]]$b = [System.IO.File]::ReadAllBytes($Path)
    if ($b.Length -eq 0) { throw "Password file is empty: $Path" }

    $offset = 0
    $enc = [System.Text.Encoding]::UTF8
    if ($b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF) {
        $offset = 3
    }
    elseif ($b.Length -ge 2 -and $b[0] -eq 0xFF -and $b[1] -eq 0xFE) {
        $enc = [System.Text.Encoding]::Unicode
        $offset = 2
    }
    elseif ($b.Length -ge 2 -and $b[0] -eq 0xFE -and $b[1] -eq 0xFF) {
        $enc = [System.Text.Encoding]::BigEndianUnicode
        $offset = 2
    }

    $text = $enc.GetString($b, $offset, $b.Length - $offset)
    $nl = $text.IndexOfAny([char[]]@("`r", "`n"))
    if ($nl -ge 0) { $text = $text.Substring(0, $nl) }
    $line = $text.Trim() -replace "`0", "" -replace "[\u200B-\u200D\uFEFF]", ""
    if ([string]::IsNullOrEmpty($line)) { throw "Password file first line is empty: $Path" }
    return $line
}

function Test-X509PfxOpens {
    param(
        [Parameter(Mandatory)][string]$PfxPath,
        [Parameter(Mandatory)][string]$PasswordPlain
    )
    $flags = [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::UserKeySet
    $cert = $null
    try {
        $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($PfxPath, $PasswordPlain, $flags)
        return $true
    }
    catch {
        return $false
    }
    finally {
        if ($null -ne $cert) { $cert.Dispose() }
    }
}

# Multiple ways to read the same password file (UTF-8/UTF-16 vs legacy ANSI without BOM).
function Get-PfxPasswordCandidatesFromFile {
    param([Parameter(Mandatory)][string]$Path)
    $cands = [System.Collections.Generic.List[string]]::new()
    [void]$cands.Add((Read-PfxPasswordSidecarFirstLine -Path $Path))

    [byte[]]$b = [System.IO.File]::ReadAllBytes($Path)
    if ($b.Length -eq 0) { return ,$cands.ToArray() }

    $looksUtf8Bom = $b.Length -ge 3 -and $b[0] -eq 0xEF -and $b[1] -eq 0xBB -and $b[2] -eq 0xBF
    $looksUtf16Le = $b.Length -ge 2 -and $b[0] -eq 0xFF -and $b[1] -eq 0xFE
    $looksUtf16Be = $b.Length -ge 2 -and $b[0] -eq 0xFE -and $b[1] -eq 0xFF
    if (-not $looksUtf8Bom -and -not $looksUtf16Le -and -not $looksUtf16Be) {
        $enc = [System.Text.Encoding]::Default
        $text = $enc.GetString($b)
        $nl = $text.IndexOfAny([char[]]@("`r", "`n"))
        if ($nl -ge 0) { $text = $text.Substring(0, $nl) }
        $ansi = ($text.Trim() -replace "`0", "" -replace "[\u200B-\u200D\uFEFF]", "")
        if ($ansi.Length -gt 0 -and $ansi -ne $cands[0]) { [void]$cands.Add($ansi) }
    }
    return ,$cands.ToArray()
}

function Resolve-PfxPasswordAgainstCertificate {
    param(
        [Parameter(Mandatory)][string]$PfxPath,
        [Parameter(Mandatory)][string]$InitialPlain,
        [string[]]$PasswordFilesToProbe = @()
    )
    if (Test-X509PfxOpens -PfxPath $PfxPath -PasswordPlain $InitialPlain) {
        return $InitialPlain
    }
    foreach ($pf in $PasswordFilesToProbe) {
        if (-not $pf -or -not (Test-Path -LiteralPath $pf)) { continue }
        foreach ($c in (Get-PfxPasswordCandidatesFromFile -Path $pf)) {
            if (Test-X509PfxOpens -PfxPath $PfxPath -PasswordPlain $c) {
                Write-Host "PFX password matched using an alternate encoding from: $pf" -ForegroundColor DarkGray
                return $c
            }
        }
    }
    throw @"
Could not open the PFX with any password candidate.

Verify that the password in the sidecar file matches how you exported the PFX. Try:
  - Notepad: one line only, Save As -> UTF-8 or ANSI (not Unicode) if the password is ASCII.
  - Re-export the PFX from certmgr with a new password, then update the sidecar file.

PFX: $PfxPath
"@
}

# Resolves password: SecureString, env TEENASTRO_PFX_PASSWORD, explicit PasswordFile, PfxPath + ".password.txt", else $null (prompt caller).
function Get-PfxPasswordPlain {
    param(
        [System.Security.SecureString]$SecurePassword,
        [Parameter(Mandatory)][string]$PfxPath,
        [string]$PasswordFile = "",
        [switch]$SkipAutoPasswordFile
    )
    if ($SecurePassword) {
        $BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($SecurePassword)
        try {
            return [System.Runtime.InteropServices.Marshal]::PtrToStringUni($BSTR)
        }
        finally {
            [System.Runtime.InteropServices.Marshal]::ZeroFreeBSTR($BSTR)
        }
    }
    if ($PasswordFile) {
        if (-not (Test-Path -LiteralPath $PasswordFile)) {
            throw "PFX password file not found: $PasswordFile"
        }
        Write-Host "Using PFX password file: $PasswordFile" -ForegroundColor Gray
        return Read-PfxPasswordSidecarFirstLine -Path $PasswordFile
    }
    $envPw = $env:TEENASTRO_PFX_PASSWORD
    if ($envPw -and $envPw.Trim().Length -gt 0) {
        Write-Host "Using PFX password from TEENASTRO_PFX_PASSWORD (environment variable)." -ForegroundColor Gray
        return $envPw.Trim()
    }
    if (-not $SkipAutoPasswordFile) {
        $autoFile = $PfxPath + ".password.txt"
        if (Test-Path -LiteralPath $autoFile) {
            Write-Host "Using PFX password file: $autoFile" -ForegroundColor Gray
            return Read-PfxPasswordSidecarFirstLine -Path $autoFile
        }
    }
    return $null
}
