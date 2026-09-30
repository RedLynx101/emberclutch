# Packs emberclutch.cia, to install on a 3DS with FBI (Luma3DS): the game with its romfs,
# the icon (assets/icon.png) and the HOME Menu banner (assets/banner.png with
# assets/audio/banner.wav). Settings: tools/cia.rsf.
#   tools\package_cia.ps1 [-NoBuild] [-Banner2D] [-ToolsDir <folder with makerom\ and bannertool\>]
# The animated 3D banner (build/banner/banner.cgfx, from tools\make_banner.ps1) is the default
# since it showed on Noah's old 3DS (banner lab, 2026-09-24; D58, D63); -Banner2D packs the
# flat one instead. (-Banner3D is still accepted: it's the default.) The emulator can't show
# HOME Menu banners.
# A 3D banner needs the SMDH's "extendedbanner" flag, as homebrew 3D banners have: with it the
# HOME Menu showed ours (0.1.1); without it, it froze opening the new title's present (0.1.2,
# 2026-09-24). The flat banner keeps bannertool's default flags.
# The boot logo is makerom's homebrew logo (the RSF's "Logo: Homebrew"): a logo of our own
# can't be signed (its HMAC key is Nintendo's), and 0.1.8's stopped the game at start (run 9).
# The CIA is checked by tools\check_3ds.py once it's built; a failed check stops here.
# makerom (3DSGuy/Project_CTR) and bannertool (diasurgical/bannertool) aren't kept in this
# repo: by default they're taken from the 3D-Claw project next to it (3ds-ai\tools\win64),
# else from PATH.
param(
    [switch]$NoBuild,
    [switch]$Banner3D,
    [switch]$Banner2D,
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

$smdhArgs = @("-s", "Emberclutch: Skyreach Valley", "-l", "Emberclutch: Skyreach Valley. Raise, breed and fly with dragons", "-p", "Noah Hicks",
    "-i", (Join-Path $root "assets\icon.png"), "-o", $smdh)
$bannerArgs = @("-i", (Join-Path $root "assets\banner.png"))
$Banner3D = -not $Banner2D
if ($Banner3D) {
    $cgfx = Join-Path $root "build\banner\banner.cgfx"
    if (-not (Test-Path $cgfx)) { & (Join-Path $PSScriptRoot "make_banner.ps1") -SkipIcon }
    if ((Get-Item $cgfx).Length -gt 512KB) { throw "banner.cgfx is over 512 KB" }
    $bannerArgs = @("-ci", $cgfx)
    $smdhArgs += @("-f", "visible,allow3d,recordusage,extendedbanner")
}
& $bannertool makesmdh @smdhArgs
if ($LASTEXITCODE -ne 0) { throw "bannertool makesmdh failed ($LASTEXITCODE)" }
& $bannertool makebanner @bannerArgs -a (Join-Path $root "assets\audio\banner.wav") -o $bnr
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
"Built $cia ($size MB, version $Version, $(if ($Banner3D) { '3D' } else { '2D' }) banner)"
& py -3.12 (Join-Path $PSScriptRoot "check_3ds.py") --quiet $cia
if ($LASTEXITCODE -ne 0) { throw "$cia failed its checks (tools\check_3ds.py): don't put it on the 3DS" }
