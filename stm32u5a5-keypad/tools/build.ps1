param([string]$CubeIdeRoot='C:\ST\STM32CubeIDE_2.1.1\STM32CubeIDE')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
function Find-Tool($name){
 $tool=Get-ChildItem -LiteralPath "$CubeIdeRoot\plugins" -Filter $name -Recurse | Select-Object -First 1
 if(!$tool){throw "Cannot find $name under $CubeIdeRoot. Pass -CubeIdeRoot for your installation."}
 return $tool.FullName
}
$gcc=Find-Tool 'arm-none-eabi-gcc.exe';$bin=Split-Path $gcc
$cmake=Find-Tool 'cmake.exe';$ninja=Find-Tool 'ninja.exe'
$env:PATH="$bin;$(Split-Path $ninja);$env:PATH"
& $cmake -S $root -B "$root\build" -G Ninja "-DCMAKE_MAKE_PROGRAM=$($ninja.Replace('\','/'))" "-DCMAKE_C_COMPILER=$($gcc.Replace('\','/'))" "-DCMAKE_ASM_COMPILER=$($gcc.Replace('\','/'))" "-DCMAKE_OBJCOPY=$($bin.Replace('\','/'))/arm-none-eabi-objcopy.exe" "-DCMAKE_SIZE=$($bin.Replace('\','/'))/arm-none-eabi-size.exe"
if($LASTEXITCODE){throw 'Configure failed'}
& $cmake --build "$root\build"
if($LASTEXITCODE){throw 'Build failed'}
