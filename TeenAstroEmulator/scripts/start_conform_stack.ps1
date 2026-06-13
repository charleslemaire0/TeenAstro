<#
.SYNOPSIS
  Build (optional), start MainUnit emulator (TCP 9997), then Alpaca bridge (TCP 11111) for ConformU.

.DESCRIPTION
  Step 1: pio run -e emu_mainunit -e bridge_alpaca (unless -SkipBuild)
  Step 2: mainunit_emu.exe in a new console
  Step 3: program.exe (bridge) in a new console

  Wait until both listening ports respond, then exit 0.

.EXAMPLE
  .\start_conform_stack.ps1

.EXAMPLE
  .\start_conform_stack.ps1 -SkipBuild
#>
param(
  [switch]$SkipBuild,
  [int]$EmuPort = 9997,
  [int]$BridgePort = 11111,
  [int]$WaitOpenSec = 120
)

$ErrorActionPreference = "Stop"

$emulatorRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$emuExe       = Join-Path $emulatorRoot ".pio\build\emu\mainunit_emu.exe"
$bridgeExe    = Join-Path $emulatorRoot ".pio\build\bridge_alpaca\program.exe"
$emuWorkDir   = Split-Path $emuExe -Parent
$bridgeWorkDir = Split-Path $bridgeExe -Parent

function Wait-TcpPort {
  param([int]$Port, [int]$TimeoutSec)
  $deadline = (Get-Date).AddSeconds($TimeoutSec)
  while ((Get-Date) -lt $deadline) {
    try {
      $c = New-Object System.Net.Sockets.TcpClient
      $c.Connect("127.0.0.1", $Port)
      $c.Close()
      return $true
    }
    catch {
      Start-Sleep -Milliseconds 400
    }
  }
  return $false
}

if (-not $SkipBuild) {
  Write-Host "=== [1] Building emu_mainunit + bridge_alpaca ===" -ForegroundColor Cyan
  Push-Location $emulatorRoot
  try {
    pio run -e emu_mainunit -e bridge_alpaca
    if ($LASTEXITCODE -ne 0) { throw "pio build failed with exit code $LASTEXITCODE" }
  }
  finally {
    Pop-Location
  }
}
else {
  Write-Host "=== [1] Skipping build (-SkipBuild) ===" -ForegroundColor Yellow
}

if (-not (Test-Path -LiteralPath $emuExe)) {
  throw "Missing $emuExe - run without -SkipBuild first."
}
if (-not (Test-Path -LiteralPath $bridgeExe)) {
  throw "Missing $bridgeExe - run without -SkipBuild first."
}

Write-Host "=== [2] Starting MainUnit emulator (port $EmuPort) ===" -ForegroundColor Cyan
Start-Process -FilePath $emuExe -WorkingDirectory $emuWorkDir

if (-not (Wait-TcpPort -Port $EmuPort -TimeoutSec $WaitOpenSec)) {
  throw "Emulator did not open TCP $EmuPort within ${WaitOpenSec}s."
}
Write-Host "Emulator listening on 127.0.0.1:$EmuPort" -ForegroundColor Green

Write-Host "=== [3] Starting Alpaca bridge (port $BridgePort) ===" -ForegroundColor Cyan
Start-Process -FilePath $bridgeExe -WorkingDirectory $bridgeWorkDir

if (-not (Wait-TcpPort -Port $BridgePort -TimeoutSec ([Math]::Min(90, $WaitOpenSec)))) {
  throw "Bridge did not open TCP $BridgePort - is the emulator still running?"
}
Write-Host "Bridge listening on 127.0.0.1:$BridgePort" -ForegroundColor Green

$repoHint = (Resolve-Path (Join-Path $emulatorRoot "..")).Path
Write-Host ""
Write-Host "Ready for ConformU, e.g.:" -ForegroundColor Cyan
Write-Host ('  conformu conformance http://127.0.0.1:{0}/api/v1/telescope/0 -n conformu_run.txt -p "{1}"' -f $BridgePort, $repoHint) -ForegroundColor Gray
