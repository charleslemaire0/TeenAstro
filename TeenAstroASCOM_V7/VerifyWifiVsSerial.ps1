param(
  [string]$Ip = "192.168.1.17",
  [int]$TcpPort = 9999,
  # value written into ASCOM profile "IP Adress" field; can be pure IP or "ip:port"
  [string]$IpField = "192.168.1.17:9999",
  [bool]$AutoDetectCom = $true,
  [string[]]$ComPorts = @(),
  [int]$MaxComPortsToTry = 10,
  [int]$ProbeAttempts = 6,
  [int]$ProbeDelayMs = 400,
  # if true, attempts to set Tracking to the same value (sends Te/Td commands)
  [bool]$AttemptTrackingSet = $true
)

$ErrorActionPreference = "Stop"

$assemblyDir = Join-Path $PSScriptRoot "TeenAstroASCOM_V7\bin\Release"
$assemblyPath = Join-Path $assemblyDir "ASCOM.TeenAstro.exe"

if (!(Test-Path $assemblyPath)) {
  throw "ASCOM.TeenAstro.exe not found: $assemblyPath"
}

Write-Host "[INFO] Loading ASCOM driver assembly: $assemblyPath"
$asm = [System.Reflection.Assembly]::LoadFrom($assemblyPath)

# Load dependent assemblies so ASCOM.Utilities / ASCOM.TeenAstro types are visible in this PowerShell session.
Get-ChildItem -Path $assemblyDir -Filter "*.dll" | ForEach-Object {
  try {
    [System.Reflection.Assembly]::LoadFrom($_.FullName) | Out-Null
  } catch { }
}

function InitLocalServerForInProcessTests {
  # The driver constructors inherit ReferenceCountedObjectBase, which calls
  # ASCOM.LocalServer.Server.IncrementObjectCount().
  # That method logs via a private static TraceLogger field "TL", so TL must be set.
  $serverType = $asm.GetType("ASCOM.LocalServer.Server", $true, $false)
  if ($serverType -eq $null) {
    throw "Could not find type ASCOM.LocalServer.Server"
  }

  $tlField = $serverType.GetField("TL", [System.Reflection.BindingFlags]::NonPublic -bor [System.Reflection.BindingFlags]::Static)
  if ($tlField -ne $null) {
    $tl = New-Object ASCOM.Utilities.TraceLogger ("", "TeenAstro.LocalServer")
    $tl.Enabled = $false
    $tlField.SetValue($null, $tl)
  }

  $startedByComField = $serverType.GetField("startedByCOM", [System.Reflection.BindingFlags]::NonPublic -bor [System.Reflection.BindingFlags]::Static)
  if ($startedByComField -ne $null) {
    $startedByComField.SetValue($null, $false)
  }
}

function SetAscomProfile {
  param(
    [Parameter(Mandatory=$true)][string]$InterfaceType,
    [Parameter()][string]$ComPort
  )

  $driverProgId = "ASCOM.TeenAstro.Telescope"
  $traceKey = "Trace Level"
  $comPortKey = "COM Port"
  $ipKey = "IP Adress"
  $portKey = "Port"
  $interfaceKey = "Interface"

  $profile = New-Object ASCOM.Utilities.Profile
  try {
    $profile.DeviceType = "Telescope"
    $profile.WriteValue($driverProgId, $traceKey, "false")
    $profile.WriteValue($driverProgId, $comPortKey, $ComPort)
    $profile.WriteValue($driverProgId, $ipKey, $IpField)
    $profile.WriteValue($driverProgId, $portKey, $TcpPort.ToString())
    $profile.WriteValue($driverProgId, $interfaceKey, $InterfaceType)
  }
  finally {
    $profile.Dispose()
  }
}

function TestConnection {
  param(
    [Parameter(Mandatory=$true)][string]$InterfaceType,
    [Parameter()][string]$ComPort,
    [Parameter(Mandatory=$true)][string]$Label
  )

  Write-Host ""
  Write-Host "[TEST] $Label"

  SetAscomProfile -InterfaceType $InterfaceType -ComPort $ComPort
  InitLocalServerForInProcessTests

  $telescope = $null
  try {
    $telescope = New-Object ASCOM.TeenAstro.Telescope.Telescope
    $telescope.Connected = $true

    $ra = $null
    $lastEx = $null
    for ($i = 1; $i -le $ProbeAttempts; $i++) {
      try {
        $ra = [double]$telescope.RightAscension
        break
      }
      catch {
        $lastEx = $_
        Start-Sleep -Milliseconds $ProbeDelayMs
      }
    }

    if ($null -eq $ra) {
      throw "RightAscension probe failed after $ProbeAttempts attempts. Last error: $($lastEx.Exception.Message)"
    }

    $dec = [double]$telescope.Declination
    $tracking = [bool]$telescope.Tracking
    $canSetTracking = [bool]$telescope.CanSetTracking

    $trackingSetOk = $null
    $trackingSetErr = ""
    if ($AttemptTrackingSet -and $canSetTracking) {
      try {
        # Setting to the current value should not change behavior, but still exercises SET path.
        $telescope.Tracking = $tracking
        $trackingSetOk = $true
      }
      catch {
        $trackingSetOk = $false
        $trackingSetErr = $_.Exception.Message
      }
    }

    return [pscustomobject]@{
      Label = $Label
      Interface = $InterfaceType
      ComPort = $ComPort
      TcpIpField = $IpField
      TcpPort = $TcpPort
      Connected = $telescope.Connected
      RightAscensionHours = $ra
      DeclinationDegrees = $dec
      Tracking = $tracking
      CanSetTracking = $canSetTracking
      AttemptTrackingSet = $AttemptTrackingSet
      TrackingSetOk = $trackingSetOk
      TrackingSetError = $trackingSetErr
    }
  }
  finally {
    try {
      if ($telescope -ne $null) {
        $telescope.Connected = $false
        $telescope.Dispose()
      }
    } catch { }
  }
}

Set-Location $assemblyDir

$results = New-Object System.Collections.Generic.List[object]

# 1) WiFi (IP)
$wifiLabel = "WiFi(IP) => IpField=$IpField TcpPort=$TcpPort"
$wifi = TestConnection -InterfaceType "IP" -ComPort "COM1" -Label $wifiLabel
$results.Add($wifi) | Out-Null

# 2) Serial (COM) - auto-detect if needed
if ($AutoDetectCom) {
  $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
  if ($ports.Count -eq 0) {
    Write-Host "[WARN] No COM ports found by SerialPort.GetPortNames()."
    $ports = @()
  }
  $portsToTry = $ports | Select-Object -First $MaxComPortsToTry
}
else {
  $portsToTry = $ComPorts | Select-Object -First $MaxComPortsToTry
}

if ($portsToTry -eq $null -or $portsToTry.Count -eq 0) {
  Write-Host "[WARN] No COM ports to try."
} else {
  $serialSuccess = $false
  foreach ($p in $portsToTry) {
    try {
      $serial = TestConnection -InterfaceType "COM" -ComPort $p -Label "Serial(COM) => $p"
      $results.Add($serial) | Out-Null
      $serialSuccess = $true
      break
    }
    catch {
      Write-Host "[WARN] Serial port $p failed: $($_.Exception.Message)"
    }
  }

  if (-not $serialSuccess) {
    Write-Host "[WARN] No serial COM port succeeded."
  }
}

Write-Host ""
Write-Host "[SUMMARY]"
foreach ($r in $results) {
  Write-Host ("- {0}: Connected={1}, RA={2:F4}h, DEC={3:F4}deg, Tracking={4}, CanSetTracking={5}, TrackingSetOk={6}" -f `
    $r.Label, $r.Connected, $r.RightAscensionHours, $r.DeclinationDegrees, $r.Tracking, $r.CanSetTracking, $r.TrackingSetOk)
}

