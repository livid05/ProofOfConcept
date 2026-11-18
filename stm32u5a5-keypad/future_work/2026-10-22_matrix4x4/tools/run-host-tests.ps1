# Builds and runs the two PC tests for the 4 x 4 matrix firmware.
#   1. harness_logic.c : the real key_matrix.c with fake time, contacts and USB
#   2. harness_hw.c    : the real matrix_hw.c against an electrical matrix model
# Nothing here touches the board, the existing build/ folder or any existing
# source file. All output goes to ..\output\host-tests\.
param([string]$CompilerPath = '')
$ErrorActionPreference = 'Stop'
$here = Split-Path $PSScriptRoot -Parent                      # ...\2026-10-22_matrix4x4
$project = Split-Path (Split-Path $here -Parent) -Parent       # ...\Keypad U5A5 (read-only use)
if (!$CompilerPath) {
    $tool = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio\2022" -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Hostx64\\x64\\cl.exe$' } | Select-Object -First 1
    if ($tool) { $CompilerPath = $tool.FullName }
}
if (!$CompilerPath) { throw 'Host tests need MSVC cl.exe; supply -CompilerPath.' }
$versionRoot = Split-Path (Split-Path (Split-Path (Split-Path $CompilerPath)))
$out = Join-Path $here 'output\host-tests'
New-Item -ItemType Directory -Path $out -Force | Out-Null

# Include order matters: fake headers first so they win over the real USB headers.
$includes = @("/I$versionRoot\include", "/I$here\tests\host\fakes", "/I$here\firmware\Core\Inc", "/I$project\firmware\Core\Inc")
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

Build-And-Run 'logic-tests' @("$here\tests\host\harness_logic.c", "$here\firmware\Core\Src\key_matrix.c")
Build-And-Run 'hw-model-tests' @("$here\tests\host\harness_hw.c", "$here\firmware\Core\Src\matrix_hw.c")

# Conservative mode: a COPY of board_config.h with eager press turned off is
# placed first on the include path. The real board_config.h is never modified.
$copyDir = Join-Path $out 'noneager-config'
New-Item -ItemType Directory -Path $copyDir -Force | Out-Null
$config = (Get-Content "$project\firmware\Core\Inc\board_config.h" -Raw) -replace '#define KEY_EAGER_PRESS 1U', '#define KEY_EAGER_PRESS 0U'
if ($config -notmatch '#define KEY_EAGER_PRESS 0U') { throw 'Could not switch KEY_EAGER_PRESS off in the copied config' }
Set-Content -LiteralPath "$copyDir\board_config.h" -Value $config -Encoding ascii
$common = @('/nologo', '/TC', '/std:c11', '/W4', '/WX', '/GS-', '/Od', '/Zl', "/I$copyDir") + $includes
Build-And-Run 'noneager-tests' @("$here\tests\host\harness_noneager.c", "$here\firmware\Core\Src\key_matrix.c")
Write-Output 'ALL HOST TESTS PASSED (logic only; no hardware was touched).'
