# Verifies TeenAstro binary/hex rate protocol on USB serial (default COM3 @ 57600).
# Matches firmware Command_SX (SXRr hex) + Command_GX (GXRr hex reply) and ASCOM DoubleToHexLe.
#
# Usage: .\VerifyRateHexCom.ps1 [-Port COM3] [-Baud 57600]
# Requires: mount on firmware with :GXRr# / hex :SXRr, support.

param(
    [string]$Port = "COM3",
    [int]$Baud = 57600
)

function DoubleToHexLe([double]$v) {
    $b = [BitConverter]::GetBytes($v)
    $sb = New-Object System.Text.StringBuilder 16
    foreach ($x in $b) { [void]$sb.Append($x.ToString("x2")) }
    return $sb.ToString()
}

function ParseHexLeDouble([string]$hex16) {
    if ($null -eq $hex16 -or $hex16.Length -ne 16) { return $null }
    $bytes = New-Object byte[] 8
    for ($i = 0; $i -lt 8; $i++) {
        $pair = $hex16.Substring($i * 2, 2)
        $bytes[$i] = [Convert]::ToByte($pair, 16)
    }
    return [BitConverter]::ToDouble($bytes, 0)
}

function Send-ReadHashResponse {
    param([System.IO.Ports.SerialPort]$sp, [string]$cmd)
    # cmd must include leading : and trailing #
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($cmd)
    $sp.Write($bytes, 0, $bytes.Length)
    $sp.BaseStream.Flush()
    $sb = New-Object System.Text.StringBuilder
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while ([DateTime]::UtcNow -lt $deadline) {
        if ($sp.BytesToRead -gt 0) {
            $b = $sp.ReadByte()
            if ($b -eq 0x23) { break } # '#'
            if ($b -ge 0) { [void]$sb.Append([char]$b) }
        } else {
            Start-Sleep -Milliseconds 2
        }
    }
    return $sb.ToString()
}

function Send-ReadOneChar {
    param([System.IO.Ports.SerialPort]$sp, [string]$cmd)
    $bytes = [System.Text.Encoding]::ASCII.GetBytes($cmd)
    $sp.Write($bytes, 0, $bytes.Length)
    $sp.BaseStream.Flush()
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while ([DateTime]::UtcNow -lt $deadline -and $sp.BytesToRead -eq 0) { Start-Sleep -Milliseconds 2 }
    if ($sp.BytesToRead -lt 1) { return "" }
    return [char]$sp.ReadByte()
}

$ErrorActionPreference = "Stop"
Write-Host "Opening $Port @ $Baud..."
$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, "None", 8, "One"
$sp.ReadTimeout = 5000
$sp.WriteTimeout = 5000
$sp.Open()
try {
    Start-Sleep -Milliseconds 200
    while ($sp.BytesToRead -gt 0) { [void]$sp.ReadExisting() }

    # Short legacy command must ACK (proves COM + firmware accept SXRr at all)
    $ackShort = Send-ReadOneChar $sp ":SXRr,0#"
    if ($ackShort -ne "1") {
        throw "Legacy :SXRr,0# did not return '1' (got '$ackShort'). Check baud 57600 and TeenAstro on $Port."
    }
    Write-Host "PASS  Legacy :SXRr,0# ack"

    # --- :GXRr# → 16 hex + '#'
    $r1 = Send-ReadHashResponse $sp ":GXRr#"
    if ($r1.Length -ne 16) {
        throw "GXRr: expected 16 hex chars before #, got length $($r1.Length): '$r1'"
    }
    $rateBefore = ParseHexLeDouble $r1
    Write-Host "PASS  GXRr readback (ASCOM RA sec/sidereal sec): $rateBefore"

    # --- SET long-precision rate via hex (the case that fails with ASCII decimal)
    $target = -0.0033333333333333335
    $hex = DoubleToHexLe $target
    $ack = Send-ReadOneChar $sp (":SXRr,$hex#")
    if ($ack -ne "1") {
        $hint = "If legacy :SXRr,0# passed but hex fails, flash firmware with CMD_MAX_PAYLOAD>=27 (serial frame must fit :SXRr,<16hex>#) and hex parse in Command_SX.cpp."
        throw "SXRr hex SET: expected '1', got '$ack'. $hint"
    }
    Write-Host "PASS  SXRr hex SET ack for $target"

    $r2 = Send-ReadHashResponse $sp ":GXRr#"
    $rateAfter = ParseHexLeDouble $r2
    $diff = [Math]::Abs($rateAfter - $target)
    if ($diff -gt 1e-12) {
        throw "Round-trip: expected $target, got $rateAfter (diff $diff)"
    }
    Write-Host "PASS  GXRr matches SET value (diff $diff)"

    # --- restore previous rate
    $hexRestore = DoubleToHexLe $rateBefore
    $ack2 = Send-ReadOneChar $sp (":SXRr,$hexRestore#")
    if ($ack2 -ne "1") { Write-Warning "Restore SXRr ack: '$ack2' (expected 1)" }
    else { Write-Host "PASS  Restored previous RA rate" }

    Write-Host ""
    Write-Host "All rate hex checks passed on $Port."
}
finally {
    $sp.Close()
    $sp.Dispose()
}
