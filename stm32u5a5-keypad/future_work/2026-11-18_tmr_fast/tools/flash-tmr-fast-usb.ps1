# Programs the TMR FAST-SCAN TEST image (output\arm-build\tmr-fast.hex) over USB DFU.
# This is NOT the keyboard firmware. To go back, use the normal Build + Flash USB task.
# Same steps as tools\flash-usb.ps1: no ST-Link, enter DFU with BOOT + RST first.
param(
    [string]$ProgrammerPath = '',
    [string]$Port = '',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$image = Join-Path $root 'output\arm-build\tmr-fast.hex'
if (!(Test-Path -LiteralPath $image)) { throw 'TMR fast-scan image not found. Run tools\build-tmr-fast.ps1 (or the Build TMR fast task) first.' }

if (!$ProgrammerPath) {
    $installed = Join-Path ${env:ProgramFiles} 'STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe'
    if (Test-Path -LiteralPath $installed) { $ProgrammerPath = $installed }
    else {
        $command = Get-Command STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue
        if ($command) { $ProgrammerPath = $command.Source }
    }
}
if (!$ProgrammerPath -or !(Test-Path -LiteralPath $ProgrammerPath)) {
    throw 'STM32CubeProgrammer CLI not found. Supply -ProgrammerPath with its full executable path.'
}
if ($Port -and $Port -notmatch '^USB\d+$') { throw 'DFU port must be USB1, USB2, etc.' }

if ($CheckOnly) {
    Write-Output "USB programmer found: $ProgrammerPath"
    Write-Output "Firmware found: $image"
    Write-Output 'Configuration check passed. No hardware accessed.'
    return
}

Write-Host 'USB DFU: connect devboard USB-C; hold BOOT, press/release RST, release BOOT.'
Write-Host 'Close/disconnect the CubeProgrammer GUI (and the TMR console) before using this task.'

$listing = @(& $ProgrammerPath -l usb 2>&1)
$listExitCode = $LASTEXITCODE
$listing | ForEach-Object { Write-Host $_ }
if ($listExitCode -ne 0) { throw 'USB DFU scan failed. Check the USB data cable and CubeProgrammer USB driver.' }
$text = $listing -join "`n"
$ports = @([regex]::Matches($text, '(?im)^\s*(?:Device Index|USB Port)\s*:\s*(USB\d+)\s*$') |
    ForEach-Object { $_.Groups[1].Value.ToUpperInvariant() } | Select-Object -Unique)
if ($ports.Count -eq 0) {
    throw 'No USB DFU board found. Hold BOOT, press/release RST, release BOOT, then run this again. Use a USB data cable.'
}
if (!$Port) {
    if ($ports.Count -ne 1) { throw 'Multiple DFU boards found. Unplug the others, or run this script with -Port USB1 (choose the listed port).' }
    $Port = $ports[0]
} elseif ($ports -notcontains $Port.ToUpperInvariant()) {
    throw "Requested DFU port $Port was not found in the USB scan."
}

# -w writes the HEX (it carries its own addresses), -v verifies. No erase-all, no option bytes.
& $ProgrammerPath -c "port=$Port" -w $image -v
if ($LASTEXITCODE -ne 0) { throw 'USB programming/verification failed. Read the CubeProgrammer error above; enter DFU again before retrying.' }
Write-Host ''
Write-Host 'TMR fast-scan test flashed and verified. Leave BOOT released and press RST.' -ForegroundColor Green
Write-Host 'Then open the console: TMR test console task, or tools\tmr-console.ps1'
