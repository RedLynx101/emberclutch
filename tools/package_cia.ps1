# Packs emberclutch.cia, to install on a 3DS with FBI (Luma3DS): the game with its romfs,
# the icon (assets/icon.png) and the HOME Menu banner (assets/banner.png with
# assets/audio/banner.wav). Settings: tools/cia.rsf.
#   tools\package_cia.ps1 [-NoBuild] [-ToolsDir <folder with makerom\ and bannertool\>]
# makerom (3DSGuy/Project_CTR) and bannertool (diasurgical/bannertool) aren't kept in this
# repo: by default they're taken from the 3D-Claw project next to it (3ds-ai\tools\win64),
# else from PATH.
param(
    [switch]$NoBuild,
    [string]$ToolsDir = "",
    [string]$Version = "0.1.0"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $ToolsDir) { $ToolsDir = Join-Path (Split-Path $root -Parent) "3ds-ai\tools\win64" }

function Find-Tool($relative, $name) {
    $local = Join-Path $ToolsDir $relative
    if (Test-Path $local) { return $local }
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "$name not found in $ToolsDir or on PATH"
}
$makerom = Find-Tool "makerom\makerom.exe" "makerom"
$bannertool = Find-Tool "bannertool\windows-x86_64\bannertool.exe" "bannertool"

if (-not $NoBuild) { & (Join-Path $PSScriptRoot "build.ps1") }
$elf = Join-Path $root "emberclutch.elf"
if (-not (Test-Path $elf)) { throw "Missing $elf (build first)" }
foreach ($f in "assets\icon.png", "assets\banner.png", "assets\audio\banner.wav") {
    if (-not (Test-Path (Join-Path $root $f))) { throw "Missing $f" }
}

$work = Join-Path $root "build\cia"
New-Item -ItemType Directory -Force $work | Out-Null
$smdh = Join-Path $work "emberclutch.smdh"
$bnr = Join-Path $work "emberclutch.bnr"
$cia = Join-Path $root "emberclutch.cia"

& $bannertool makesmdh -s "Emberclutch" -l "Emberclutch: raise, breed and fly with dragons" -p "Noah Hicks" `
    -i (Join-Path $root "assets\icon.png") -o $smdh
if ($LASTEXITCODE -ne 0) { throw "bannertool makesmdh failed ($LASTEXITCODE)" }
& $bannertool makebanner -i (Join-Path $root "assets\banner.png") -a (Join-Path $root "assets\audio\banner.wav") -o $bnr
if ($LASTEXITCODE -ne 0) { throw "bannertool makebanner failed ($LASTEXITCODE)" }

$v = $Version.Split(".")
Push-Location $root  # the RSF's RomFs path is relative to here
try {
    & $makerom -f cia -o $cia -elf $elf -icon $smdh -banner $bnr -rsf (Join-Path $PSScriptRoot "cia.rsf") `
        -target t -DAPP_ENCRYPTED=false -major $v[0] -minor $v[1] -micro $v[2]
    if ($LASTEXITCODE -ne 0) { throw "makerom failed ($LASTEXITCODE)" }
} finally {
    Pop-Location
}
$size = [math]::Round((Get-Item $cia).Length / 1MB, 1)
"Built $cia ($size MB, version $Version)"
