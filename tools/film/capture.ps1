# The trailer's footage (docs/plan/trailer.md, V3): runs tests/film/<script>.txt in the headless emulator
# (tools/wsl/autotest.sh), each recording its reels (autotest `film start`: every frame, raw), then turns
# every reel into build/film/reels/<reel>.mp4 (the 3DS's own pixels scaled 4x, sharp, 60 fps), a still from
# its middle, and build/film/contact.png (every reel's still, labelled).
#   tools\film\capture.ps1 [-Scripts a_den,d_fly] [-Parallel 4] [-NoBuild] [-ConvertOnly]
param(
    [string[]]$Scripts,
    [int]$Parallel = 4,
    [switch]$NoBuild,
    [switch]$ConvertOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$film = Join-Path $root "build\film"
$reels = Join-Path $film "reels"
New-Item -ItemType Directory -Force $reels | Out-Null

function ConvertTo-WslPath([string]$p) {
    $full = (Resolve-Path -LiteralPath $p).Path
    "/mnt/" + $full.Substring(0, 1).ToLower() + ($full.Substring(2) -replace "\\", "/")
}

if (-not $Scripts) { $Scripts = Get-ChildItem (Join-Path $root "tests\film") -Filter "*.txt" | Where-Object { $_.BaseName -ne "spike" } | ForEach-Object { $_.BaseName } }

if (-not $ConvertOnly) {
    if (-not $NoBuild) { & (Join-Path $root "tools\build.ps1") | Out-Null }
    $game = ConvertTo-WslPath (Join-Path $root "emberclutch.3dsx")
    $runner = ConvertTo-WslPath (Join-Path $root "tools\wsl\autotest.sh")
    $dspWin = Join-Path $env:APPDATA "Azahar\sdmc\3ds\dspfirm.cdc"
    $queue = [System.Collections.Queue]::new($Scripts)
    $jobs = @()
    while ($queue.Count -gt 0 -or $jobs.Count -gt 0) {
        while ($queue.Count -gt 0 -and $jobs.Count -lt $Parallel) {
            $s = $queue.Dequeue()
            $out = Join-Path $film "runs\$s"
            New-Item -ItemType Directory -Force $out | Out-Null
            $wslArgs = @("-d", "emberclutch-test", "--", "bash", $runner, "--game", $game,
                "--script", (ConvertTo-WslPath (Join-Path $root "tests\film\$s.txt")), "--out", (ConvertTo-WslPath $out), "--timeout", "600")
            if (Test-Path $dspWin) { $wslArgs += @("--dsp", (ConvertTo-WslPath $dspWin)) }
            $jobs += Start-Job -Name $s -ScriptBlock { param($a) & wsl.exe @a 2>&1 | Select-Object -Last 1 } -ArgumentList (, $wslArgs)
        }
        Wait-Job $jobs -Any | Out-Null
        foreach ($j in @($jobs | Where-Object { $_.State -ne "Running" })) {
            "{0}: {1}" -f $j.Name, ((Receive-Job $j) -join " ")
            Remove-Job $j
            $jobs = @($jobs | Where-Object { $_.Id -ne $j.Id })
        }
    }
}

# Every reel: the raw framebuffers (240-tall columns, bottom to top, BGR) turned upright, scaled 4x.
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
        & ffmpeg -loglevel error -y -f rawvideo -pix_fmt bgr24 -s "240x$w" -r 60 -i $raw.FullName `
            -vf "transpose=2,scale=$($w * 4):960:flags=neighbor" -c:v libx264 -crf 12 -preset medium -pix_fmt yuv420p $mp4
        $still = Join-Path $reels "$name.png"
        & ffmpeg -loglevel error -y -f rawvideo -pix_fmt bgr24 -s "240x$w" -r 60 -i $raw.FullName `
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
