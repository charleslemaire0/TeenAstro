param(
  [string]$PfxPath = "C:\Users\charl\TeenAstroCodeSigning.pfx",
  [string]$TimestampUrl = "http://timestamp.sectigo.com"
)

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

# Ask for the PFX password securely
$pwd = Read-Host -AsSecureString "Enter PFX password"

# Convert secure string to plain text just for this command call
$pwdBSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($pwd)
$pwdPlain = [System.Runtime.InteropServices.Marshal]::PtrToStringUni($pwdBSTR)

# Adjust if signtool.exe is not in PATH
$signTool = "signtool.exe"

Write-Host "Signing '$exePath' with certificate '$PfxPath'..."

& $signTool sign `
  /f $PfxPath `
  /p $pwdPlain `
  /tr $TimestampUrl `
  /td sha256 `
  /fd sha256 `
  "$exePath"

# Clear plain-text password variable
$pwdPlain = $null

Write-Host "Verifying signature..."
& $signTool verify /pa /v "$exePath"

Write-Host "Done."

