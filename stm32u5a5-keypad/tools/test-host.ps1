# Builds and runs the PC tests for the 4 x 4 matrix firmware. Logic only: this
# does not flash or open the real board. Needs MSVC cl.exe (the firmware build does not).
#   logic-tests     : the real key_matrix.c with fake time, key contacts and USB
#   hw-model-tests  : the real matrix_hw.c against an electrical model of the matrix
#   noneager-tests  : key_matrix.c again with eager press switched off (on a COPY of board_config.h)
# Output goes to build\host-tests\. Failures return the harness source line number.
param([string]$CompilerPath = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (!$CompilerPath) {
    $tool = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022" -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Hostx64\\x64\\cl.exe$' } | Select-Object -First 1
    if ($tool) { $CompilerPath = $tool.FullName }
}
if (!$CompilerPath) { throw 'Optional host tests need native MSVC cl.exe; supply -CompilerPath. Firmware build does not require MSVC.' }
$versionRoot = Split-Path (Split-Path (Split-Path (Split-Path $CompilerPath)))
$out = Join-Path $root 'build\host-tests'
New-Item -ItemType Directory -Path $out -Force | Out-Null
# Fake headers first so they win over the real STM32/USB headers.
$includes = @("/I$versionRoot\include", "/I$root\tests\host\fakes", "/I$root\firmware\Core\Inc")
$common = @('/nologo', '/TC', '/std:c11', '/W4', '/WX', '/GS-', '/Od', '/Zl') + $includes

function Build-And-Run($name, $sources) {
    $exe = Join-Path $out "$name.exe"
    $objDir = (Join-Path $out $name).Replace('\', '/') + '/'
    New-Item -ItemType Directory -Path (Join-Path $out $name) -Force | Out-Null
    & $CompilerPath @common @sources "/Fo$objDir" '/link' '/NODEFAULTLIB' '/ENTRY:main' '/SUBSYSTEM:CONSOLE' "/OUT:$exe"
    if ($LASTEXITCODE) { throw "$name failed to compile" }
    & $exe
    if ($LASTEXITCODE) { throw "$name FAILED at source line $LASTEXITCODE of its harness (exit code = line number)" }
    Write-Output "PASS: $name"
}

Build-And-Run 'logic-tests' @("$root\tests\host\harness_logic.c", "$root\firmware\Core\Src\key_matrix.c")
Build-And-Run 'hw-model-tests' @("$root\tests\host\harness_hw.c", "$root\firmware\Core\Src\matrix_hw.c")

$copyDir = Join-Path $out 'noneager-config'
New-Item -ItemType Directory -Path $copyDir -Force | Out-Null
$config = (Get-Content "$root\firmware\Core\Inc\board_config.h" -Raw) -replace '#define KEY_EAGER_PRESS 1U', '#define KEY_EAGER_PRESS 0U'
if ($config -notmatch '#define KEY_EAGER_PRESS 0U') { throw 'Could not switch KEY_EAGER_PRESS off in the copied config' }
Set-Content -LiteralPath "$copyDir\board_config.h" -Value $config -Encoding ascii
# A quoted #include looks in the including file's own folder first, so the headers that
# include board_config.h must sit beside the modified copy or the real one would win.
foreach ($h in 'matrix_config.h', 'key_matrix.h', 'key_clock.h') { Copy-Item "$root\firmware\Core\Inc\$h" $copyDir -Force }
$common = @('/nologo', '/TC', '/std:c11', '/W4', '/WX', '/GS-', '/Od', '/Zl', "/I$copyDir") + $includes
Build-And-Run 'noneager-tests' @("$root\tests\host\harness_noneager.c", "$root\firmware\Core\Src\key_matrix.c")
Write-Output 'ALL HOST TESTS PASSED: 15 keys, chords, press order, bounce, 10 ms release, 6-key limit, busy-USB ordering, reconnect with held keys, overflow recovery, clock wrap, 8000 scans/s, pin mapping.'
