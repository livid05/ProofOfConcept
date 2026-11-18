# PC test of the rate-test streamer (rate_diag.c) plus the measurement maths.
# Reads project files but writes only to ..\output\. Touches no hardware.
param([string]$CompilerPath = '')
$ErrorActionPreference = 'Stop'
$here = Split-Path $PSScriptRoot -Parent
$project = Split-Path (Split-Path $here -Parent) -Parent
if (!$CompilerPath) {
    $tool = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022" -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Hostx64\\x64\\cl.exe$' } | Select-Object -First 1
    if ($tool) { $CompilerPath = $tool.FullName }
}
if (!$CompilerPath) { throw 'Host tests need MSVC cl.exe; supply -CompilerPath.' }
$versionRoot = Split-Path (Split-Path (Split-Path (Split-Path $CompilerPath)))
$out = Join-Path $here 'output\host-tests'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$exe = Join-Path $out 'diag-tests.exe'
& $CompilerPath /nologo /TC /std:c11 /W4 /WX /GS- /Od /Zl "/I$versionRoot\include" "/I$project\tests\host\fakes" "/I$here\firmware\Core\Src" "/I$project\firmware\Core\Inc" "$here\tests\host\harness_diag.c" "$here\firmware\Core\Src\rate_diag.c" "/Fo$($out.Replace('\','/'))/" /link /NODEFAULTLIB /ENTRY:main /SUBSYSTEM:CONSOLE "/OUT:$exe"
if ($LASTEXITCODE) { throw 'diag test failed to compile' }
& $exe
if ($LASTEXITCODE) { throw "diag-tests FAILED at source line $LASTEXITCODE of harness_diag.c" }
Write-Output 'PASS: rate_diag streamer (layout, counter, busy retry, clock wrap)'
& powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot\measure-rate.ps1" -SelfTest
if ($LASTEXITCODE) { throw 'measurement self-test failed' }
