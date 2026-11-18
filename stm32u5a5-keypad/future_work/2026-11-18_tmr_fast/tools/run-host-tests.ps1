# PC test of tmr_analysis.c (classification, press detection, statistics).
# Same approach as the rate test: MSVC cl.exe, no C runtime. Touches no hardware.
param([string]$CompilerPath = '')
$ErrorActionPreference = 'Stop'
$here = Split-Path $PSScriptRoot -Parent
if (!$CompilerPath) {
    $tool = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022" -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Hostx64\\x64\\cl.exe$' } | Select-Object -First 1
    if ($tool) { $CompilerPath = $tool.FullName }
}
if (!$CompilerPath) { throw 'Host tests need MSVC cl.exe; supply -CompilerPath.' }
$versionRoot = Split-Path (Split-Path (Split-Path (Split-Path $CompilerPath)))
$out = Join-Path $here 'output\host-tests'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$exe = Join-Path $out 'tmr-tests.exe'
& $CompilerPath /nologo /TC /std:c11 /W4 /WX /GS- /Od /Zl "/I$versionRoot\include" "/I$here\firmware\Core\Src" "$here\tests\host\harness_tmr.c" "$here\firmware\Core\Src\tmr_analysis.c" "/Fo$($out.Replace('\','/'))/" /link /NODEFAULTLIB /ENTRY:main /SUBSYSTEM:CONSOLE "/OUT:$exe"
if ($LASTEXITCODE) { throw 'TMR host test failed to compile' }
& $exe
if ($LASTEXITCODE) { throw "tmr-tests FAILED at source line $LASTEXITCODE of harness_tmr.c" }
Write-Output 'PASS: TMR analysis (classification, press/release, polarity, drift, statistics)'
