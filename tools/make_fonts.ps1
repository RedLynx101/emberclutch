# Converts the interface fonts (D33) into romfs/fonts with devkitPro's mkbcfnt:
#   ui.bcfnt     Nunito SemiBold, ASCII + Latin-1 (+ a few punctuation marks), for everything
#   title.bcfnt  Cinzel Decorative Bold, ASCII, for the wordmark and headings
# Both are subsets, which the SIL OFL counts as modified versions: Cinzel's licence reserves
# its name, so the converted files carry neutral names. Sources and licences: assets/fonts/.
# The sizes give line heights close to the system font's; ui_draw.cpp scales them to match
# exactly at run time.

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$mkbcfnt = "C:\msys64\opt\devkitpro\tools\bin\mkbcfnt.exe"
if (-not (Test-Path -LiteralPath $mkbcfnt)) { throw "mkbcfnt not found at $mkbcfnt (devkitPro tools)" }

$fonts = Join-Path $root "assets\fonts"
$out = Join-Path $root "romfs\fonts"
New-Item -ItemType Directory -Force $out | Out-Null

& $mkbcfnt -s 17 -w (Join-Path $fonts "latin.txt") -o (Join-Path $out "ui.bcfnt") (Join-Path $fonts "nunito\Nunito-SemiBold.ttf")
if ($LASTEXITCODE -ne 0) { throw "mkbcfnt failed on Nunito" }
& $mkbcfnt -s 24 -w (Join-Path $fonts "ascii.txt") -o (Join-Path $out "title.bcfnt") (Join-Path $fonts "cinzel-decorative\CinzelDecorative-Bold.ttf")
if ($LASTEXITCODE -ne 0) { throw "mkbcfnt failed on Cinzel Decorative" }
Get-ChildItem $out -Filter *.bcfnt | Select-Object Name, Length
