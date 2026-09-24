# Copies the game's screenshots (Y, anywhere; src/app/screenshot.hpp) off the 3DS over Wi-Fi
# with ftpd (default port 5000): sdmc:/3ds/emberclutch/screenshots/shot_NNNN.bmp and log.txt
# (what was going on in each: the scene, frame time, triangles, free memory, the build) into
# build/shots/, each picture also as a PNG. Only reads: nothing on the SD card is changed.
#   tools\pull_shots.ps1 -FtpHost <3ds-ip> [-Port 5000]
param(
    [Parameter(Mandatory = $true)][string]$FtpHost,
    [int]$Port = 5000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$out = Join-Path $root "build\shots"
New-Item -ItemType Directory -Force $out | Out-Null
$remote = "/3ds/emberclutch/screenshots"

function New-Request([string]$path, [string]$method) {
    $req = [System.Net.FtpWebRequest]::Create("ftp://${FtpHost}:${Port}$path")
    $req.Method = $method
    $req.Credentials = New-Object System.Net.NetworkCredential("anonymous", "")
    $req.UseBinary = $true
    $req.UsePassive = $true
    $req.KeepAlive = $false
    $req.Timeout = 60000
    $req.ReadWriteTimeout = 60000
    return $req
}

try {
    $resp = (New-Request "$remote/" ([System.Net.WebRequestMethods+Ftp]::ListDirectory)).GetResponse()
} catch [System.Net.WebException] {
    if ($_.Exception.Response -and $_.Exception.Response.StatusCode -eq [System.Net.FtpStatusCode]::ActionNotTakenFileUnavailable) {
        "No screenshots on the 3DS yet (press Y in the game)."
        return
    }
    throw
}
$reader = New-Object System.IO.StreamReader($resp.GetResponseStream())
$names = @($reader.ReadToEnd() -split "`r?`n" | ForEach-Object { Split-Path $_ -Leaf } | Where-Object { $_ -match '^(shot_\d+\.bmp|log\.txt)$' })
$reader.Close()
$resp.Close()

$new = 0
foreach ($name in $names) {
    $local = Join-Path $out $name
    if ($name -ne "log.txt" -and (Test-Path $local)) { continue }  # pulled before (the log grows: always fresh)
    $resp = (New-Request "$remote/$name" ([System.Net.WebRequestMethods+Ftp]::DownloadFile)).GetResponse()
    $file = [System.IO.File]::Create($local)
    $resp.GetResponseStream().CopyTo($file)
    $file.Close()
    $resp.Close()
    if ($name -like "*.bmp") {
        $img = [System.Drawing.Image]::FromFile($local)
        $img.Save([System.IO.Path]::ChangeExtension($local, ".png"), [System.Drawing.Imaging.ImageFormat]::Png)
        $img.Dispose()
        $new++
    }
}
"$new new screenshot(s) in $out ($(@($names | Where-Object { $_ -like '*.bmp' }).Count) on the 3DS)"
if (Test-Path (Join-Path $out "log.txt")) { Get-Content (Join-Path $out "log.txt") -Tail 10 }
