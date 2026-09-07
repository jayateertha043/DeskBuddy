# Detects the connected board by USB Vendor ID (VID) and runs PlatformIO for the
# matching environment. Falls back to the ESP32-C3 env if nothing is detected.
#
#   VID 303A  -> esp32-c3-devkitm-1 (native USB Serial/JTAG)
#   VID 1A86  -> nodemcuv2 (CH340 USB-serial bridge)
#   VID 10C4  -> nodemcuv2 (CP2102 USB-serial bridge)
#
# Usage: auto_env.ps1 [-Targets upload,monitor]
param(
    [string[]]$Targets = @('upload', 'monitor')
)

$ErrorActionPreference = 'Stop'

$pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe'
if (-not (Test-Path $pio)) {
    $cmd = Get-Command platformio -ErrorAction SilentlyContinue
    if ($cmd) { $pio = $cmd.Source } else { throw 'platformio.exe not found.' }
}

# Map of USB VID -> PlatformIO environment. First match wins.
$vidToEnv = [ordered]@{
    'VID_303A' = 'esp32-c3-devkitm-1'
    'VID_1A86' = 'nodemcuv2'
    'VID_10C4' = 'nodemcuv2'
}

$ids = @(Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue |
    Where-Object { $_.PNPDeviceID } |
    Select-Object -ExpandProperty PNPDeviceID)

$env = $null
foreach ($vid in $vidToEnv.Keys) {
    if ($ids | Where-Object { $_ -like "*$vid*" }) {
        $env = $vidToEnv[$vid]
        Write-Host "Detected $vid -> building environment '$env'" -ForegroundColor Green
        break
    }
}

if (-not $env) {
    $env = 'esp32-c3-devkitm-1'
    Write-Host "No known board VID detected; falling back to '$env'" -ForegroundColor Yellow
}

$args = @('run', '-e', $env)
# Accept either an array or a single comma-separated string of targets.
# 'build' is the default action in `pio run`; it is not a real target name.
$flat = @($Targets) -join ',' -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ }
foreach ($t in $flat) { if ($t -ne 'build') { $args += @('-t', $t) } }

Write-Host "> platformio $($args -join ' ')" -ForegroundColor Cyan
& $pio @args
exit $LASTEXITCODE
