# Runs a scripted session (src/app/autotest.hpp) in Azahar with nobody at the controls and
# collects its screenshots as PNGs in build/autotest/<script name>/.
#   tools\autotest.ps1 tests\autotest\tour.txt [-ResetSave] [-NoBuild] [-TimeoutSec 180]
# The script is copied to the emulated SD card as autotest.txt, the game plays it and quits,
# and the script file is removed again so a normal launch plays normally. The emulator's dev
# save is set aside first and put back afterwards: a run never touches it.
param(
    [Parameter(Mandatory = $true)][string]$Script,
    [switch]$ResetSave,
    [switch]$NoBuild,
    [switch]$KeepSave,  # leave this run's save for the next run (the dev save stays backed up)
    [int]$TimeoutSec = 180,
    [string]$Game = ""  # what Azahar boots: the .3dsx by default, or an installed CIA's .app
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$azahar = "C:\Program Files\Azahar\azahar.exe"
if (-not (Test-Path $azahar)) { throw "Azahar not found. Install it: winget install AzaharEmu.Azahar" }
$scriptPath = (Resolve-Path $Script).Path
$sd = Join-Path $env:APPDATA "Azahar\sdmc\3ds\emberclutch"
$shots = Join-Path $sd "shots"
$runs = Join-Path $root "build\autotest"
$out = Join-Path $runs ([IO.Path]::GetFileNameWithoutExtension($scriptPath))
$backup = Join-Path $runs "save-backup"
$saves = "save.a", "save.b"

if (-not $NoBuild) { & (Join-Path $PSScriptRoot "build.ps1") }
New-Item -ItemType Directory -Force $sd | Out-Null
$kept = Join-Path $backup "kept.txt"  # a previous run kept its save: the backup is the dev save
if (-not (Test-Path $kept)) {
    Remove-Item $backup -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force $backup | Out-Null
    foreach ($f in $saves) {
        $p = Join-Path $sd $f
        if (Test-Path $p) { Copy-Item $p $backup }
    }
}
if ($ResetSave) {
    foreach ($f in $saves) { Remove-Item (Join-Path $sd $f) -ErrorAction SilentlyContinue }
}
Remove-Item $shots -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item $scriptPath (Join-Path $sd "autotest.txt")

try {
    Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process
    if (-not $Game) { $Game = Join-Path $root "emberclutch.3dsx" }
    Start-Process $azahar -ArgumentList ('"' + $Game + '"')
    $done = Join-Path $shots "done.txt"
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while (-not (Test-Path $done) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }
    $finished = Test-Path $done
    Start-Sleep -Milliseconds 500
} finally {
    Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process
    Remove-Item (Join-Path $sd "autotest.txt") -ErrorAction SilentlyContinue
    if ($KeepSave) {
        Set-Content $kept "the save in the emulator is a test run's; the dev save is here"
    } else {
        foreach ($f in $saves) {  # put the dev save back
            Remove-Item (Join-Path $sd $f) -ErrorAction SilentlyContinue
            $b = Join-Path $backup $f
            if (Test-Path $b) { Copy-Item $b $sd }
        }
        Remove-Item $kept -ErrorAction SilentlyContinue
    }
}

Add-Type -AssemblyName System.Drawing
Remove-Item $out -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $out | Out-Null
$count = 0
if (Test-Path $shots) {
    foreach ($bmp in Get-ChildItem $shots -Filter *.bmp) {
        $img = [System.Drawing.Image]::FromFile($bmp.FullName)
        $img.Save((Join-Path $out ($bmp.BaseName + ".png")), [System.Drawing.Imaging.ImageFormat]::Png)
        $img.Dispose()
        $count++
    }
}
# Contact sheets: four steps per sheet, each its top screen over its bottom screen, labelled.
$names = @(Get-ChildItem $out -Filter *_top.png | ForEach-Object { $_.BaseName -replace "_top$", "" } | Sort-Object)
$font = New-Object System.Drawing.Font("Segoe UI", 11)
for ($s = 0; $s -lt $names.Count; $s += 4) {
    $sheet = New-Object System.Drawing.Bitmap(820, 1000)
    $g = [System.Drawing.Graphics]::FromImage($sheet)
    $g.Clear([System.Drawing.Color]::FromArgb(24, 24, 28))
    for ($k = 0; $k -lt 4 -and $s + $k -lt $names.Count; $k++) {
        $n = $names[$s + $k]
        $x = 10 + ($k % 2) * 405
        $y = 5 + [math]::Floor($k / 2) * 497
        $g.DrawString($n, $font, [System.Drawing.Brushes]::White, $x, $y)
        $top = [System.Drawing.Image]::FromFile((Join-Path $out "$($n)_top.png"))
        $g.DrawImage($top, $x, $y + 20, 400, 240)
        $top.Dispose()
        $bottomPath = Join-Path $out "$($n)_bottom.png"
        if (Test-Path $bottomPath) {
            $bottom = [System.Drawing.Image]::FromFile($bottomPath)
            $g.DrawImage($bottom, $x + 40, $y + 262, 320, 240)
            $bottom.Dispose()
        }
    }
    $g.Dispose()
    $sheet.Save((Join-Path $out ("sheet-{0:D2}.png" -f ($s / 4 + 1))), [System.Drawing.Imaging.ImageFormat]::Png)
    $sheet.Dispose()
}
if ($finished) { "finished: $count screenshots in $out" } else { "TIMED OUT after $TimeoutSec s: $count screenshots in $out" }
# Reads and writes where no memory is mapped: the emulator logs them and carries on (a null
# read gives 0), the 3DS faults. That's how the first hardware crash hid (2026-09-24).
$emuLog = Join-Path $env:APPDATA "Azahar\log\azahar_log.txt"
if (Test-Path $emuLog) {
    $bad = @(Select-String -Path $emuLog -Pattern "unmapped" -SimpleMatch)
    if ($bad.Count) {
        "WARNING: $($bad.Count) unmapped memory accesses (a crash on the 3DS). First ones:"
        $bad | Select-Object -First 5 | ForEach-Object { "  " + ($_.Line -replace '^\[\s*[\d.]+\]\s*', '') }
    } else { "memory: no unmapped accesses" }
}
