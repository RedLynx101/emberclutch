# Builds emberclutch.3dsx with devkitPro (MSYS2).
param(
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$bash = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $bash)) { throw "MSYS2 bash not found at $bash" }

$unixRoot = $root -replace "\\", "/"
if ($unixRoot -match "^([A-Za-z]):(.*)$") { $unixRoot = "/" + $Matches[1].ToLowerInvariant() + $Matches[2] }

$cleanCmd = if ($Clean) { "make clean && " } else { "" }
# make doesn't track romfs/ (models, clips, sounds): drop the .3dsx so it is always repacked.
Remove-Item (Join-Path $root "emberclutch.3dsx") -ErrorAction SilentlyContinue
& $bash -lc "source /etc/profile.d/devkit-env.sh && cd '$unixRoot' && $cleanCmd make"
if ($LASTEXITCODE -ne 0) { throw "3DS build failed with exit code $LASTEXITCODE" }

Get-Item (Join-Path $root "emberclutch.3dsx"), (Join-Path $root "emberclutch.smdh") |
    Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
