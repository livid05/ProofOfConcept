# Compile the actual key_switch.c against fake time/GPIO/USB, then run it on
# Windows. This checks logic only; it does not flash or open the real board.
param([string]$CompilerPath='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!$CompilerPath){
    $tool=Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022" -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Hostx64\\x64\\cl.exe$' } | Select-Object -First 1
    if($tool){$CompilerPath=$tool.FullName}
}
if(!$CompilerPath){throw 'Optional host tests need native MSVC cl.exe; supply -CompilerPath. Firmware build does not require MSVC.'}
$versionRoot=Split-Path (Split-Path (Split-Path (Split-Path $CompilerPath)))
$testBuild=Join-Path $root 'build\host-tests'
New-Item -ItemType Directory -Path $testBuild -Force | Out-Null
$exe=Join-Path $testBuild 'key-tests.exe'
$arguments=@('/nologo','/TC','/std:c11','/W4','/WX','/GS-','/Od','/Zl',
    "/I$versionRoot\include", "/I$root\tests\host", "/I$root\firmware\Core\Inc",
    "$root\tests\host\harness.c", "$root\firmware\Core\Src\key_switch.c",
    "/Fo$($testBuild.Replace('\','/'))/", '/link','/NODEFAULTLIB','/ENTRY:main','/SUBSYSTEM:CONSOLE',"/OUT:$exe")
& $CompilerPath @arguments
if($LASTEXITCODE){throw 'Host test compilation failed'}
& $exe
if($LASTEXITCODE){throw "Key state-machine assertion failed (code $LASTEXITCODE); see tests/host/harness.c"}
Write-Output 'PASS: eager press, contact bounce, stable release, busy USB ordering, reconnect-held suppression, overflow recovery, uint32 rollover, 8000 samples/nominal second.'
