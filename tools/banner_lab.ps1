# Banner lab: small test titles for trying HOME Menu banners on the 3DS without touching the
# game's own title (the HOME Menu caches a title's banner, and a banner that freezes it costs a
# restart). Each variant is its own title ("Banner lab A", "B", ...; unique IDs 0xEC0D1 up),
# holding the game's code but no romfs: select them on the HOME Menu, never start them.
#   tools\banner_lab.ps1 -Variants "A=<cgfx>;<wav>", "B=..." [-FirstId 0xEC0D1] [-Deploy <3ds-ip>]
# Each round takes fresh IDs (-FirstId): the HOME Menu may keep a deleted title's banner.
# A variant's banner is a CGFX (3D) or a PNG (flat) and a WAV. The CIAs land in build/lab/;
# -Deploy uploads them to sdmc:/cias/lab/ (FBI can install a whole folder at once). Remove
# them afterwards in FBI: Titles, "Banner lab ...", Delete Title.
param(
    [Parameter(Mandatory = $true)][string[]]$Variants,
    [string]$Deploy = "",
    [int]$FirstId = 0xEC0D1,
    [string]$ToolsDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $ToolsDir) { $ToolsDir = Join-Path (Split-Path $root -Parent) "3ds-ai\tools\win64" }
$makerom = Join-Path $ToolsDir "makerom\makerom.exe"
$bannertool = Join-Path $ToolsDir "bannertool\windows-x86_64\bannertool.exe"
$elf = Join-Path $root "emberclutch.elf"
$lab = Join-Path $root "build\lab"
New-Item -ItemType Directory -Force $lab | Out-Null

# The game's RSF without its romfs, a title and ID per variant.
$rsf = Join-Path $lab "lab.rsf"
$text = Get-Content (Join-Path $PSScriptRoot "cia.rsf") -Raw
$text = $text -replace '(?ms)^RomFs:\r?\n  RootPath[^\n]*\n', ''
$text = $text -replace 'Title                   : "Emberclutch"', 'Title                   : "$(LAB_TITLE)"'
$text = $text -replace 'ProductCode             : "CTR-P-EMBC"', 'ProductCode             : "CTR-P-EMBL"'
$text = $text -replace 'UniqueId(\s*): 0x[0-9A-Fa-f]+', 'UniqueId$1: $(LAB_ID)'
$text = $text -replace 'JumpId(\s*): 0x[0-9A-Fa-f]+', 'JumpId$1: $(LAB_JUMP)'
Set-Content -Path $rsf -Value $text -Encoding ascii -NoNewline

$id = $FirstId
$built = @()
foreach ($v in $Variants) {
    $name, $spec = $v -split "=", 2
    $banner, $wav = $spec -split ";", 2
    $title = "Banner lab $name"
    $smdh = Join-Path $lab "$name.smdh"
    $bnr = Join-Path $lab "$name.bnr"
    & $bannertool makesmdh -s $title -l "Emberclutch banner test $name (don't start it)" -p "Noah Hicks" `
        -i (Join-Path $root "assets\icon.png") -f "visible,allow3d,recordusage,extendedbanner" -o $smdh | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "makesmdh failed for $name" }
    $kind = if ($banner -like "*.png") { "-i" } else { "-ci" }
    & $bannertool makebanner $kind $banner -a $wav -o $bnr | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "makebanner failed for $name" }
    $cia = Join-Path $lab ("banner-lab-{0}.cia" -f $name.ToLower())
    & $makerom -f cia -o $cia -elf $elf -icon $smdh -banner $bnr -rsf $rsf -target t -DAPP_ENCRYPTED=false `
        ("-DLAB_TITLE=$title") ("-DLAB_ID=0x{0:X}" -f $id) ("-DLAB_JUMP=0x00040000{0:X6}00" -f $id) -major 0 -minor 0 -micro 1
    if ($LASTEXITCODE -ne 0) { throw "makerom failed for $name" }
    $built += Get-Item $cia
    "{0}: {1} ({2:N1} MB, title 00040000{3:X6}00) <- {4} + {5}" -f $title, (Split-Path $cia -Leaf), ((Get-Item $cia).Length / 1MB), $id, (Split-Path $banner -Leaf), (Split-Path $wav -Leaf)
    $id++
}

if ($Deploy) {
    foreach ($c in $built) {
        $uri = "ftp://${Deploy}:5000/cias/lab/$($c.Name)"
        try {
            $mk = [System.Net.FtpWebRequest]::Create("ftp://${Deploy}:5000/cias/lab")
            $mk.Method = [System.Net.WebRequestMethods+Ftp]::MakeDirectory
            $mk.Credentials = New-Object System.Net.NetworkCredential("anonymous", "")
            $mk.GetResponse().Close()
        } catch { }
        $req = [System.Net.FtpWebRequest]::Create($uri)
        $req.Method = [System.Net.WebRequestMethods+Ftp]::UploadFile
        $req.Credentials = New-Object System.Net.NetworkCredential("anonymous", "")
        $req.UseBinary = $true; $req.UsePassive = $true; $req.KeepAlive = $false; $req.Timeout = 120000
        $bytes = [System.IO.File]::ReadAllBytes($c.FullName)
        $s = $req.GetRequestStream(); $s.Write($bytes, 0, $bytes.Length); $s.Close()
        $req.GetResponse().Close()
        "uploaded /cias/lab/$($c.Name)"
    }
}
