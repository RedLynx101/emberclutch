# The app icon and the HOME Menu banners (Alpha 2 WP10, D48, D50): the emblem icon
# (assets/icon.png), the flat 2D banner (assets/banner.png) and the animated 3D banner
# (build/banner/banner.cgfx), built by Blender and converted by pycgfx.
#   tools\make_banner.ps1 [-SkipIcon] [-SkipBanner]
# pycgfx is a build tool, never committed: git clone --depth 1 https://github.com/skyfloogle/pycgfx build\tools\pycgfx
# with gltflib and pillow for Python 3.12 (py -3.12 -m pip install gltflib pillow). Approved for this step (D50).
param(
    [switch]$SkipIcon,
    [switch]$SkipBanner
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$blender = "C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe"
$pycgfx = Join-Path $root "build\tools\pycgfx\main.py"

if (-not $SkipIcon) {
    & $blender -b -P (Join-Path $root "tools\blender\emblem.py") | Select-String "\[emblem\]"
    if ($LASTEXITCODE -ne 0) { throw "emblem.py failed" }
}
if (-not $SkipBanner) {
    & $blender -b -P (Join-Path $root "tools\blender\banner3d.py") | Select-String "\[banner\]"
    if ($LASTEXITCODE -ne 0) { throw "banner3d.py failed" }
    if (-not (Test-Path $pycgfx)) { throw "pycgfx missing: git clone --depth 1 https://github.com/skyfloogle/pycgfx build\tools\pycgfx" }
    $gltf = Join-Path $root "build\banner\banner.gltf"
    $cgfx = Join-Path $root "build\banner\banner.cgfx"
    & py -3.12 (Join-Path $root "tools\banner_cgfx.py") $gltf $cgfx  # pycgfx, plus unlit materials
    if ($LASTEXITCODE -ne 0) { throw "banner_cgfx.py (pycgfx) failed" }
    & py -3.12 (Join-Path $root "tools\check_3ds.py") --quiet $gltf $cgfx
    if ($LASTEXITCODE -ne 0) { throw "the banner failed its checks (tools\check_3ds.py)" }
    $kb = [math]::Round((Get-Item $cgfx).Length / 1KB)
    if ((Get-Item $cgfx).Length -gt 512KB) { throw "banner.cgfx is $kb KB: the HOME Menu takes at most 512 KB" }
    "3D banner: $cgfx ($kb KB of 512)"
}
