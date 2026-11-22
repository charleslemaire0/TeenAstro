# Builds tool/tetra/tetra.dll for the photo window and the benchmark.
# Android builds the same source through android/app/build.gradle.kts.
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$app = Split-Path -Parent $here
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -property installationPath
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $app 'tool\tetra'
$obj = Join-Path $env:TEMP 'teenastro-tetra-build'
New-Item -ItemType Directory -Force $out, $obj | Out-Null
$src = Join-Path $here 'tetra\tetra.cpp'
$dll = Join-Path $out 'tetra.dll'
cmd /c "`"$vcvars`" >nul && cl /nologo /O2 /EHsc /std:c++17 /LD `"$src`" /Fo`"$obj\\`" /Fe`"$dll`" /link /NOLOGO"
if ($LASTEXITCODE -ne 0) { throw "cl failed with $LASTEXITCODE" }
Get-Item $dll | Select-Object FullName, Length
