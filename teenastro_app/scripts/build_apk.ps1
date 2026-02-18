# Build release APK for TeenAstro app.
# Use this if "flutter build apk" fails with Java SSL/trust-store errors.

$ErrorActionPreference = "Stop"
$script:ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

# Java SSL fix (short path avoids "Program Files" space issues)
$env:JAVA_TOOL_OPTIONS = "-Djavax.net.ssl.trustStore=C:/PROGRA~1/Android/ANDROI~1/jbr/lib/security/cacerts -Djavax.net.ssl.trustStorePassword=changeit -Djavax.net.ssl.trustStoreType=JKS"
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
$env:Path = "$env:JAVA_HOME\bin;C:\Users\clemair\AppData\Local\Programs\Git\bin;C:\Users\clemair\flutter\bin;$env:Path"
$env:ANDROID_HOME = [Environment]::GetEnvironmentVariable("ANDROID_HOME", "User")
if (-not $env:ANDROID_HOME) { $env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk" }
$env:ANDROID_SDK_ROOT = $env:ANDROID_HOME

Set-Location $script:ProjectRoot
flutter build apk @args
exit $LASTEXITCODE
