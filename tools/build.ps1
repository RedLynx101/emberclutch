# Builds emberclutch.3dsx with devkitPro (MSYS2): the dev build, or with -Player the player build
# (make DEV=0: emberclutch-player.3dsx, in build-player/; no dev menu, tracer or Y screenshots; D135).
param(
    [switch]$Clean,
    [switch]$Player
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$bash = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $bash)) { throw "MSYS2 bash not found at $bash" }

$unixRoot = $root -replace "\\", "/"
if ($unixRoot -match "^([A-Za-z]):(.*)$") { $unixRoot = "/" + $Matches[1].ToLowerInvariant() + $Matches[2] }

$name = if ($Player) { "emberclutch-player" } else { "emberclutch" }
$makeArgs = if ($Player) { "DEV=0" } else { "" }
$cleanCmd = if ($Clean) { "make $makeArgs clean && " } else { "" }
# make doesn't track romfs/ (models, clips, sounds): drop the .3dsx so it is always repacked.
Remove-Item (Join-Path $root "$name.3dsx") -ErrorAction SilentlyContinue
& $bash -lc "source /etc/profile.d/devkit-env.sh && cd '$unixRoot' && $cleanCmd make $makeArgs"
if ($LASTEXITCODE -ne 0) { throw "3DS build failed with exit code $LASTEXITCODE" }

Get-Item (Join-Path $root "$name.3dsx"), (Join-Path $root "$name.smdh") |
    Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
