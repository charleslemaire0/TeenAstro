# Wrapper: delegates to the canonical build script in repo scripts\.
# You can run this from teenastro_app\scripts\ or use the canonical script:  scripts\build_app.ps1

$_me = $MyInvocation.MyCommand.Path
if ($_me -and (Test-Path $_me)) {
    $_appScripts = Split-Path $_me -Parent
} else {
    $_appScripts = $PSScriptRoot
}
$RepoRoot = (Resolve-Path (Join-Path $_appScripts "..\..")).Path
$buildApp = Join-Path $RepoRoot "scripts\build_app.ps1"
if (-not (Test-Path $buildApp)) {
    Write-Error "Could not find scripts\build_app.ps1 at $buildApp. Run from repo: scripts\build_app.ps1"
}
& $buildApp @args
