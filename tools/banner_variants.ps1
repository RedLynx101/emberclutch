# Run 25 (Noah: "Make a more highly detailed banner, cleaner, and use one of our baby dragons still
# in the game. Then variants of it for a banner test."): builds banner variants side by side, each
# a kit kind's hatchling (full detail, its 256 skin) in its egg, into build/banner_v/<name>/
# (banner.gltf, banner.cgfx, the review renders and its flat banner.png; assets/ is never touched).
#   tools\banner_variants.ps1 -Variants "A=pouncer:0:0.012", "B=pouncer:0:0", "C=blazeplume:0:0.012"
# Each is <name>=<kind>:<colouring>:<outline width, of the dragon's height; 0 none>[:still] (still: the
# head and tail joined into the body, nothing at the joints to part). Then try them on
# the 3DS's HOME Menu as banner-lab titles (tools\banner_lab.ps1, fresh IDs: docs/tech/banner-labs.md).
param([Parameter(Mandatory = $true)][string[]]$Variants)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$blender = "C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe"

foreach ($v in $Variants) {
    $name, $spec = $v -split "=", 2
    $kind, $colour, $ink, $still = $spec -split ":"
    $extra = @()
    if ($still -eq "still") { $extra += "--still-head" }  # (run 26: the head and tail joined, no joints)
    $out = Join-Path $root "build\banner_v\$name"
    New-Item -ItemType Directory -Force $out | Out-Null
    $ErrorActionPreference = "Continue"  # (Blender's deprecation warnings go to stderr)
    & $blender -b -P (Join-Path $root "tools\blender\banner3d.py") -- --kind $kind --variant $colour --outline $ink --turn $extra `
        --out $out --review $out --assets $out 2>&1 | ForEach-Object { "$_" } | Select-String "\[banner\] (dragon|5 dragon|names)|Error|Traceback" | ForEach-Object { "  $name $_" }
    $ErrorActionPreference = "Stop"
    $gltf = Join-Path $out "banner.gltf"
    $cgfx = Join-Path $out "banner.cgfx"
    & py -3.12 (Join-Path $root "tools\banner_cgfx.py") $gltf $cgfx --turn "body*:1,egg:1" | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "$name`: banner_cgfx.py failed" }
    & py -3.12 (Join-Path $root "tools\check_3ds.py") --quiet $gltf $cgfx
    if ($LASTEXITCODE -ne 0) { throw "$name`: failed its checks (tools\check_3ds.py)" }
    $kb = [math]::Round((Get-Item $cgfx).Length / 1KB)
    # (512 KB is the HOME Menu's limit; under 500 for a margin. The freezes of runs 25-27 were the textures'
    # alignment, not the size: tools/banner_cgfx.py puts them on 128-byte boundaries now)
    $ok = if ((Get-Item $cgfx).Length -le 500KB) { "ok" } else { "TOO BIG (keep under 500 KB)" }
    "$name ($kind, colouring $colour, outline $ink$(if ($still) { ', still head' })): $kb KB, $ok"
}
