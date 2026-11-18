# Programs the application through the STM32 ROM USB DFU bootloader.
# No ST-Link is used. Enter DFU physically with BOOT + RST first.
param(
    [string]$ProgrammerPath = '',
    [string]$Port = '',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$image = Join-Path $root 'build\keypad.hex'
if (!(Test-Path -LiteralPath $image)) { throw 'Firmware not found. Click Build or Build + Flash USB first.' }

# Prefer the standalone CubeProgrammer already installed on this computer.
# Optional parameter/PATH support lets this script work on another computer.
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

# Validation mode checks paths only: it neither opens USB nor programs the MCU.
if ($CheckOnly) {
    Write-Output "USB programmer found: $ProgrammerPath"
    Write-Output "Firmware found: $image"
    Write-Output 'Configuration check passed. No hardware accessed.'
    return
}

Write-Host 'USB DFU: connect devboard USB-C; hold BOOT, press/release RST, release BOOT.'
Write-Host 'Close/disconnect the CubeProgrammer GUI before using this task.'

# Enumerating DFU devices does not write firmware. Auto-select only when exactly
# one bootloader is present, rather than accidentally programming another board.
$listing = @(& $ProgrammerPath -l usb 2>&1)
$listExitCode = $LASTEXITCODE
$listing | ForEach-Object { Write-Host $_ }
if ($listExitCode -ne 0) { throw 'USB DFU scan failed. Check the USB data cable and CubeProgrammer USB driver.' }
$text = $listing -join "`n"
$ports = @([regex]::Matches($text, '(?im)^\s*(?:Device Index|USB Port)\s*:\s*(USB\d+)\s*$') |
    ForEach-Object { $_.Groups[1].Value.ToUpperInvariant() } | Select-Object -Unique)
if ($ports.Count -eq 0) {
    throw 'No USB DFU board found. Hold BOOT, press/release RST, release BOOT, then click Flash USB again. Use a USB data cable.'
}
if (!$Port) {
    if ($ports.Count -ne 1) { throw 'Multiple DFU boards found. Unplug the others, or run this script with -Port USB1 (choose the listed port).' }
    $Port = $ports[0]
} elseif ($ports -notcontains $Port.ToUpperInvariant()) {
    throw "Requested DFU port $Port was not found in the USB scan."
}

# HEX contains its flash addresses, so no separate start address is needed.
# -w writes the image; -v verifies it. No mass erase or option-byte changes.
# Do not use -g here: a physical reset with BOOT released gives a clean startup.
& $ProgrammerPath -c "port=$Port" -w $image -v
if ($LASTEXITCODE -ne 0) { throw 'USB programming/verification failed. Read the CubeProgrammer error above; enter DFU again before retrying.' }
Write-Host ''
Write-Host 'USB flash and verification completed. Leave BOOT released and press RST.' -ForegroundColor Green
Write-Host 'Open Notepad and press each key. Expected, row by row: a b c d / e f g h / i j k l / m n o.'
