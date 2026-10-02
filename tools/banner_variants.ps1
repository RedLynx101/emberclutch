# Run 25 (Noah: "Make a more highly detailed banner, cleaner, and use one of our baby dragons still
# in the game. Then variants of it for a banner test."): builds banner variants side by side, each
# a kit kind's hatchling (full detail, its 256 skin) in its egg, into build/banner_v/<name>/
# (banner.gltf, banner.cgfx, the review renders and its flat banner.png; assets/ is never touched).
#   tools\banner_variants.ps1 -Variants "A=pouncer:0:0.012", "B=pouncer:0:0", "C=blazeplume:0:0.012"
# Each is <name>=<kind>:<colouring>:<outline width, of the dragon's height; 0 none>. Then try them on
# the 3DS's HOME Menu as banner-lab titles (tools\banner_lab.ps1, fresh IDs: docs/tech/banner-labs.md).
param([Parameter(Mandatory = $true)][string[]]$Variants)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$blender = "C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe"

foreach ($v in $Variants) {
    $name, $spec = $v -split "=", 2
    $kind, $colour, $ink = $spec -split ":"
    $out = Join-Path $root "build\banner_v\$name"
    New-Item -ItemType Directory -Force $out | Out-Null
    $ErrorActionPreference = "Continue"  # (Blender's deprecation warnings go to stderr)
    & $blender -b -P (Join-Path $root "tools\blender\banner3d.py") -- --kind $kind --variant $colour --outline $ink --turn `
        --out $out --review $out --assets $out 2>&1 | ForEach-Object { "$_" } | Select-String "\[banner\] (dragon|5 dragon|names)|Error|Traceback" | ForEach-Object { "  $name $_" }
    $ErrorActionPreference = "Stop"
    $gltf = Join-Path $out "banner.gltf"
    $cgfx = Join-Path $out "banner.cgfx"
    & py -3.12 (Join-Path $root "tools\banner_cgfx.py") $gltf $cgfx --turn "body*:1,egg:1" | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "$name`: banner_cgfx.py failed" }
    & py -3.12 (Join-Path $root "tools\check_3ds.py") --quiet $gltf $cgfx
    if ($LASTEXITCODE -ne 0) { throw "$name`: failed its checks (tools\check_3ds.py)" }
    $kb = [math]::Round((Get-Item $cgfx).Length / 1KB)
    # (run 25: the labs at 502 and 509 KB froze the HOME Menu, every one at 494 KB or less held)
    $ok = if ((Get-Item $cgfx).Length -le 480KB) { "ok" } elseif ((Get-Item $cgfx).Length -le 512KB) { "TOO BIG for the HOME Menu (over 480 KB)" } else { "TOO BIG" }
    "$name ($kind, colouring $colour, outline $ink): $kb KB (keep under 480), $ok"
}
