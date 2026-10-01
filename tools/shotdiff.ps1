# Compares an autotest run's pictures with a baseline, so only the ones that changed need a
# look (docs/tech/headless-emulator.md).
#   tools\shotdiff.ps1 tour -Save          build/autotest/tour/ becomes the baseline
#   tools\shotdiff.ps1 tour                compare the latest run with it
#   [-Threshold 24] [-MaxPercent 0.1]      a pixel counts as changed when a channel moves more
#                                          than Threshold; a picture when over MaxPercent of it does
# Baselines live in build/autotest-baseline/<script>/ (local, not committed). A changed picture
# gets ref | new | diff (changed pixels red) in build/autotest/<script>/diff/. Exit code 1 when
# anything changed, appeared or went missing.
param(
    [Parameter(Mandatory = $true)][string]$Name,
    [switch]$Save,
    [int]$Threshold = 24,
    [double]$MaxPercent = 0.1
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$run = Join-Path $root "build\autotest\$Name"
$base = Join-Path $root "build\autotest-baseline\$Name"
if (-not (Test-Path $run)) { throw "No run in $run" }
$shots = { param($dir) Get-ChildItem $dir -Filter *.png | Where-Object { $_.Name -notlike "sheet-*" } }

if ($Save) {
    Remove-Item $base -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force $base | Out-Null
    & $shots $run | Copy-Item -Destination $base
    "baseline: $(@(& $shots $base).Count) pictures in $base"
    return
}
if (-not (Test-Path $base)) { throw "No baseline for $Name. Make one: tools\shotdiff.ps1 $Name -Save" }

Add-Type -AssemblyName System.Drawing
# Pixel loops in PowerShell take seconds a picture; this does them in C#.
if (-not ("ShotDiff" -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System; using System.Drawing; using System.Drawing.Imaging; using System.Runtime.InteropServices;
public static class ShotDiff {
    static byte[] Bytes(Bitmap b, out int stride) {
        var d = b.LockBits(new Rectangle(0, 0, b.Width, b.Height), ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
        stride = d.Stride; var a = new byte[d.Stride * b.Height];
        Marshal.Copy(d.Scan0, a, 0, a.Length); b.UnlockBits(d); return a;
    }
    // Changed pixels; with diffPath set, writes ref | new | diff side by side.
    public static int Compare(string refPath, string newPath, int threshold, string diffPath) {
        using (var r = new Bitmap(refPath)) using (var n = new Bitmap(newPath)) {
            if (r.Width != n.Width || r.Height != n.Height) return r.Width * r.Height;
            int sr, sn; var a = Bytes(r, out sr); var b = Bytes(n, out sn);
            int w = r.Width, h = r.Height, changed = 0;
            var mask = new bool[w * h];
            for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
                int i = y * sr + x * 3, j = y * sn + x * 3;
                if (Math.Abs(a[i] - b[j]) > threshold || Math.Abs(a[i + 1] - b[j + 1]) > threshold ||
                    Math.Abs(a[i + 2] - b[j + 2]) > threshold) { mask[y * w + x] = true; changed++; }
            }
            if (!String.IsNullOrEmpty(diffPath) && changed > 0) {  // PowerShell's null arrives as ""
                using (var o = new Bitmap(w * 3 + 8, h)) using (var g = Graphics.FromImage(o)) {
                    g.Clear(Color.FromArgb(24, 24, 28));
                    g.DrawImage(r, 0, 0, w, h); g.DrawImage(n, w + 4, 0, w, h);
                    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
                        int j = y * sn + x * 3;
                        int grey = (b[j] + b[j + 1] + b[j + 2]) / 9;  // the new picture, dimmed
                        o.SetPixel(2 * w + 8 + x, y, mask[y * w + x] ? Color.Red : Color.FromArgb(grey, grey, grey));
                    }
                    o.Save(diffPath, ImageFormat.Png);
                }
            }
            return changed;
        }
    }
}
"@
}

$diffDir = Join-Path $run "diff"
Remove-Item $diffDir -Recurse -Force -ErrorAction SilentlyContinue
$refs = @{}; & $shots $base | ForEach-Object { $refs[$_.Name] = $_.FullName }
$news = @{}; & $shots $run | ForEach-Object { $news[$_.Name] = $_.FullName }
$changed = @(); $same = 0
foreach ($k in ($refs.Keys | Sort-Object)) {
    if (-not $news.ContainsKey($k)) { $changed += "MISSING  $k"; continue }
    $img = [System.Drawing.Image]::FromFile($refs[$k]); $total = $img.Width * $img.Height; $img.Dispose()
    $n = [ShotDiff]::Compare($refs[$k], $news[$k], $Threshold, $null)
    $pct = 100.0 * $n / $total
    if ($pct -gt $MaxPercent) {
        New-Item -ItemType Directory -Force $diffDir | Out-Null
        [void][ShotDiff]::Compare($refs[$k], $news[$k], $Threshold, (Join-Path $diffDir $k))
        $changed += ("CHANGED  {0}  {1:N2}% of pixels" -f $k, $pct)
    } else { $same++ }
}
foreach ($k in ($news.Keys | Sort-Object)) { if (-not $refs.ContainsKey($k)) { $changed += "NEW      $k" } }
"$same unchanged, $($changed.Count) to look at (threshold $Threshold, over $MaxPercent%)"
$changed
if ($changed.Count) { "diff pictures: $diffDir"; exit 1 }
