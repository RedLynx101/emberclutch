# Sends emberclutch.3dsx to the 3DS over Wi-Fi and launches it.
# On the 3DS: open the Homebrew Launcher and press Y (netloader) first.
# Without -Address, 3dslink tries to find the console by broadcast.
param(
    [string]$Address = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exe = "C:\msys64\opt\devkitpro\tools\bin\3dslink.exe"
$file = Join-Path $root "emberclutch.3dsx"
if (-not (Test-Path $file)) { throw "Build first: tools\build.ps1" }

$args3ds = @()
if ($Address) { $args3ds += @("-a", $Address) }
& $exe @args3ds $file
if ($LASTEXITCODE -ne 0) { throw "3dslink failed (is the Homebrew Launcher netloader open?)" }
