# The concept images' small JPGs for the review pages (make_concepts.py writes the full PNGs):
#   tools\concept\to_jpg.ps1 -Names dragon_pouncer,dragon_adults -OutDir docs\art\concept\dragons
# Each <SrcDir>\<name>.png (default build\concept) becomes <OutDir>\<name>.jpg, 960 px wide, quality 85.
param(
    [Parameter(Mandatory = $true)][string[]]$Names,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [string]$SrcDir = "build\concept",
    [int]$Width = 960,
    [int]$Quality = 85
)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$out = Join-Path $root $OutDir
New-Item -ItemType Directory -Force $out | Out-Null
$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$params = New-Object System.Drawing.Imaging.EncoderParameters(1)
$params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]$Quality)
foreach ($name in ($Names -split ",")) {
    $src = [System.Drawing.Image]::FromFile((Join-Path $root "$SrcDir\$name.png"))
    $h = [int]($src.Height * $Width / $src.Width)
    $dst = New-Object System.Drawing.Bitmap($Width, $h)
    $g = [System.Drawing.Graphics]::FromImage($dst)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($src, 0, 0, $Width, $h)
    $file = Join-Path $out "$name.jpg"
    $dst.Save($file, $codec, $params)
    $g.Dispose(); $dst.Dispose(); $src.Dispose()
    Write-Host ("{0} ({1:N0} KB)" -f $file, ((Get-Item $file).Length / 1KB))
}
