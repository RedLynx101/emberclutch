# The GitHub release (1.0, R4: docs/plan/release-1.0.md), on Noah's word only. Two steps:
#   tools\release\release.ps1            tags v<VERSION> on main and creates the release as a DRAFT with its files
#   tools\release\release.ps1 -Publish   publishes that draft, then checks FBI's QR link fetches the CIA
# The files are the player build that was tested on the 3DS (dist\player\: tools\package_cia.ps1 -Player)
# and the guide's PDF (docs\guide\), the notes docs\release\notes-v<VERSION>.md. It stops, sending
# nothing, if the repo is still private (the QR link and the notes' pictures need it public), the version
# doesn't match the Makefile's, main isn't pushed, or a file fails its checks (tools\check_3ds.py).
param(
    [switch]$Publish
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$repo = "RedLynx101/emberclutch"

$version = (Select-String -Path (Join-Path $root "Makefile") -Pattern "^VERSION\s*:=\s*(\S+)").Matches[0].Groups[1].Value
$tag = "v$version"
$notes = Join-Path $root "docs\release\notes-$tag.md"
$files = @(
    (Join-Path $root "dist\player\emberclutch.cia"),
    (Join-Path $root "dist\player\emberclutch.3dsx"),
    (Join-Path $root "dist\player\emberclutch-$version.zip"),
    (Join-Path $root "docs\guide\Emberclutch-Guide.pdf")
)

$visibility = (gh repo view $repo --json visibility --jq .visibility)
if ($LASTEXITCODE -ne 0) { throw "gh couldn't read $repo (gh auth status)" }
if ($visibility -ne "PUBLIC") { throw "$repo is ${visibility}: make it public first (the QR link and the notes' pictures need it)" }

if ($Publish) {
    gh release edit $tag --repo $repo --draft=false
    if ($LASTEXITCODE -ne 0) { throw "couldn't publish $tag" }
    "Published: https://github.com/$repo/releases/tag/$tag"
    # FBI's QR code points at the latest release's CIA: it should now answer with the file.
    $url = "https://github.com/$repo/releases/latest/download/emberclutch.cia"
    $size = (Get-Item $files[0]).Length
    $resp = Invoke-WebRequest -Uri $url -Method Head -MaximumRedirection 5 -UseBasicParsing
    $length = [int64]$resp.Headers["Content-Length"]
    "QR link: HTTP $($resp.StatusCode), $length bytes (the CIA here: $size)"
    if ($resp.StatusCode -ne 200 -or $length -ne $size) { throw "the QR link doesn't fetch the release's CIA yet" }
    return
}

foreach ($f in @($notes) + $files) { if (-not (Test-Path $f)) { throw "Missing $f" } }
& py -3.12 (Join-Path $root "tools\check_3ds.py") --quiet $files[0] $files[1]
if ($LASTEXITCODE -ne 0) { throw "a file failed its checks (tools\check_3ds.py): nothing was sent" }
& git -C $root fetch -q origin main
$ahead = (& git -C $root rev-list --count origin/main..main)
if ($ahead -ne "0") { throw "main is $ahead commit(s) ahead of origin: push first" }
foreach ($f in $files) { "{0}  {1:N1} MB  sha256 {2}" -f (Split-Path $f -Leaf), ((Get-Item $f).Length / 1MB), (Get-FileHash $f -Algorithm SHA256).Hash.ToLower() }

gh release create $tag @files --repo $repo --target main --draft --title "Emberclutch: Skyreach Valley $version" --notes-file $notes
if ($LASTEXITCODE -ne 0) { throw "gh release create failed" }
"Draft created: look it over at https://github.com/$repo/releases, then tools\release\release.ps1 -Publish"
