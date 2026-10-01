# Runs a scripted session (src/app/autotest.hpp) in Azahar with nobody at the controls and
# collects its screenshots as PNGs in build/autotest/<script name>/.
#   tools\autotest.ps1 tests\autotest\tour.txt [-ResetSave] [-NoBuild] [-TimeoutSec 180]
#   tools\autotest.ps1 tests\autotest\tour.txt -Headless [-Speed 0] [-SystemClock] [-New3DS]
# The script is copied to the emulated SD card as autotest.txt, the game plays it and quits,
# and the script file is removed again so a normal launch plays normally. The emulator's dev
# save is set aside first and put back afterwards: a run never touches it.
# -Headless runs it in the emberclutch-test WSL distro instead (docs/tech/headless-emulator.md):
# no window, no shared SD card (so no waiting on other runs), as fast as the PC goes, and the
# 3DS clock fixed at the same morning every run. Set it up once with tools\wsl\install.ps1.
param(
    [Parameter(Mandatory = $true)][string]$Script,
    [switch]$ResetSave,
    [switch]$NoBuild,
    [switch]$KeepSave,  # leave this run's save for the next run (the dev save stays backed up)
    [int]$TimeoutSec = 180,
    [string]$Game = "",  # what Azahar boots: the .3dsx by default, or an installed CIA's .app
    [switch]$Headless,
    [int]$Speed = 0,      # headless: Azahar's frame limit in percent (0: no limit)
    [switch]$SystemClock, # headless: the PC's clock instead of the fixed morning
    [switch]$New3DS       # headless: emulate a New 3DS (the default is the old 3DS, our floor)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$azahar = "C:\Program Files\Azahar\azahar.exe"
$scriptPath = (Resolve-Path $Script).Path
$sd = Join-Path $env:APPDATA "Azahar\sdmc\3ds\emberclutch"
$shots = Join-Path $sd "shots"
$emuLog = Join-Path $env:APPDATA "Azahar\log\azahar_log.txt"
$runs = Join-Path $root "build\autotest"
$name = [IO.Path]::GetFileNameWithoutExtension($scriptPath)
$out = Join-Path $runs $name
$backup = Join-Path $runs "save-backup"
$saves = "save.a", "save.b"
$distro = "emberclutch-test"

function ConvertTo-WslPath([string]$p) {
    $full = [IO.Path]::GetFullPath($p)
    "/mnt/" + $full.Substring(0, 1).ToLower() + ($full.Substring(2) -replace "\\", "/")
}

if ($Headless) {
    if ($Game) { throw "-Headless boots the .3dsx only (no installed CIA)" }
    $distros = (wsl.exe -l -q) -replace "`0", ""  # wsl.exe answers in UTF-16
    if (-not ($distros -contains $distro)) { throw "No $distro WSL distro. Set it up: tools\wsl\install.ps1" }
} elseif (-not (Test-Path $azahar)) { throw "Azahar not found. Install it: winget install AzaharEmu.Azahar" }

if (-not $NoBuild) { & (Join-Path $PSScriptRoot "build.ps1") }
# One run at a time on this PC: the emulator and its SD card are shared (runs from other
# worktrees too), and a run stops any Azahar it finds. The lock is an open file, let go when
# this PowerShell ends however it ends; others wait for it (up to 30 minutes). Headless runs
# each have their own emulator and SD card, so they don't take it.
$lockPath = Join-Path $env:TEMP "emberclutch-autotest.lock"
$lock = $null
for ($waited = 0; -not $Headless -and -not $lock; $waited += 5) {
    try { $lock = [IO.File]::Open($lockPath, "OpenOrCreate", "ReadWrite", "None") }
    catch {
        if ($waited -ge 1800) { throw "Another autotest has held the emulator for 30 minutes" }
        if ($waited -eq 0) { Write-Host "Waiting for another autotest run to finish..." }
        Start-Sleep -Seconds 5
    }
}
# Let go of it however the run ends, even when another run follows in this same PowerShell.
try {
if ($Headless) {
    # The save a run starts from: none (-ResetSave), the one a -KeepSave run left in this
    # worktree, or a copy of the dev save. The dev save itself is only ever read.
    $keptSave = Join-Path $runs "headless-save"
    $saveIn = $null
    if (-not $ResetSave) { $saveIn = if (Test-Path $keptSave) { $keptSave } else { $sd } }
    $raw = Join-Path $runs ".headless\$name"
    Remove-Item $raw -Recurse -Force -ErrorAction SilentlyContinue
    $wslArgs = @("-d", $distro, "--", "bash", (ConvertTo-WslPath "$PSScriptRoot\wsl\autotest.sh"),
        "--game", (ConvertTo-WslPath (Join-Path $root "emberclutch.3dsx")),
        "--script", (ConvertTo-WslPath $scriptPath), "--out", (ConvertTo-WslPath $raw),
        "--timeout", $TimeoutSec, "--speed", $Speed,
        "--clock", $(if ($SystemClock) { "system" } else { "fixed" }))
    if ($saveIn -and (Test-Path $saveIn)) { $wslArgs += @("--save-in", (ConvertTo-WslPath $saveIn)) }
    if ($New3DS) { $wslArgs += "--new3ds" }
    $dsp = Join-Path $env:APPDATA "Azahar\sdmc\3ds\dspfirm.cdc"  # the sound (ndsp needs it)
    if (Test-Path $dsp) { $wslArgs += @("--dsp", (ConvertTo-WslPath $dsp)) }
    & wsl.exe @wslArgs | ForEach-Object { "headless: $_" }
    $finished = $LASTEXITCODE -eq 0
    $shots = Join-Path $raw "shots"
    $emuLog = Join-Path $raw "azahar_log.txt"
    Remove-Item $keptSave -Recurse -Force -ErrorAction SilentlyContinue
    if ($KeepSave) { Copy-Item (Join-Path $raw "save") $keptSave -Recurse }
} else {
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
    # The game's log beside them (each picture's triangles and the like: autotest::log).
    $gameLog = Join-Path $shots "log.txt"
    if (Test-Path $gameLog) { Copy-Item $gameLog $out }
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
if (Test-Path $emuLog) {
    $bad = @(Select-String -Path $emuLog -Pattern "unmapped" -SimpleMatch)
    if ($bad.Count) {
        "WARNING: $($bad.Count) unmapped memory accesses (a crash on the 3DS). First ones:"
        $bad | Select-Object -First 5 | ForEach-Object { "  " + ($_.Line -replace '^\[\s*[\d.]+\]\s*', '') }
    } else { "memory: no unmapped accesses" }
}
} finally { if ($lock) { $lock.Close() } }
