# Builds (unless -NoBuild) and launches emberclutch.3dsx in the Azahar emulator.
# The emulated SD card (dev saves) lives in %APPDATA%\Azahar\sdmc\3ds\emberclutch.
# Default keys: A=A, B=S, X=Z, Y=X, L=Q, R=W, START=M; the mouse is the stylus.
# Emulator speed is not representative of an old 3DS; sign off performance on hardware.
param(
    [switch]$NoBuild,
    [switch]$ResetSave
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$azahar = "C:\Program Files\Azahar\azahar.exe"
if (-not (Test-Path $azahar)) { throw "Azahar not found. Install it: winget install AzaharEmu.Azahar" }

if (-not $NoBuild) { & (Join-Path $PSScriptRoot "build.ps1") }
if ($ResetSave) {
    $saves = Join-Path $env:APPDATA "Azahar\sdmc\3ds\emberclutch"
    foreach ($f in "save.a", "save.b", "dev-save.bin") {  # A/B slots + the pre-WP8 file
        Remove-Item (Join-Path $saves $f) -ErrorAction SilentlyContinue
    }
}
Get-Process azahar -ErrorAction SilentlyContinue | Stop-Process
Start-Process $azahar -ArgumentList ('"' + (Join-Path $root "emberclutch.3dsx") + '"')
