# Builds the TMR sensor test image into ..\output\arm-build\ WITHOUT touching the
# project's build\ folder or any project source. Same compiler and flags as the
# project (-Wall -Wextra -Werror). Never flashes anything.
#
# Differences from the normal keyboard image (same pattern as the 8K rate test):
#   - key_matrix.c is replaced by tmr_diag.c + tmr_analysis.c + tmr_hw.c
#   - usb_keyboard.c is replaced by usb_cdc_diag.c (USB COM port, 0483:5712)
#   - this folder's usbd_conf.h / stm32u5xx_hal_conf.h are found first
#     (2 USB interfaces, ADC enabled); the HAL ADC driver v1.6.2 comes from
#     firmware\Drivers_ADC (same HAL version as the project).
param([string]$CubeIdeRoot = 'C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE')
$ErrorActionPreference = 'Stop'
$here = Split-Path $PSScriptRoot -Parent
$project = Split-Path (Split-Path $here -Parent) -Parent
$out = Join-Path $here 'output\arm-build'
New-Item -ItemType Directory -Path $out -Force | Out-Null

function Find-Tool($name) {
    $t = Get-ChildItem -LiteralPath "$CubeIdeRoot\plugins" -Filter $name -Recurse | Select-Object -First 1
    if (!$t) { throw "Cannot find $name under $CubeIdeRoot. Pass -CubeIdeRoot for your installation." }
    return $t.FullName
}
$gcc = Find-Tool 'arm-none-eabi-gcc.exe'
$bin = Split-Path $gcc
$objcopy = Join-Path $bin 'arm-none-eabi-objcopy.exe'
$size = Join-Path $bin 'arm-none-eabi-size.exe'
$fw = "$project\firmware"
$diag = "$here\firmware"
$inc = @("$diag\Core\Src", "$diag\Drivers_ADC\Inc", "$fw\Core\Inc", "$fw\Drivers\STM32U5xx_HAL_Driver\Inc",
    "$fw\Drivers\STM32U5xx_HAL_Driver\Inc\Legacy", "$fw\Drivers\CMSIS\Include",
    "$fw\Drivers\CMSIS\Device\ST\STM32U5xx\Include", "$fw\USB\Core\Inc") | ForEach-Object { "-I$_" }
$cpu = @('-mcpu=cortex-m33', '-mthumb', '-mfpu=fpv5-sp-d16', '-mfloat-abi=hard')
$cflags = $cpu + @('-g3', '-Og', '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra', '-Werror', '-DSTM32U5A5xx', '-DUSE_HAL_DRIVER')

$replaced = @('key_matrix.c', 'usb_keyboard.c')
$sources = @()
$sources += Get-ChildItem "$fw\Core\Src\*.c" | Where-Object { $replaced -notcontains $_.Name } | ForEach-Object FullName
$sources += Get-ChildItem "$fw\USB\Core\Src\usbd_*.c" | Where-Object { $_.Name -notmatch 'template' } | ForEach-Object FullName
foreach ($n in 'hal', 'hal_cortex', 'hal_dma', 'hal_dma_ex', 'hal_exti', 'hal_flash', 'hal_flash_ex', 'hal_gpio', 'hal_pwr', 'hal_pwr_ex', 'hal_rcc', 'hal_rcc_ex', 'hal_pcd', 'hal_pcd_ex', 'll_usb') {
    $sources += "$fw\Drivers\STM32U5xx_HAL_Driver\Src\stm32u5xx_$n.c"
}
$sources += "$diag\Drivers_ADC\Src\stm32u5xx_hal_adc.c", "$diag\Drivers_ADC\Src\stm32u5xx_hal_adc_ex.c"
foreach ($n in 'tmr_diag', 'tmr_analysis', 'tmr_hw', 'usb_cdc_diag') { $sources += "$diag\Core\Src\$n.c" }
$startup = Get-ChildItem "$fw\Core\Startup\*.s" | ForEach-Object FullName

$objects = @()
foreach ($s in $sources + $startup) {
    $obj = Join-Path $out (([IO.Path]::GetFileNameWithoutExtension($s)) + '.o')
    if ($s -like '*.s') { & $gcc @cpu -x assembler-with-cpp -c $s -o $obj }
    else { & $gcc @cflags @inc -c $s -o $obj }
    if ($LASTEXITCODE) { throw "Compile failed: $s" }
    $objects += $obj
}
$elf = Join-Path $out 'tmr-test.elf'
& $gcc @cpu "-T$fw\STM32U5A5RJTX_FLASH.ld" --specs=nano.specs --specs=nosys.specs "-Wl,--gc-sections,-Map=$out\tmr-test.map" "-Wl,--print-memory-usage" @objects -o $elf
if ($LASTEXITCODE) { throw 'Link failed' }
& $objcopy -O binary $elf "$out\tmr-test.bin"
& $objcopy -O ihex $elf "$out\tmr-test.hex"
& $size $elf
Write-Output "BUILD OK (isolated): $out\tmr-test.hex. Nothing was flashed."
