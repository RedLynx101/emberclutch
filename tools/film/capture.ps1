# The trailer's footage (docs/plan/trailer.md): runs tests/film/<script>.txt in the headless emulator
# (tools/wsl/autotest.sh) and films its reels (autotest `film start`).
# By default at 6x the 3DS's resolution: the game hands over every frame and tools/wsl/grab.py reads it off the
# emulator's display into build/film/grab/<reel>.mkv (60 fps). A script with a two-screen reel ("film start
# <reel> both") is filmed side by side (both screens, 4320x1440 at 6x); the rest the top screen alone (2400x1440).
# Each reel also gets build/film/grab/<reel>.png (its middle frame), and build/film/contact.png labels them all.
# -Native: the game's own 400x240 frames instead (written raw to the SD card), each made build/film/reels/<reel>.mp4
# (the pixels scaled 4x, sharp).
#   tools\film\capture.ps1 [-Scripts a_den,d_fly] [-Parallel 4] [-Scale 6] [-NoBuild] [-ConvertOnly] [-Native]
param(
    [string[]]$Scripts,
    [int]$Parallel = 4,
    [int]$Scale = 6,
    [switch]$NoBuild,
    [switch]$ConvertOnly,
    [switch]$Native
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$film = Join-Path $root "build\film"
$reels = Join-Path $film "reels"
$grab = Join-Path $film "grab"
New-Item -ItemType Directory -Force $reels, $grab | Out-Null

function ConvertTo-WslPath([string]$p) {
    $full = (Resolve-Path -LiteralPath $p).Path
    "/mnt/" + $full.Substring(0, 1).ToLower() + ($full.Substring(2) -replace "\\", "/")
}

$Scripts = @($Scripts | ForEach-Object { $_ -split "," } | Where-Object { $_ })  # (powershell -File passes a,b as one string)
if (-not $Scripts) { $Scripts = Get-ChildItem (Join-Path $root "tests\film") -Filter "*.txt" | Where-Object { $_.BaseName -ne "spike" } | ForEach-Object { $_.BaseName } }
$ffmpeg = (Get-Command ffmpeg).Source

if (-not $ConvertOnly) {
    if (-not $NoBuild) { & (Join-Path $root "tools\build.ps1") | Out-Null }
    $game = ConvertTo-WslPath (Join-Path $root "emberclutch.3dsx")
    $runner = ConvertTo-WslPath (Join-Path $root "tools\wsl\autotest.sh")
    # The sound (and so the cue sheets): ndsp needs a dspfirm.cdc on the SD card. Copied to %TEMP% first, as
    # tools/autotest.ps1 does: WSL can't see a packaged app's virtual %APPDATA%.
    $dspWin = Join-Path $env:APPDATA "Azahar\sdmc\3ds\dspfirm.cdc"
    if (Test-Path $dspWin) {
        $stage = Join-Path $env:TEMP "emberclutch-dsp"
        New-Item -ItemType Directory -Force $stage | Out-Null
        Copy-Item $dspWin (Join-Path $stage "dspfirm.cdc") -Force
        $dspWin = Join-Path $stage "dspfirm.cdc"
    }
    $queue = [System.Collections.Queue]::new($Scripts)
    $jobs = @()
    $tries = @{}
    while ($queue.Count -gt 0 -or $jobs.Count -gt 0) {
        while ($queue.Count -gt 0 -and $jobs.Count -lt $Parallel) {
            $s = $queue.Dequeue()
            $txt = Join-Path $root "tests\film\$s.txt"
            $out = Join-Path $film "runs\$s"
            New-Item -ItemType Directory -Force $out | Out-Null
            $wslArgs = @("-d", "emberclutch-test", "--", "bash", $runner, "--game", $game,
                "--script", (ConvertTo-WslPath $txt), "--out", (ConvertTo-WslPath $out), "--timeout", "5400")
            if (Test-Path $dspWin) { $wslArgs += @("--dsp", (ConvertTo-WslPath $dspWin)) }
            if (-not $Native) {
                # (the layout's size at the scale, exactly: grab.py makes Azahar's window fill it)
                $both = [bool](Select-String -Path $txt -Pattern "^film start \S+ both" -Quiet)
                $size = if ($both) { "{0}x{1}" -f (720 * $Scale), (240 * $Scale) } else { "{0}x{1}" -f (400 * $Scale), (240 * $Scale) }
                $wslArgs += @("--scale", "$Scale", "--screen", $size, "--layout", $(if ($both) { "3" } else { "1" }), "--fullscreen",
                    "--grab", (($grab -replace "\\", "/") + "/"), "--ffmpeg", (ConvertTo-WslPath $ffmpeg))
            }
            $jobs += Start-Job -Name $s -ScriptBlock { param($a) & wsl.exe @a 2>&1 | Select-Object -Last 1 } -ArgumentList (, $wslArgs)
        }
        Wait-Job $jobs -Any | Out-Null
        foreach ($j in @($jobs | Where-Object { $_.State -ne "Running" })) {
            "{0}: {1}" -f $j.Name, ((Receive-Job $j) -join " ")
            Remove-Job $j
            $jobs = @($jobs | Where-Object { $_.Id -ne $j.Id })
            # (grab mode: every reel the script films in the grabber's log, or the script goes again, once)
            if (-not $Native) {
                $want = @(Select-String -Path (Join-Path $root "tests\film\$($j.Name).txt") -Pattern "^film start (\S+)" |
                    ForEach-Object { $_.Matches[0].Groups[1].Value })
                $log = Join-Path $film "runs\$($j.Name)\grab.txt"
                $got = if (Test-Path $log) { @(Select-String -Path $log -Pattern "^\[grab\] (\S+): \d+ frames" | ForEach-Object { $_.Matches[0].Groups[1].Value }) } else { @() }
                $missing = @($want | Where-Object { $got -notcontains $_ })
                $broken = (Test-Path $log) -and [bool](Select-String -Path $log -Pattern "Traceback" -Quiet)
                if ($missing.Count -or $broken) {
                    $tries[$j.Name] = 1 + $(if ($tries.ContainsKey($j.Name)) { $tries[$j.Name] } else { 0 })
                    "  {0}: {1}{2}" -f $j.Name, $(if ($missing.Count) { "missing " + ($missing -join ", ") } else { "the grabber failed" }),
                        $(if ($tries[$j.Name] -lt 2) { "; again" } else { "; giving up" })
                    if ($tries[$j.Name] -lt 2) { $queue.Enqueue($j.Name) }
                }
            }
        }
    }
}

if (-not $Native) {
    # Every reel's middle frame: full size (<reel>.png) and the top screen at 400x240 for the contact sheet.
    $stills = @()
    foreach ($s in $Scripts) {
        $log = Join-Path $film "runs\$s\grab.txt"
        if (-not (Test-Path $log)) { "${s}: no grab log"; continue }
        foreach ($m in Select-String -Path $log -Pattern "^\[grab\] (\S+): (\d+) frames") {
            $name, $frames = $m.Matches[0].Groups[1].Value, [int]$m.Matches[0].Groups[2].Value
            $mkv = Join-Path $grab "$name.mkv"
            if (-not (Test-Path $mkv) -or $frames -lt 1) { "${name}: missing"; continue }
            $still = Join-Path $grab "$name.png"
            & $ffmpeg -loglevel error -y -i $mkv -vf "select=eq(n\,$([int]($frames / 2)))" -frames:v 1 $still
            $small = Join-Path $grab "contact\$name.png"
            New-Item -ItemType Directory -Force (Split-Path $small) | Out-Null
            & $ffmpeg -loglevel error -y -i $still -vf "crop=$(400 * $Scale):$(240 * $Scale):0:0,scale=400:240:flags=area" $small
            "{0}: {1} frames ({2:N2} s)" -f $name, $frames, ($frames / 60.0)
            $stills += $small
        }
    }
    if ($stills.Count) {
        $all = @(Get-ChildItem (Join-Path $grab "contact") -Filter "*.png" | ForEach-Object { $_.FullName })  # (every reel so far)
        & py -3.12 (Join-Path $PSScriptRoot "contact.py") (Join-Path $film "contact.png") @all
    }
    return
}

# -Native: the raw framebuffers (240-tall columns, bottom to top, BGR) turned upright, scaled 4x.
$stills = @()
foreach ($s in $Scripts) {
    $dir = Join-Path $film "runs\$s\film"
    if (-not (Test-Path $dir)) { "${s}: no reels"; continue }
    foreach ($raw in Get-ChildItem $dir -Filter "*.raw") {
        $name = $raw.BaseName  # e.g. s03_pet_top
        $w = if ($name -like "*_bottom") { 320 } else { 400 }
        $frames = [int]($raw.Length / ($w * 240 * 3))
        if ($frames -lt 1) { "${name}: empty"; continue }
        $mp4 = Join-Path $reels "$name.mp4"
        & $ffmpeg -loglevel error -y -f rawvideo -pix_fmt bgr24 -s "240x$w" -r 60 -i $raw.FullName `
            -vf "transpose=2,scale=$($w * 4):960:flags=neighbor" -c:v libx264 -crf 12 -preset medium -pix_fmt yuv420p $mp4
        $still = Join-Path $reels "$name.png"
        & $ffmpeg -loglevel error -y -f rawvideo -pix_fmt bgr24 -s "240x$w" -r 60 -i $raw.FullName `
            -vf "select=eq(n\,$([int]($frames / 2))),transpose=2" -frames:v 1 $still
        "{0}: {1} frames ({2:N2} s)" -f $name, $frames, ($frames / 60.0)
        if ($name -like "*_top") { $stills += $still }
    }
}
# The contact sheet: every top reel's middle frame, labelled, four across.
if ($stills.Count) {
    $all = @(Get-ChildItem $reels -Filter "*_top.png" | ForEach-Object { $_.FullName })  # (every reel captured so far)
    & py -3.12 (Join-Path $PSScriptRoot "contact.py") (Join-Path $film "contact.png") @all
}
