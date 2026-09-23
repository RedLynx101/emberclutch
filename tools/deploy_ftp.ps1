# Uploads the build to the 3DS over Wi-Fi using ftpd (default port 5000).
#   sdmc:/3ds/emberclutch/emberclutch.3dsx  (+ .smdh)
#   sdmc:/cias/emberclutch.cia              (with -Cia, once CIA packaging exists)
param(
    [Parameter(Mandatory = $true)][string]$FtpHost,
    [int]$Port = 5000,
    [switch]$Cia
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Send-File([string]$local, [string]$remote) {
    if (-not (Test-Path $local)) { throw "Missing $local" }
    $uri = "ftp://${FtpHost}:${Port}${remote}"
    $req = [System.Net.FtpWebRequest]::Create($uri)
    $req.Method = [System.Net.WebRequestMethods+Ftp]::UploadFile
    $req.Credentials = New-Object System.Net.NetworkCredential("anonymous", "")
    $req.UseBinary = $true
    $req.UsePassive = $true
    $req.KeepAlive = $false
    $bytes = [System.IO.File]::ReadAllBytes($local)
    $req.ContentLength = $bytes.Length
    $stream = $req.GetRequestStream()
    $stream.Write($bytes, 0, $bytes.Length)
    $stream.Close()
    $resp = $req.GetResponse()
    Write-Host ("{0} -> {1} ({2})" -f (Split-Path $local -Leaf), $remote, $resp.StatusDescription.Trim())
    $resp.Close()
}

function New-RemoteDir([string]$remote) {
    try {
        $req = [System.Net.FtpWebRequest]::Create("ftp://${FtpHost}:${Port}${remote}")
        $req.Method = [System.Net.WebRequestMethods+Ftp]::MakeDirectory
        $req.Credentials = New-Object System.Net.NetworkCredential("anonymous", "")
        $req.GetResponse().Close()
    } catch { }  # already exists
}

New-RemoteDir "/3ds/emberclutch"
Send-File (Join-Path $root "emberclutch.3dsx") "/3ds/emberclutch/emberclutch.3dsx"
Send-File (Join-Path $root "emberclutch.smdh") "/3ds/emberclutch/emberclutch.smdh"
if ($Cia) { Send-File (Join-Path $root "emberclutch.cia") "/cias/emberclutch.cia" }
