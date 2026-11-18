# Simple serial console for the TMR test image. Finds the board's COM port by
# its USB ID (0483:5712, see tmr_config.h), shows everything it prints, and
# sends the keys you type (t, s, l, r, z, c, m, h). Ctrl+C quits.
# Any other serial terminal works too (115200 or any baud; it is USB).
param([string]$Port = '', [int]$WaitSeconds = 30)
$ErrorActionPreference = 'Stop'
$idPattern = 'VID_0483&PID_5712'

function Find-Port {
    $dev = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue |
        Where-Object { $_.PNPDeviceID -match $idPattern -and $_.Name -match '\((COM\d+)\)' } |
        Select-Object -First 1
    if ($dev -and $dev.Name -match '\((COM\d+)\)') { return $Matches[1] }
    return ''
}

if (!$Port) {
    Write-Host "Looking for the TMR test board (USB $idPattern)..."
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    while (!($Port = Find-Port)) {
        if ((Get-Date) -gt $deadline) {
            throw "TMR test board not found. Is the TMR test flashed, BOOT released and RST pressed? Check Device Manager > Ports for 'USB Serial Device'."
        }
        Start-Sleep -Milliseconds 500
    }
}
Write-Host "Opening $Port. Keys: t self-test, s summary, l live, r raw, z re-zero, c clear, m learn, h help. Ctrl+C quits." -ForegroundColor Cyan

$sp = New-Object System.IO.Ports.SerialPort $Port, 115200
$sp.DtrEnable = $true   # tells the board a terminal is open: it prints the report
$sp.RtsEnable = $true
$sp.ReadTimeout = 50
$canType = $true
try {
    $sp.Open()
    while ($true) {
        if ($sp.BytesToRead -gt 0) { Write-Host -NoNewline $sp.ReadExisting() }
        if ($canType) {
            try {
                while ([Console]::KeyAvailable) {
                    $k = [Console]::ReadKey($true)
                    if ($k.KeyChar -ne [char]0) { $sp.Write([string]$k.KeyChar) }
                }
            } catch {
                $canType = $false
                Write-Host '(This window cannot read single keys: output only. Use a terminal program to send commands.)'
            }
        }
        Start-Sleep -Milliseconds 15
    }
} catch [System.IO.IOException], [System.UnauthorizedAccessException] {
    Write-Host ''
    Write-Host "Lost $Port ($($_.Exception.Message)). Board reset or unplugged? Run the console again." -ForegroundColor Yellow
} finally {
    if ($sp.IsOpen) { $sp.Close() }
}
