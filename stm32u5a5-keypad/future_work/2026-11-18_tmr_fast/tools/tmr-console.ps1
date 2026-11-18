# Simple serial console for the TMR test image. Finds the board's COM port by
# its USB ID (0483:5712, see tmr_config.h), shows everything it prints, and
# sends the keys you type (t, s, l, r, z, c, m, x, f, g, h). Ctrl+C quits.
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
Write-Host "Opening $Port. Keys: t self-test, s summary, l live, r raw, z re-zero, c clear, m learn, x sweep, f fast, g rapid trigger, h help. Ctrl+C quits." -ForegroundColor Cyan

$sp = New-Object System.IO.Ports.SerialPort $Port, 115200
$sp.DtrEnable = $true   # tells the board a terminal is open: it prints the report
$sp.RtsEnable = $true
$sp.ReadTimeout = 50
$canType = $true
# If this window cannot report single key presses (VS Code task panels often
# cannot), a background thread reads whole lines instead: type a letter and
# press Enter, and the first character of each line is sent to the board.
$lineQueue = New-Object System.Collections.Concurrent.ConcurrentQueue[string]
$lineReader = $null
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
                Write-Host '(This window cannot read single keys. Type a command letter and press Enter instead.)'
                $lineReader = [powershell]::Create().AddScript({
                    param($q)
                    while ($true) {
                        $l = [Console]::In.ReadLine()
                        if ($null -eq $l) { break }
                        $q.Enqueue($l)
                    }
                }).AddArgument($lineQueue)
                [void]$lineReader.BeginInvoke()
            }
        }
        $typed = $null
        while ($lineQueue.TryDequeue([ref]$typed)) {
            if ($typed.Length -gt 0) { $sp.Write($typed.Substring(0, 1)) } else { $sp.Write("`r") }
        }
        Start-Sleep -Milliseconds 15
    }
} catch [System.IO.IOException], [System.UnauthorizedAccessException] {
    Write-Host ''
    Write-Host "Lost $Port ($($_.Exception.Message)). Board reset or unplugged? Run the console again." -ForegroundColor Yellow
} finally {
    if ($lineReader) { $lineReader.Stop() }
    if ($sp.IsOpen) { $sp.Close() }
}
