# Uploads the build to the 3DS over Wi-Fi using ftpd (default port 5000), each file's size
# checked on the 3DS afterwards:
#   sdmc:/3ds/emberclutch/emberclutch.3dsx  (+ .smdh)   run it from the Homebrew Launcher
#   sdmc:/cias/<name>.cia                               with -Cia: every CIA in build/cia-test/
#                                                       (tools\package_cia.ps1 builds them), for FBI
# The 3DS's address changes now and then (DHCP): ftpd shows it on its screen.
param(
    [Parameter(Mandatory = $true)][string]$FtpHost,
    [int]$Port = 5000,
    [switch]$Cia
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

New-RemoteDir "/3ds/emberclutch"
Send-File (Join-Path $root "emberclutch.3dsx") "/3ds/emberclutch/emberclutch.3dsx"
Send-File (Join-Path $root "emberclutch.smdh") "/3ds/emberclutch/emberclutch.smdh"
if ($Cia) {
    New-RemoteDir "/cias"
    $cias = @(Get-ChildItem (Join-Path $root "build\cia-test") -Filter *.cia -ErrorAction SilentlyContinue)
    if (-not $cias.Count) { throw "No CIAs in build\cia-test (tools\package_cia.ps1 builds them)" }
    foreach ($c in $cias) { Send-File $c.FullName "/cias/$($c.Name)" }
}
