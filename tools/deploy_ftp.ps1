# Uploads the build to the 3DS over Wi-Fi using ftpd (default port 5000), each file's size
# checked on the 3DS afterwards:
#   sdmc:/3ds/emberclutch/emberclutch.3dsx  (+ .smdh)   run it from the Homebrew Launcher
#   sdmc:/cias/<name>.cia                               with -Cia: every CIA in build/cia-test/
#                                                       (tools\package_cia.ps1 builds them), for FBI
# -Player sends the player build in their place (the release's own files: emberclutch-player.3dsx
# as emberclutch.3dsx, and dist\player\emberclutch.cia; tools\package_cia.ps1 -Player builds them).
# The 3DS's address changes now and then (DHCP): ftpd shows it on its screen.
# Everything is checked first (tools\check_3ds.py): if any file fails, nothing is sent.
param(
    [Parameter(Mandatory = $true)][string]$FtpHost,
    [int]$Port = 5000,
    [switch]$Cia,
    [switch]$Player
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function New-Request([string]$remote, [string]$method) {
    $req = [System.Net.FtpWebRequest]::Create("ftp://${FtpHost}:${Port}${remote}")
    $req.Method = $method
    $req.Credentials = New-Object System.Net.NetworkCredential("anonymous", "")
    $req.UseBinary = $true
    $req.UsePassive = $true
    $req.KeepAlive = $false
    $req.Timeout = 180000  # 20 MB takes 20-40 s
    $req.ReadWriteTimeout = 180000
    return $req
}

function Send-File([string]$local, [string]$remote) {
    if (-not (Test-Path $local)) { throw "Missing $local" }
    $req = New-Request $remote ([System.Net.WebRequestMethods+Ftp]::UploadFile)
    $bytes = [System.IO.File]::ReadAllBytes($local)
    $req.ContentLength = $bytes.Length
    $stream = $req.GetRequestStream()
    $stream.Write($bytes, 0, $bytes.Length)
    $stream.Close()
    $req.GetResponse().Close()
    $check = (New-Request $remote ([System.Net.WebRequestMethods+Ftp]::GetFileSize)).GetResponse()
    $size = $check.ContentLength
    $check.Close()
    if ($size -ne $bytes.Length) { throw "$remote is $size bytes on the 3DS, $($bytes.Length) here" }
    Write-Host ("{0} -> {1} ({2:N1} MB, checked)" -f (Split-Path $local -Leaf), $remote, ($bytes.Length / 1MB))
}

function New-RemoteDir([string]$remote) {
    try {
        (New-Request $remote ([System.Net.WebRequestMethods+Ftp]::MakeDirectory)).GetResponse().Close()
    } catch { }  # already exists
}

$name = if ($Player) { "emberclutch-player" } else { "emberclutch" }
$cias = @()
if ($Player) {
    $cias = @(Get-Item (Join-Path $root "dist\player\emberclutch.cia"))
} elseif ($Cia) {
    $cias = @(Get-ChildItem (Join-Path $root "build\cia-test") -Filter *.cia -ErrorAction SilentlyContinue)
    if (-not $cias.Count) { throw "No CIAs in build\cia-test (tools\package_cia.ps1 builds them)" }
}
$files = @((Join-Path $root "$name.3dsx"), (Join-Path $root "$name.smdh")) + @($cias | ForEach-Object { $_.FullName })
& py -3.12 (Join-Path $PSScriptRoot "check_3ds.py") --quiet @files
if ($LASTEXITCODE -ne 0) { throw "A file failed its checks (tools\check_3ds.py): nothing was sent" }

New-RemoteDir "/3ds/emberclutch"
Send-File (Join-Path $root "$name.3dsx") "/3ds/emberclutch/emberclutch.3dsx"
Send-File (Join-Path $root "$name.smdh") "/3ds/emberclutch/emberclutch.smdh"
if ($cias.Count) {
    New-RemoteDir "/cias"
    foreach ($c in $cias) { Send-File $c.FullName "/cias/$($c.Name)" }
}
