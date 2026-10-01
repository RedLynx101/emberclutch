# Sets up headless autotests on this PC (docs/tech/headless-emulator.md): a WSL distro of
# their own (emberclutch-test, Ubuntu 24.04, so your everyday distro is never touched), then
# Xvfb, Mesa and the pinned Azahar inside it (tools/wsl/setup.sh). Safe to run again.
#   tools\wsl\install.ps1 [-Remove]
param([switch]$Remove)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$distro = "emberclutch-test"
$location = Join-Path $env:LOCALAPPDATA "wsl\$distro"

$distros = (wsl.exe -l -q) -replace "`0", ""  # wsl.exe answers in UTF-16
if ($Remove) {
    if ($distros -contains $distro) { wsl.exe --unregister $distro }
    "removed $distro"
    return
}
if (-not ($distros -contains $distro)) {
    # --no-launch skips the first-run user prompt: everything here runs as root.
    wsl.exe --install Ubuntu-24.04 --name $distro --no-launch --location $location
    if ($LASTEXITCODE -ne 0) { throw "wsl --install failed ($LASTEXITCODE)" }
}
$full = (Resolve-Path (Join-Path $PSScriptRoot "setup.sh")).Path
$setup = "/mnt/" + $full.Substring(0, 1).ToLower() + ($full.Substring(2) -replace "\\", "/")
wsl.exe -d $distro -u root -- bash $setup
if ($LASTEXITCODE -ne 0) { throw "setup.sh failed ($LASTEXITCODE)" }
