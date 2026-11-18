# Builds a COMPLETE matrix-keyboard firmware image without touching the
# existing project: nothing is written to ..\..\build, no CMake cache is used,
# and no existing source file is edited.
#
# Sources used:
#   - every unchanged project source, READ-ONLY, from firmware\ (HAL, USB core, startup...)
#   - this folder's key_matrix.c and matrix_hw.c (replace key_switch.c)
#   - integration\proposed\ usb_keyboard.c and main.c (replace the originals)
# Same compiler flags as CMakeLists.txt, including -Wall -Wextra -Werror.
# Output: ..\output\arm-build\keypad-matrix.{elf,bin,hex,map}
# This script never flashes anything.
param([string]$CubeIdeRoot = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE')
$ErrorActionPreference = 'Stop'
$here = Split-Path $PSScriptRoot -Parent
$project = Split-Path (Split-Path $here -Parent) -Parent
$out = Join-Path $here 'output\arm-build'
New-Item -ItemType Directory -Path $out -Force | Out-Null

function Find-Tool($name) {
    $t = Get-ChildItem -LiteralPath "$CubeIdeRoot\plugins" -Filter $name -Recurse | Select-Object -First 1
    if (!$t) { throw "Cannot find $name under $CubeIdeRoot" }
    return $t.FullName
}
$gcc = Find-Tool 'arm-none-eabi-gcc.exe'
$bin = Split-Path $gcc
$objcopy = Join-Path $bin 'arm-none-eabi-objcopy.exe'
$size = Join-Path $bin 'arm-none-eabi-size.exe'

$fw = "$project\firmware"
$proposed = "$here\integration\proposed\firmware\Core"
# Include order: proposed headers first, then this folder's, then the project's.
$inc = @("$proposed\Inc", "$here\firmware\Core\Inc", "$fw\Core\Inc",
    "$fw\Drivers\STM32U5xx_HAL_Driver\Inc", "$fw\Drivers\STM32U5xx_HAL_Driver\Inc\Legacy",
    "$fw\Drivers\CMSIS\Include", "$fw\Drivers\CMSIS\Device\ST\STM32U5xx\Include", "$fw\USB\Core\Inc") | ForEach-Object { "-I$_" }
$cpu = @('-mcpu=cortex-m33', '-mthumb', '-mfpu=fpv5-sp-d16', '-mfloat-abi=hard')
$cflags = $cpu + @('-g3', '-Og', '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra', '-Werror', '-DSTM32U5A5xx', '-DUSE_HAL_DRIVER')

# Same file set as CMake, minus the three replaced sources.
$replaced = @('key_switch.c', 'usb_keyboard.c', 'main.c')
$sources = @()
$sources += Get-ChildItem "$fw\Core\Src\*.c" | Where-Object { $replaced -notcontains $_.Name } | ForEach-Object FullName
$sources += Get-ChildItem "$fw\USB\Core\Src\usbd_*.c" | Where-Object { $_.Name -notmatch 'template' } | ForEach-Object FullName
foreach ($n in 'hal', 'hal_cortex', 'hal_dma', 'hal_dma_ex', 'hal_exti', 'hal_flash', 'hal_flash_ex', 'hal_gpio', 'hal_pwr', 'hal_pwr_ex', 'hal_rcc', 'hal_rcc_ex', 'hal_pcd', 'hal_pcd_ex', 'll_usb') {
    $sources += "$fw\Drivers\STM32U5xx_HAL_Driver\Src\stm32u5xx_$n.c"
}
$sources += "$here\firmware\Core\Src\key_matrix.c", "$here\firmware\Core\Src\matrix_hw.c"
$sources += "$proposed\Src\usb_keyboard.c", "$proposed\Src\main.c"
$startup = Get-ChildItem "$fw\Core\Startup\*.s" | ForEach-Object FullName

$objects = @()
foreach ($s in $sources + $startup) {
    $obj = Join-Path $out (([IO.Path]::GetFileNameWithoutExtension($s)) + '.o')
    if ($s -like '*.s') { & $gcc @cpu -x assembler-with-cpp -c $s -o $obj }
    else { & $gcc @cflags @inc -c $s -o $obj }
    if ($LASTEXITCODE) { throw "Compile failed: $s" }
    $objects += $obj
}

$elf = Join-Path $out 'keypad-matrix.elf'
& $gcc @cpu "-T$fw\STM32U5A5RJTX_FLASH.ld" --specs=nano.specs --specs=nosys.specs "-Wl,--gc-sections,-Map=$out\keypad-matrix.map" "-Wl,--print-memory-usage" @objects -o $elf
if ($LASTEXITCODE) { throw 'Link failed' }
& $objcopy -O binary $elf "$out\keypad-matrix.bin"
& $objcopy -O ihex $elf "$out\keypad-matrix.hex"
& $size $elf
Write-Output "BUILD OK (isolated): $out\keypad-matrix.hex. Nothing was flashed."
