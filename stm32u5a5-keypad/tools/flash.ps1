param([string]$CubeIdeRoot='C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE', [switch]$CheckOnly)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!(Test-Path "$root\build\keypad.elf")){throw 'Build first.'}
$ocd=Get-ChildItem "$CubeIdeRoot\plugins" -Filter openocd.exe -Recurse | Select-Object -First 1
if(!$ocd){throw 'OpenOCD not found'}
# CubeIDE stores ST target scripts in a different plugin from openocd.exe.
$target=Get-ChildItem "$CubeIdeRoot\plugins" -Filter stm32u5x.cfg -Recurse | Select-Object -First 1
if(!$target){throw 'STM32U5 OpenOCD target script not found'}
$scripts=Split-Path (Split-Path $target.FullName)
if(!(Test-Path "$scripts\interface\stlink-dap.cfg")){throw 'ST-Link interface script not found'}
# This option verifies tool/image paths without opening the probe or flashing.
if($CheckOnly){Write-Output 'OpenOCD, STM32U5/ST-Link scripts and firmware ELF found. No hardware access performed.'; return}
$elf="$root/build/keypad.elf".Replace('\','/')
& $ocd.FullName -s $scripts -f "$root\tools\openocd.cfg" -c "program {$elf} verify reset exit"
if($LASTEXITCODE){throw 'Flash failed; stop any active IDE debug session and check power/SWD.'}
